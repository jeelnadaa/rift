#include "torrent.hpp"

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/announce_entry.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/fingerprint.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/peer_info.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/version.hpp>
#include <libtorrent/write_resume_data.hpp>
#ifdef _WIN32
#include <libtorrent/posix_disk_io.hpp>
#endif

#include <atomic>
#include <cstdio>
#include <iterator>
#include <chrono>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace lt = libtorrent;

namespace rift::bt {

namespace {

std::string hex(const char *p, size_t n) {
    static const char *k = "0123456789abcdef";
    std::string s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) {
        const auto b = static_cast<unsigned char>(p[i]);
        s += k[b >> 4];
        s += k[b & 15];
    }
    return s;
}

template <typename H> std::string hexOf(const H &h) { return hex(h.data(), static_cast<size_t>(h.size())); }

std::string idOf(const lt::info_hash_t &ih) { return hexOf(ih.get_best()); }

lt::settings_pack makePack(const Settings &s) {
    lt::settings_pack p;
    const std::string port = std::to_string(s.listenPort > 0 ? s.listenPort : 0);
    if (s.networkInterface.empty())
        p.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:" + port + ",[::]:" + port);
    else {
        p.set_str(lt::settings_pack::listen_interfaces, s.networkInterface + ":" + port);
        p.set_str(lt::settings_pack::outgoing_interfaces, s.networkInterface);
    }
    p.set_bool(lt::settings_pack::enable_dht, s.dht);
    p.set_bool(lt::settings_pack::enable_lsd, s.lsd);
    p.set_bool(lt::settings_pack::enable_upnp, s.upnp);
    p.set_bool(lt::settings_pack::enable_natpmp, s.natpmp);
    const int pol = s.encryption == 1 ? lt::settings_pack::pe_forced
                    : s.encryption == 2 ? lt::settings_pack::pe_disabled
                                        : lt::settings_pack::pe_enabled;
    p.set_int(lt::settings_pack::out_enc_policy, pol);
    p.set_int(lt::settings_pack::in_enc_policy, pol);
    p.set_int(lt::settings_pack::allowed_enc_level, lt::settings_pack::pe_both);
    p.set_bool(lt::settings_pack::anonymous_mode, s.anonymous);
    p.set_int(lt::settings_pack::download_rate_limit, s.downloadLimit);
    p.set_int(lt::settings_pack::upload_rate_limit, s.uploadLimit);
    p.set_int(lt::settings_pack::connections_limit, s.maxConnections);
    p.set_int(lt::settings_pack::unchoke_slots_limit, s.maxUploads);
    p.set_int(lt::settings_pack::active_downloads, s.queueing ? s.activeDownloads : -1);
    p.set_int(lt::settings_pack::active_seeds, s.queueing ? s.activeSeeds : -1);
    p.set_int(lt::settings_pack::active_limit, s.queueing ? s.activeLimit : -1);
    p.set_int(lt::settings_pack::active_checking, 1);
    p.set_int(lt::settings_pack::max_queued_disk_bytes, 16 * 1024 * 1024);
    p.set_int(lt::settings_pack::send_buffer_watermark, 3 * 1024 * 1024);
    p.set_int(lt::settings_pack::send_buffer_low_watermark, 512 * 1024);
    p.set_int(lt::settings_pack::mixed_mode_algorithm, lt::settings_pack::peer_proportional);
    p.set_str(lt::settings_pack::user_agent, "Rift/" + s.version);
    int a = 1, b = 0, c = 0;
    std::sscanf(s.version.c_str(), "%d.%d.%d", &a, &b, &c);
    p.set_str(lt::settings_pack::peer_fingerprint, lt::generate_fingerprint("RF", a, b, c % 10, 0));
    p.set_int(lt::settings_pack::alert_mask, lt::alert_category::status | lt::alert_category::error |
                                                 lt::alert_category::storage | lt::alert_category::port_mapping |
                                                 lt::alert_category::dht);
    if (s.proxyType > 0 && !s.proxyHost.empty()) {
        p.set_int(lt::settings_pack::proxy_type, s.proxyType == 1 ? lt::settings_pack::socks5 : lt::settings_pack::http);
        p.set_str(lt::settings_pack::proxy_hostname, s.proxyHost);
        p.set_int(lt::settings_pack::proxy_port, s.proxyPort);
        p.set_bool(lt::settings_pack::proxy_peer_connections, true);
        p.set_bool(lt::settings_pack::proxy_tracker_connections, true);
    } else {
        p.set_int(lt::settings_pack::proxy_type, lt::settings_pack::none);
    }
    return p;
}

std::string stateOf(const lt::torrent_status &st) {
    if (st.errc) return "error";
    if (st.moving_storage) return "moving";
    const bool paused = static_cast<bool>(st.flags & lt::torrent_flags::paused);
    const bool managed = static_cast<bool>(st.flags & lt::torrent_flags::auto_managed);
    if (paused) return managed ? "queued" : "paused";
    switch (st.state) {
    case lt::torrent_status::checking_files:
    case lt::torrent_status::checking_resume_data: return "checking";
    case lt::torrent_status::downloading_metadata: return "metadata";
    case lt::torrent_status::downloading: return st.download_payload_rate > 0 ? "downloading" : "stalled";
    case lt::torrent_status::finished: return "completed";
    case lt::torrent_status::seeding: return "seeding";
    default: return "downloading";
    }
}

} // namespace

struct Engine::Impl {
    std::unique_ptr<lt::session> ses;
    std::thread alertThread;
    std::atomic<bool> stop{false};
    std::atomic<int> pendingResume{0};
    mutable std::mutex mtx;
    std::map<std::string, lt::torrent_handle> handles;
    std::map<std::string, lt::torrent_status> status;
    std::vector<Event> events;
    SessionInfo info;
    std::chrono::steady_clock::time_point lastDht{};
    std::function<void(const std::string &, const std::vector<char> &)> *onResume = nullptr;

    lt::torrent_handle find(const std::string &id) const {
        std::lock_guard<std::mutex> lk(mtx);
        auto it = handles.find(id);
        return it == handles.end() ? lt::torrent_handle() : it->second;
    }
    void push(Event e) {
        std::lock_guard<std::mutex> lk(mtx);
        events.push_back(std::move(e));
    }
    void handle(lt::alert *a);
    void loop();
};

void Engine::Impl::handle(lt::alert *a) {
    if (auto *x = lt::alert_cast<lt::add_torrent_alert>(a)) {
        if (x->error) {
            push({Event::AddFailed, idOf(x->params.info_hashes), x->params.name, x->error.message()});
            return;
        }
        const std::string id = idOf(x->handle.info_hashes());
        {
            std::lock_guard<std::mutex> lk(mtx);
            handles[id] = x->handle;
            status[id] = x->handle.status();
        }
        push({Event::Added, id, x->handle.status(lt::torrent_handle::query_name).name, {}});
        if (x->handle.torrent_file()) x->handle.save_resume_data(lt::torrent_handle::save_info_dict);
    } else if (auto *x = lt::alert_cast<lt::state_update_alert>(a)) {
        std::lock_guard<std::mutex> lk(mtx);
        for (const lt::torrent_status &st : x->status) status[idOf(st.info_hashes)] = st;
    } else if (auto *x = lt::alert_cast<lt::metadata_received_alert>(a)) {
        const std::string id = idOf(x->handle.info_hashes());
        push({Event::Metadata, id, x->handle.status(lt::torrent_handle::query_name).name, {}});
        x->handle.save_resume_data(lt::torrent_handle::save_info_dict);
    } else if (auto *x = lt::alert_cast<lt::torrent_finished_alert>(a)) {
        const std::string id = idOf(x->handle.info_hashes());
        push({Event::Finished, id, x->handle.status(lt::torrent_handle::query_name).name, {}});
        x->handle.save_resume_data(lt::torrent_handle::save_info_dict);
    } else if (auto *x = lt::alert_cast<lt::torrent_error_alert>(a)) {
        push({Event::Error, idOf(x->handle.info_hashes()), {}, x->error.message()});
    } else if (auto *x = lt::alert_cast<lt::file_error_alert>(a)) {
        push({Event::Error, idOf(x->handle.info_hashes()), {}, x->error.message() + " (" + x->filename() + ")"});
    } else if (auto *x = lt::alert_cast<lt::save_resume_data_alert>(a)) {
        const std::vector<char> buf = lt::write_resume_data_buf(x->params);
        if (onResume && *onResume) (*onResume)(idOf(x->params.info_hashes), buf);
        --pendingResume;
    } else if (lt::alert_cast<lt::save_resume_data_failed_alert>(a)) {
        --pendingResume;
    } else if (auto *x = lt::alert_cast<lt::torrent_removed_alert>(a)) {
        const std::string id = idOf(x->info_hashes);
        {
            std::lock_guard<std::mutex> lk(mtx);
            handles.erase(id);
            status.erase(id);
        }
        push({Event::Removed, id, {}, {}});
    } else if (auto *x = lt::alert_cast<lt::storage_moved_alert>(a)) {
        push({Event::Moved, idOf(x->handle.info_hashes()), {}, x->storage_path()});
        x->handle.save_resume_data();
    } else if (auto *x = lt::alert_cast<lt::storage_moved_failed_alert>(a)) {
        push({Event::MoveFailed, idOf(x->handle.info_hashes()), {}, x->error.message()});
    } else if (auto *x = lt::alert_cast<lt::listen_succeeded_alert>(a)) {
        std::lock_guard<std::mutex> lk(mtx);
        info.listening = true;
        info.listenPort = x->port;
    } else if (lt::alert_cast<lt::listen_failed_alert>(a)) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!info.listenPort) info.listening = false;
    } else if (lt::alert_cast<lt::portmap_alert>(a)) {
        std::lock_guard<std::mutex> lk(mtx);
        info.portMapped = 1;
    } else if (lt::alert_cast<lt::portmap_error_alert>(a)) {
        std::lock_guard<std::mutex> lk(mtx);
        if (info.portMapped != 1) info.portMapped = -1;
    } else if (auto *x = lt::alert_cast<lt::external_ip_alert>(a)) {
        std::lock_guard<std::mutex> lk(mtx);
        info.externalIp = x->external_address.to_string();
    } else if (auto *x = lt::alert_cast<lt::dht_stats_alert>(a)) {
        int n = 0;
        for (const auto &b : x->routing_table) n += b.num_nodes;
        std::lock_guard<std::mutex> lk(mtx);
        info.dhtNodes = n;
    }
}

void Engine::Impl::loop() {
    auto lastUpdate = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    while (!stop.load()) {
        ses->wait_for_alert(std::chrono::milliseconds(250));
        std::vector<lt::alert *> alerts;
        ses->pop_alerts(&alerts);
        for (lt::alert *a : alerts) handle(a);
        const auto now = std::chrono::steady_clock::now();
        if (now - lastUpdate >= std::chrono::milliseconds(500)) {
            ses->post_torrent_updates();
            lastUpdate = now;
        }
        if (now - lastDht >= std::chrono::seconds(5)) {
            ses->post_dht_stats();
            lastDht = now;
        }
    }
}

Engine::Engine(const Settings &s, const std::vector<char> &state) : d(std::make_unique<Impl>()) {
    lt::session_params params;
    if (!state.empty()) {
        try {
            params = lt::read_session_params(lt::span<char const>(state.data(), static_cast<std::ptrdiff_t>(state.size())),
                                             lt::session::save_dht_state);
        } catch (...) {
            params = lt::session_params();
        }
    }
    params.settings = makePack(s);
#ifdef _WIN32
    params.disk_io_constructor = lt::posix_disk_io_constructor; // PLAN 4.4: avoids mmap working-set issues on Windows
#endif
    d->ses = std::make_unique<lt::session>(std::move(params));
    d->onResume = &onResumeData;
    d->alertThread = std::thread([this] { d->loop(); });
}

Engine::~Engine() {
    if (!d->stop.load()) shutdown(2000);
}

void Engine::apply(const Settings &s) { d->ses->apply_settings(makePack(s)); }

static void applyAddOptions(lt::add_torrent_params &p, const Engine::AddOptions &o) {
    p.save_path = o.savePath;
    if (o.paused) {
        p.flags |= lt::torrent_flags::paused;
        p.flags &= ~lt::torrent_flags::auto_managed;
    }
    if (o.sequential) p.flags |= lt::torrent_flags::sequential_download;
    if (!o.filePriorities.empty()) {
        p.file_priorities.clear();
        for (int v : o.filePriorities) p.file_priorities.push_back(lt::download_priority_t(static_cast<std::uint8_t>(v)));
    }
    for (const std::string &t : o.trackers) p.trackers.push_back(t);
}

std::string Engine::addMagnet(const std::string &magnet, const AddOptions &o, std::string &error) {
    lt::error_code ec;
    lt::add_torrent_params p = lt::parse_magnet_uri(magnet, ec);
    if (ec) { error = "Not a valid magnet link: " + ec.message(); return {}; }
    const std::string id = idOf(p.info_hashes);
    if (has(id)) { error = "This torrent is already in the list"; return id; }
    applyAddOptions(p, o);
    d->ses->async_add_torrent(std::move(p));
    return id;
}

std::string Engine::addTorrent(const std::vector<char> &data, const AddOptions &o, std::string &error) {
    lt::error_code ec;
    auto ti = std::make_shared<lt::torrent_info>(lt::span<char const>(data.data(), static_cast<std::ptrdiff_t>(data.size())), ec,
                                                 lt::from_span);
    if (ec) { error = "Not a valid .torrent file: " + ec.message(); return {}; }
    lt::add_torrent_params p;
    p.ti = ti;
    const std::string id = idOf(ti->info_hashes());
    if (has(id)) { error = "This torrent is already in the list"; return id; }
    applyAddOptions(p, o);
    d->ses->async_add_torrent(std::move(p));
    return id;
}

std::string Engine::addResume(const std::vector<char> &buf, std::string &error) {
    lt::error_code ec;
    lt::add_torrent_params p = lt::read_resume_data(lt::span<char const>(buf.data(), static_cast<std::ptrdiff_t>(buf.size())), ec);
    if (ec) { error = ec.message(); return {}; }
    const std::string id = idOf(p.info_hashes);
    d->ses->async_add_torrent(std::move(p));
    return id;
}

bool Engine::inspect(const std::vector<char> &data, Preview &out, std::string &error) {
    lt::error_code ec;
    lt::torrent_info ti(lt::span<char const>(data.data(), static_cast<std::ptrdiff_t>(data.size())), ec, lt::from_span);
    if (ec) { error = ec.message(); return false; }
    out.name = ti.name();
    out.hash = idOf(ti.info_hashes());
    out.comment = ti.comment();
    out.creator = ti.creator();
    out.size = ti.total_size();
    out.isPrivate = ti.priv();
    const lt::file_storage &fs = ti.files();
    for (const lt::file_index_t i : fs.file_range()) {
        if (fs.pad_file_at(i)) continue;
        FileEntry f;
        f.index = static_cast<int>(i);
        f.path = fs.file_path(i);
        f.size = fs.file_size(i);
        out.files.push_back(f);
    }
    return true;
}

bool Engine::has(const std::string &id) const {
    std::lock_guard<std::mutex> lk(d->mtx);
    return d->handles.count(id) > 0;
}

void Engine::pause(const std::string &id) {
    if (auto h = d->find(id); h.is_valid()) { h.unset_flags(lt::torrent_flags::auto_managed); h.pause(lt::torrent_handle::graceful_pause); h.save_resume_data(); }
}
void Engine::resume(const std::string &id) {
    if (auto h = d->find(id); h.is_valid()) { h.set_flags(lt::torrent_flags::auto_managed); h.resume(); }
}
void Engine::forceStart(const std::string &id) {
    if (auto h = d->find(id); h.is_valid()) { h.unset_flags(lt::torrent_flags::auto_managed); h.resume(); }
}
void Engine::remove(const std::string &id, bool deleteFiles) {
    if (auto h = d->find(id); h.is_valid())
        d->ses->remove_torrent(h, deleteFiles ? lt::session::delete_files : lt::remove_flags_t{});
}
void Engine::recheck(const std::string &id) {
    if (auto h = d->find(id); h.is_valid()) h.force_recheck();
}
void Engine::reannounce(const std::string &id) {
    if (auto h = d->find(id); h.is_valid()) { h.force_reannounce(); h.force_dht_announce(); }
}
void Engine::setSequential(const std::string &id, bool on) {
    if (auto h = d->find(id); h.is_valid()) {
        if (on) h.set_flags(lt::torrent_flags::sequential_download);
        else h.unset_flags(lt::torrent_flags::sequential_download);
    }
}
void Engine::setFirstLast(const std::string &id, bool on) {
    auto h = d->find(id);
    if (!h.is_valid()) return;
    auto ti = h.torrent_file();
    if (!ti) return;
    const lt::file_storage &fs = ti->files();
    const auto prio = on ? lt::top_priority : lt::default_priority;
    for (const lt::file_index_t i : fs.file_range()) {
        if (fs.pad_file_at(i) || fs.file_size(i) == 0) continue;
        const auto first = fs.map_file(i, 0, 1).piece;
        const auto last = fs.map_file(i, std::max<std::int64_t>(0, fs.file_size(i) - 1), 1).piece;
        h.piece_priority(first, prio);
        h.piece_priority(last, prio);
    }
}
void Engine::setFilePriority(const std::string &id, int index, int priority) {
    if (auto h = d->find(id); h.is_valid())
        h.file_priority(lt::file_index_t(index), lt::download_priority_t(static_cast<std::uint8_t>(priority)));
}
void Engine::setLimits(const std::string &id, int down, int up) {
    if (auto h = d->find(id); h.is_valid()) { h.set_download_limit(down); h.set_upload_limit(up); }
}
void Engine::queue(const std::string &id, QueueOp op) {
    auto h = d->find(id);
    if (!h.is_valid()) return;
    switch (op) {
    case QueueOp::Up: h.queue_position_up(); break;
    case QueueOp::Down: h.queue_position_down(); break;
    case QueueOp::Top: h.queue_position_top(); break;
    case QueueOp::Bottom: h.queue_position_bottom(); break;
    }
}
void Engine::moveStorage(const std::string &id, const std::string &path) {
    if (auto h = d->find(id); h.is_valid()) h.move_storage(path);
}
void Engine::addTracker(const std::string &id, const std::string &url) {
    if (auto h = d->find(id); h.is_valid()) h.add_tracker(lt::announce_entry(url));
}
void Engine::pauseAll() {
    std::vector<lt::torrent_handle> hs;
    {
        std::lock_guard<std::mutex> lk(d->mtx);
        for (auto &[id, h] : d->handles) hs.push_back(h);
    }
    for (auto &h : hs) { h.unset_flags(lt::torrent_flags::auto_managed); h.pause(lt::torrent_handle::graceful_pause); }
}
void Engine::resumeAll() {
    std::vector<lt::torrent_handle> hs;
    {
        std::lock_guard<std::mutex> lk(d->mtx);
        for (auto &[id, h] : d->handles) hs.push_back(h);
    }
    for (auto &h : hs) { h.set_flags(lt::torrent_flags::auto_managed); h.resume(); }
}

std::string Engine::magnet(const std::string &id) const {
    auto h = d->find(id);
    if (!h.is_valid()) return {};
    try { return lt::make_magnet_uri(h); } catch (...) { return {}; }
}

bool Engine::exportTorrent(const std::string &id, std::vector<char> &out) {
    auto h = d->find(id);
    if (!h.is_valid()) return false;
    auto ti = h.torrent_file();
    if (!ti) return false;
    lt::create_torrent ct(*ti);
    lt::bencode(std::back_inserter(out), ct.generate());
    return !out.empty();
}

std::vector<TorrentStatus> Engine::snapshot() const {
    std::vector<std::pair<std::string, lt::torrent_status>> copy;
    {
        std::lock_guard<std::mutex> lk(d->mtx);
        copy.assign(d->status.begin(), d->status.end());
    }
    std::vector<TorrentStatus> out;
    out.reserve(copy.size());
    for (auto &[id, st] : copy) {
        TorrentStatus t;
        t.id = id;
        t.name = st.name.empty() ? id : st.name;
        t.savePath = st.save_path;
        t.error = st.errc ? st.errc.message() : std::string();
        t.state = stateOf(st);
        t.size = st.has_metadata ? st.total_wanted : 0;
        t.done = st.total_wanted_done;
        t.downloaded = st.all_time_download;
        t.uploaded = st.all_time_upload;
        t.progress = st.has_metadata ? (st.total_wanted > 0 ? static_cast<double>(st.total_wanted_done) / static_cast<double>(st.total_wanted) : 1.0) : 0.0;
        t.ratio = st.all_time_download > 0 ? static_cast<double>(st.all_time_upload) / static_cast<double>(st.all_time_download) : 0.0;
        t.availability = st.distributed_copies;
        t.down = st.download_payload_rate;
        t.up = st.upload_payload_rate;
        t.seeds = st.num_seeds;
        t.peers = st.num_peers - st.num_seeds;
        t.seedsTotal = st.num_complete;
        t.peersTotal = st.num_incomplete;
        t.queuePos = static_cast<int>(st.queue_position);
        t.hasMetadata = st.has_metadata;
        t.sequential = static_cast<bool>(st.flags & lt::torrent_flags::sequential_download);
        t.autoManaged = static_cast<bool>(st.flags & lt::torrent_flags::auto_managed);
        t.forced = !t.autoManaged && !(st.flags & lt::torrent_flags::paused);
        t.addedTime = static_cast<int64_t>(st.added_time);
        t.completedTime = static_cast<int64_t>(st.completed_time);
        t.currentTracker = st.current_tracker;
        if (st.info_hashes.has_v1()) t.hashV1 = hexOf(st.info_hashes.v1);
        if (st.info_hashes.has_v2()) t.hashV2 = hexOf(st.info_hashes.v2);
        if (auto ti = st.torrent_file.lock()) {
            t.isPrivate = ti->priv();
            t.comment = ti->comment();
            t.creator = ti->creator();
            t.numFiles = ti->num_files();
            t.numPieces = ti->num_pieces();
            t.pieceLength = ti->piece_length();
            t.totalSize = ti->total_size();
        }
        t.downLimit = 0;
        t.upLimit = 0;
        out.push_back(std::move(t));
    }
    return out;
}

std::vector<FileEntry> Engine::files(const std::string &id) const {
    std::vector<FileEntry> out;
    auto h = d->find(id);
    if (!h.is_valid()) return out;
    auto ti = h.torrent_file();
    if (!ti) return out;
    std::vector<std::int64_t> progress;
    h.file_progress(progress, lt::torrent_handle::piece_granularity);
    const auto prios = h.get_file_priorities();
    const lt::file_storage &fs = ti->files();
    for (const lt::file_index_t i : fs.file_range()) {
        if (fs.pad_file_at(i)) continue;
        FileEntry f;
        const int idx = static_cast<int>(i);
        f.index = idx;
        f.path = fs.file_path(i);
        f.size = fs.file_size(i);
        f.done = idx < static_cast<int>(progress.size()) ? progress[static_cast<size_t>(idx)] : 0;
        f.priority = idx < static_cast<int>(prios.size()) ? static_cast<int>(static_cast<std::uint8_t>(prios[static_cast<size_t>(idx)])) : 4;
        out.push_back(f);
    }
    return out;
}

std::vector<Peer> Engine::peers(const std::string &id) const {
    std::vector<Peer> out;
    auto h = d->find(id);
    if (!h.is_valid()) return out;
    std::vector<lt::peer_info> v;
    h.get_peer_info(v);
    for (const lt::peer_info &p : v) {
        Peer x;
        x.ip = p.ip.address().to_string();
        x.port = p.ip.port();
        x.client = p.client;
        x.progress = p.progress;
        x.down = p.payload_down_speed;
        x.up = p.payload_up_speed;
        x.utp = static_cast<bool>(p.flags & lt::peer_info::utp_socket);
        x.encrypted = static_cast<bool>(p.flags & (lt::peer_info::rc4_encrypted | lt::peer_info::plaintext_encrypted));
        x.seed = static_cast<bool>(p.flags & lt::peer_info::seed);
        std::string f;
        if (p.flags & lt::peer_info::interesting) f += 'D';
        if (p.flags & lt::peer_info::remote_interested) f += 'U';
        if (p.flags & lt::peer_info::choked) f += 'c';
        if (x.encrypted) f += 'E';
        if (x.utp) f += 'P';
        if (p.source & lt::peer_info::dht) f += 'H';
        if (p.source & lt::peer_info::pex) f += 'X';
        if (p.source & lt::peer_info::lsd) f += 'L';
        x.flags = f;
        out.push_back(std::move(x));
    }
    return out;
}

std::vector<Tracker> Engine::trackers(const std::string &id) const {
    std::vector<Tracker> out;
    auto h = d->find(id);
    if (!h.is_valid()) return out;
    for (const lt::announce_entry &ae : h.trackers()) {
        Tracker t;
        t.url = ae.url;
        t.tier = ae.tier;
        t.status = ae.verified ? "Working" : "Not contacted";
        for (const lt::announce_endpoint &ep : ae.endpoints) {
            for (const lt::announce_infohash &ih : ep.info_hashes) {
                if (ih.updating) t.status = "Updating";
                else if (ih.last_error) { t.status = "Error"; t.message = ih.last_error.message(); }
                else if (ih.start_sent || ih.complete_sent) t.status = "Working";
                if (!ih.message.empty()) t.message = ih.message;
                t.seeds = std::max(t.seeds, ih.scrape_complete);
                t.peers = std::max(t.peers, ih.scrape_incomplete);
                t.downloaded = std::max(t.downloaded, ih.scrape_downloaded);
            }
        }
        out.push_back(std::move(t));
    }
    return out;
}

std::vector<uint8_t> Engine::pieceMap(const std::string &id, int buckets) const {
    std::vector<uint8_t> out;
    auto h = d->find(id);
    if (!h.is_valid() || buckets <= 0) return out;
    const lt::torrent_status st = h.status(lt::torrent_handle::query_pieces);
    const int n = st.pieces.size();
    if (n <= 0) return out;
    out.assign(static_cast<size_t>(buckets), 0);
    for (int b = 0; b < buckets; ++b) {
        const int from = static_cast<int>(static_cast<int64_t>(n) * b / buckets);
        int to = static_cast<int>(static_cast<int64_t>(n) * (b + 1) / buckets);
        if (to <= from) to = from + 1;
        int have = 0, total = 0;
        for (int i = from; i < to && i < n; ++i, ++total)
            if (st.pieces.get_bit(lt::piece_index_t(i))) ++have;
        out[static_cast<size_t>(b)] = total ? static_cast<uint8_t>(have * 255 / total) : 0;
    }
    return out;
}

SessionInfo Engine::session() const {
    std::lock_guard<std::mutex> lk(d->mtx);
    SessionInfo s = d->info;
    for (auto &[id, st] : d->status) { s.totalDown += st.total_payload_download; s.totalUp += st.total_payload_upload; }
    return s;
}

std::vector<Event> Engine::takeEvents() {
    std::lock_guard<std::mutex> lk(d->mtx);
    std::vector<Event> out;
    out.swap(d->events);
    return out;
}

void Engine::saveResumeData() {
    std::vector<lt::torrent_handle> hs;
    {
        std::lock_guard<std::mutex> lk(d->mtx);
        for (auto &[id, h] : d->handles) hs.push_back(h);
    }
    for (auto &h : hs) {
        if (h.need_save_resume_data()) {
            ++d->pendingResume;
            h.save_resume_data(lt::torrent_handle::save_info_dict | lt::torrent_handle::only_if_modified);
        }
    }
}

std::vector<char> Engine::shutdown(int timeoutMs) {
    if (d->stop.exchange(true)) return {};
    if (d->alertThread.joinable()) d->alertThread.join();
    d->ses->pause();
    std::vector<lt::torrent_handle> hs;
    {
        std::lock_guard<std::mutex> lk(d->mtx);
        for (auto &[id, h] : d->handles) hs.push_back(h);
    }
    d->pendingResume = 0;
    for (auto &h : hs) {
        ++d->pendingResume;
        h.save_resume_data(lt::torrent_handle::save_info_dict);
    }
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (d->pendingResume.load() > 0 && std::chrono::steady_clock::now() < until) {
        d->ses->wait_for_alert(std::chrono::milliseconds(100));
        std::vector<lt::alert *> alerts;
        d->ses->pop_alerts(&alerts);
        for (lt::alert *a : alerts) d->handle(a);
    }
    std::vector<char> state = lt::write_session_params_buf(d->ses->session_state(lt::session::save_dht_state));
    d->ses.reset();
    return state;
}

std::string Engine::version() { return LIBTORRENT_VERSION; }

} // namespace rift::bt
