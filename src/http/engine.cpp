#include "engine.hpp"

#include "../core/segments.hpp"
#include "../core/speed.hpp"
#include "../core/text.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;
using namespace rift::core;

namespace rift::http {

namespace {

int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

fs::path u8path(const std::string &s) { return fs::path(std::u8string(s.begin(), s.end())); }
std::string pathString(const fs::path &p) {
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

std::FILE *openFile(const fs::path &p, bool truncate) {
#ifdef _WIN32
    std::FILE *f = _wfopen(p.c_str(), truncate ? L"w+b" : L"r+b");
#else
    std::FILE *f = std::fopen(p.c_str(), truncate ? "w+b" : "r+b");
#endif
    return f;
}

bool writeAt(std::FILE *f, int64_t off, const char *data, size_t n) {
#ifdef _WIN32
    if (_fseeki64(f, off, SEEK_SET) != 0) return false;
#else
    if (fseeko(f, static_cast<off_t>(off), SEEK_SET) != 0) return false;
#endif
    return std::fwrite(data, 1, n, f) == n;
}

std::string uniqueName(const fs::path &dir, const std::string &name) {
    std::error_code ec;
    auto taken = [&](const std::string &n) {
        return fs::exists(dir / u8path(n), ec) || fs::exists(dir / u8path(n + ".riftpart"), ec);
    };
    if (!taken(name)) return name;
    const size_t dot = name.rfind('.');
    const std::string stem = (dot == std::string::npos || dot == 0) ? name : name.substr(0, dot);
    const std::string ext = (dot == std::string::npos || dot == 0) ? std::string() : name.substr(dot);
    for (int i = 1; i < 10000; ++i) {
        const std::string n = stem + " (" + std::to_string(i) + ")" + ext;
        if (!taken(n)) return n;
    }
    return name;
}

const char *kCaPaths[] = {"/etc/ssl/certs/ca-certificates.crt", "/etc/pki/tls/certs/ca-bundle.crt",
                          "/etc/ssl/ca-bundle.pem", "/etc/pki/tls/cacert.pem", "/etc/ssl/cert.pem"};

} // namespace

struct Download;

struct Conn {
    Download *dl = nullptr;
    CURL *easy = nullptr;
    curl_slist *hdrs = nullptr;
    int worker = 0;
    int64_t offset = 0;   // next file offset this connection writes
    bool probe = false;
    bool decided = false;
    bool jobEnded = false; // stopped on purpose: job finished or stolen
    bool failed = false;   // write-side failure (bad status, disk error)
    std::string failReason;
    int64_t lastByteMs = 0;
    int64_t limit = -1;
    SpeedMeter meter;
    // Parsed response headers (reset on every status line, so redirects don't leak values)
    std::string headerBuf;
    int64_t contentLength = -1;
    std::string contentRange, acceptRanges, etag, lastModified, disposition, retryAfter;
};

bool riftHttpDecide(Conn *c);

struct Download {
    AddRequest req;
    Options opt;
    State state = State::Queued;
    std::string finalUrl, fileName, error;
    int64_t size = -1;
    bool resumable = false;
    std::string etag, lastModified;
    int httpCode = 0;
    std::unique_ptr<SegmentScheduler> sched;
    int64_t singleDone = 0;
    std::FILE *file = nullptr;
    std::vector<std::unique_ptr<Conn>> conns;
    SpeedMeter meter;
    int retries = 0;
    int64_t retryAtMs = 0;
    int64_t nextAddMs = 0;
    int64_t nextRampMs = 0;
    double rampBase = 0;
    int target = 1;
    double firstConnSpeed = 0;
    int nextWorker = 1;

    fs::path partPath() const { return u8path(req.dir) / u8path(fileName + ".riftpart"); }
    fs::path finalPath() const { return u8path(req.dir) / u8path(fileName); }
    int64_t done() const { return sched ? sched->doneBytes() : singleDone; }
};

struct Engine::Impl {
    CURLM *multi = nullptr;
    std::thread thread;
    std::atomic<bool> stop{false};
    mutable std::mutex mtx;                  // guards cmds + snap
    std::deque<std::function<void()>> cmds;
    std::vector<Status> snap;
    std::map<uint64_t, std::unique_ptr<Download>> dls;
    int64_t globalLimit = 0;
    int maxActive = 5;
    std::string caPath;
    int64_t lastTick = 0, lastLimits = 0;

    void post(std::function<void()> f) {
        {
            std::lock_guard<std::mutex> lk(mtx);
            cmds.push_back(std::move(f));
        }
        if (multi) curl_multi_wakeup(multi);
    }

    void run();
    void startConn(Download *dl, bool probe, const Work *job);
    void dropConn(Download *dl, Conn *c, bool releaseWork);
    void onDone(Conn *c, CURLcode res);
    bool decide(Conn *c);
    void begin(Download *dl);
    void finish(Download *dl);
    void fail(Download *dl, const std::string &msg);
    void halt(Download *dl);
    void restartSingle(Download *dl);
    void backoff(Download *dl);
    void schedule(int64_t now);
    void publish();

    static size_t writeCb(char *p, size_t sz, size_t n, void *ud);
    static size_t headerCb(char *p, size_t sz, size_t n, void *ud);
};

// ---------------------------------------------------------------- callbacks

size_t Engine::Impl::headerCb(char *p, size_t sz, size_t n, void *ud) {
    auto *c = static_cast<Conn *>(ud);
    const size_t len = sz * n;
    std::string line(p, len);
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    if (line.rfind("HTTP/", 0) == 0) {
        c->contentLength = -1;
        c->contentRange.clear(); c->acceptRanges.clear(); c->etag.clear();
        c->lastModified.clear(); c->disposition.clear(); c->retryAfter.clear();
        return len;
    }
    const size_t colon = line.find(':');
    if (colon == std::string::npos) return len;
    const std::string name = toLower(trim(line.substr(0, colon)));
    const std::string value = trim(line.substr(colon + 1));
    if (name == "content-length") { try { c->contentLength = std::stoll(value); } catch (...) {} }
    else if (name == "content-range") c->contentRange = value;
    else if (name == "accept-ranges") c->acceptRanges = toLower(value);
    else if (name == "etag") c->etag = value;
    else if (name == "last-modified") c->lastModified = value;
    else if (name == "content-disposition") c->disposition = value;
    else if (name == "retry-after") c->retryAfter = value;
    return len;
}

size_t Engine::Impl::writeCb(char *p, size_t sz, size_t n, void *ud) {
    auto *c = static_cast<Conn *>(ud);
    Download *dl = c->dl;
    const size_t len = sz * n;
    const int64_t now = nowMs();
    if (!c->decided) {
        if (!riftHttpDecide(c)) { c->failed = true; return 0; }
    }
    c->lastByteMs = now;
    if (dl->sched) {
        const int64_t ok = dl->sched->accept(c->worker, c->offset, static_cast<int64_t>(len));
        if (ok > 0) {
            if (!writeAt(dl->file, c->offset, p, static_cast<size_t>(ok))) {
                c->failed = true; c->failReason = "Could not write to disk (is it full?)";
                return 0;
            }
            c->offset += ok;
            c->meter.add(ok, now);
            dl->meter.add(ok, now);
        }
        if (ok < static_cast<int64_t>(len) || !dl->sched->job(c->worker)) {
            c->jobEnded = true;
            return 0; // stop this transfer: the rest of the range belongs to another connection now
        }
        return len;
    }
    if (!writeAt(dl->file, dl->singleDone, p, len)) {
        c->failed = true; c->failReason = "Could not write to disk (is it full?)";
        return 0;
    }
    dl->singleDone += static_cast<int64_t>(len);
    c->meter.add(static_cast<int64_t>(len), now);
    dl->meter.add(static_cast<int64_t>(len), now);
    return len;
}

// The first body bytes arrived: headers are complete. Decide size, name, mode, and open the file.
bool riftHttpDecide(Conn *c) {
    Download *dl = c->dl;
    long code = 0;
    curl_easy_getinfo(c->easy, CURLINFO_RESPONSE_CODE, &code);
    dl->httpCode = static_cast<int>(code);
    c->decided = true;
    const bool isHttp = isHttpUrl(dl->req.url) && toLower(dl->req.url).rfind("ftp", 0) != 0;
    if (isHttp && code >= 400) {
        c->failReason = "Server answered HTTP " + std::to_string(code);
        return false;
    }
    if (!c->probe) {
        // Segment request: must be a 206 starting exactly at our offset.
        if (isHttp && code != 206) {
            c->failReason = code == 200 ? "The file changed on the server (If-Range did not match)" : "Unexpected HTTP " + std::to_string(code);
            return false;
        }
        if (isHttp) {
            const size_t sp = c->contentRange.find(' ');
            const size_t dash = c->contentRange.find('-');
            if (sp != std::string::npos && dash != std::string::npos) {
                try {
                    const long long start = std::stoll(c->contentRange.substr(sp + 1, dash - sp - 1));
                    if (start != c->offset) { c->failReason = "Server sent the wrong range"; return false; }
                } catch (...) {}
            }
        }
        return true;
    }

    // Probe.
    char *eff = nullptr;
    curl_easy_getinfo(c->easy, CURLINFO_EFFECTIVE_URL, &eff);
    if (eff) dl->finalUrl = eff;
    int64_t total = -1;
    bool ranges = false;
    if (code == 206) {
        if (auto t = totalFromContentRange(c->contentRange)) { total = *t; ranges = true; }
    } else {
        total = c->contentLength;
        ranges = total > 0 && c->acceptRanges == "bytes";
    }
    if (!isHttp) { // FTP: size from curl
        curl_off_t cl = -1;
        curl_easy_getinfo(c->easy, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
        total = cl;
        ranges = false;
    }
    dl->size = total;
    dl->resumable = ranges && total > 0;
    dl->etag = c->etag;
    dl->lastModified = c->lastModified;

    if (dl->fileName.empty()) {
        std::string name = fileNameFromContentDisposition(c->disposition);
        if (name.empty()) name = fileNameFromUrl(dl->finalUrl.empty() ? dl->req.url : dl->finalUrl);
        if (name.empty()) name = fileNameFromUrl(dl->req.url);
        dl->fileName = sanitizeFileName(name.empty() ? "download" : name);
    }
    std::error_code ec;
    fs::create_directories(u8path(dl->req.dir), ec);
    dl->fileName = uniqueName(u8path(dl->req.dir), dl->fileName);
    dl->file = openFile(dl->partPath(), true);
    if (!dl->file) {
        c->failReason = "Could not create the file in " + dl->req.dir;
        return false;
    }
    if (dl->resumable) {
        dl->sched = std::make_unique<SegmentScheduler>(total, dl->opt.minSegment);
        auto w = dl->sched->acquire(c->worker);
        c->offset = w ? w->cursor : 0;
    } else {
        dl->sched.reset();
        dl->singleDone = 0;
    }
    dl->state = State::Downloading;
    return true;
}

// ---------------------------------------------------------------- connections

void Engine::Impl::startConn(Download *dl, bool probe, const Work *job) {
    auto c = std::make_unique<Conn>();
    c->dl = dl;
    c->probe = probe;
    c->worker = probe ? 0 : job->worker;
    c->offset = probe ? 0 : job->cursor;
    c->lastByteMs = nowMs();
    CURL *e = curl_easy_init();
    if (!e) return;
    c->easy = e;
    const std::string &url = dl->finalUrl.empty() ? dl->req.url : dl->finalUrl;
    curl_easy_setopt(e, CURLOPT_URL, url.c_str());
    curl_easy_setopt(e, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(e, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(e, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(e, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(e, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(e, CURLOPT_BUFFERSIZE, 256L * 1024L);
    curl_easy_setopt(e, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1); // separate connections per segment
    curl_easy_setopt(e, CURLOPT_WRITEFUNCTION, &Impl::writeCb);
    curl_easy_setopt(e, CURLOPT_WRITEDATA, c.get());
    curl_easy_setopt(e, CURLOPT_HEADERFUNCTION, &Impl::headerCb);
    curl_easy_setopt(e, CURLOPT_HEADERDATA, c.get());
    curl_easy_setopt(e, CURLOPT_PRIVATE, c.get());
    curl_easy_setopt(e, CURLOPT_SSL_VERIFYPEER, dl->opt.verifyTls ? 1L : 0L);
    curl_easy_setopt(e, CURLOPT_SSL_VERIFYHOST, dl->opt.verifyTls ? 2L : 0L);
#if defined(_WIN32) && defined(CURLSSLOPT_NATIVE_CA)
    curl_easy_setopt(e, CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_NATIVE_CA));
#endif
    if (!caPath.empty()) curl_easy_setopt(e, CURLOPT_CAINFO, caPath.c_str());
    curl_easy_setopt(e, CURLOPT_USERAGENT, dl->opt.userAgent.empty() ? "Rift" : dl->opt.userAgent.c_str());
    if (!dl->opt.referer.empty()) curl_easy_setopt(e, CURLOPT_REFERER, dl->opt.referer.c_str());
    if (!dl->opt.cookie.empty()) curl_easy_setopt(e, CURLOPT_COOKIE, dl->opt.cookie.c_str());
    for (const std::string &h : dl->opt.headers) c->hdrs = curl_slist_append(c->hdrs, h.c_str());
    if (probe) {
        curl_easy_setopt(e, CURLOPT_RANGE, "0-");
    } else {
        const std::string r = std::to_string(job->cursor) + "-" + std::to_string(job->end - 1);
        curl_easy_setopt(e, CURLOPT_RANGE, r.c_str()); // libcurl copies the string
        const std::string &v = !dl->etag.empty() ? dl->etag : dl->lastModified;
        if (!v.empty()) c->hdrs = curl_slist_append(c->hdrs, ("If-Range: " + v).c_str());
    }
    if (c->hdrs) curl_easy_setopt(e, CURLOPT_HTTPHEADER, c->hdrs);
    curl_multi_add_handle(multi, e);
    dl->conns.push_back(std::move(c));
}

void Engine::Impl::dropConn(Download *dl, Conn *c, bool releaseWork) {
    if (c->easy) {
        curl_multi_remove_handle(multi, c->easy);
        curl_easy_cleanup(c->easy);
        c->easy = nullptr;
    }
    if (c->hdrs) { curl_slist_free_all(c->hdrs); c->hdrs = nullptr; }
    if (releaseWork && dl->sched) dl->sched->release(c->worker);
    dl->conns.erase(std::remove_if(dl->conns.begin(), dl->conns.end(),
                                   [c](const std::unique_ptr<Conn> &p) { return p.get() == c; }),
                    dl->conns.end());
}

void Engine::Impl::backoff(Download *dl) {
    dl->retries++;
    const int k = std::min(dl->retries, 5);
    dl->retryAtMs = nowMs() + (1000LL << k) / 2 + (dl->retries * 137) % 700;
}

void Engine::Impl::onDone(Conn *c, CURLcode res) {
    Download *dl = c->dl;
    long code = 0;
    curl_easy_getinfo(c->easy, CURLINFO_RESPONSE_CODE, &code);
    const bool wasProbe = c->probe;
    const bool ended = c->jobEnded;
    const bool failed = c->failed;
    const std::string reason = c->failReason;
    const std::string retryAfter = c->retryAfter;
    const bool undecided = !c->decided;

    if (dl->state != State::Downloading && dl->state != State::Probing) { dropConn(dl, c, true); return; }

    // Normal end of a job (finished or handed to another connection).
    if (ended && !failed) {
        dropConn(dl, c, false);
        if (dl->sched && dl->sched->complete()) finish(dl);
        return;
    }
    if (res == CURLE_OK && !failed) {
        if (!dl->sched) {
            // Single connection: transfer ended cleanly.
            if (undecided && !riftHttpDecide(c)) { // empty body
                const std::string why = c->failReason.empty() ? "Download failed" : c->failReason;
                dropConn(dl, c, false);
                fail(dl, why);
                return;
            }
            dropConn(dl, c, false);
            if (dl->size > 0 && dl->singleDone < dl->size) { fail(dl, "The server closed the connection early"); return; }
            dl->size = dl->singleDone;
            finish(dl);
            return;
        }
        dropConn(dl, c, true); // server ended early; the rest goes back to the pool
        if (dl->sched->complete()) finish(dl);
        return;
    }

    // Errors.
    const bool hard = code == 401 || code == 403 || code == 404 || code == 410 || code == 416;
    const bool changed = reason.find("changed") != std::string::npos;
    std::string msg = reason;
    if (msg.empty()) msg = res != CURLE_OK ? curl_easy_strerror(res) : "Download failed";
    if (code == 403 || code == 410) msg = "The link has expired or access was denied (HTTP " + std::to_string(code) + ")";
    if (code == 404) msg = "File not found on the server (HTTP 404)";
    dropConn(dl, c, true);
    if (reason.rfind("Could not", 0) == 0 || changed || (hard && (wasProbe || dl->conns.empty()))) {
        fail(dl, msg);
        return;
    }
    if (dl->retries >= dl->opt.maxRetries) { fail(dl, msg); return; }
    backoff(dl);
    if ((code == 429 || code == 503) && !retryAfter.empty()) {
        try { dl->retryAtMs = nowMs() + std::min<int64_t>(std::stoll(retryAfter), 120) * 1000; } catch (...) {}
    }
    // Server stopped accepting more connections: stop ramping.
    if (code == 429 || code == 503) dl->target = std::max<int>(1, static_cast<int>(dl->conns.size()));
    if (!dl->sched) restartSingle(dl); // no range support: start over
    else if (wasProbe && undecided) dl->state = State::Probing;
    dl->error = msg;
}

// ---------------------------------------------------------------- lifecycle

void Engine::Impl::begin(Download *dl) {
    dl->error.clear();
    dl->retries = 0;
    dl->retryAtMs = 0;
    dl->target = std::clamp(std::min(dl->opt.maxConnections, 4), 1, 32);
    dl->nextRampMs = nowMs() + 2000;
    dl->rampBase = 0;
    std::error_code ec;
    if (dl->resumable && dl->size > 0 && !dl->fileName.empty() && fs::exists(dl->partPath(), ec)) {
        dl->file = openFile(dl->partPath(), false);
        if (dl->file) {
            if (!dl->sched) dl->sched = std::make_unique<SegmentScheduler>(dl->size, dl->opt.minSegment);
            dl->state = State::Downloading;
            return;
        }
    }
    // Fresh start.
    if (!dl->fileName.empty()) fs::remove(dl->partPath(), ec);
    if (dl->req.fileName.empty()) dl->fileName.clear();
    else dl->fileName = sanitizeFileName(dl->req.fileName);
    dl->sched.reset();
    dl->singleDone = 0;
    dl->size = -1;
    dl->state = State::Probing;
}

void Engine::Impl::halt(Download *dl) {
    while (!dl->conns.empty()) dropConn(dl, dl->conns.back().get(), true);
    if (dl->file) { std::fclose(dl->file); dl->file = nullptr; }
}

void Engine::Impl::finish(Download *dl) {
    halt(dl);
    std::error_code ec;
    const fs::path part = dl->partPath();
    if (fs::exists(dl->finalPath(), ec)) {
        // Someone created a file with this name meanwhile: pick "name (1).ext".
        const std::string taken = dl->fileName;
        dl->fileName.clear();
        dl->fileName = uniqueName(u8path(dl->req.dir), taken);
        if (dl->fileName == taken) dl->fileName = "Rift " + taken;
    }
    fs::rename(part, dl->finalPath(), ec);
    if (ec) dl->error = "Finished, but could not rename the file: " + ec.message();
    if (dl->size < 0) dl->size = dl->done();
    dl->state = State::Completed;
}

void Engine::Impl::restartSingle(Download *dl) {
    halt(dl);
    std::error_code ec;
    if (!dl->fileName.empty()) fs::remove(dl->partPath(), ec);
    dl->singleDone = 0;
    dl->state = State::Probing;
}

void Engine::Impl::fail(Download *dl, const std::string &msg) {
    halt(dl);
    dl->error = msg;
    dl->state = State::Error;
}

void Engine::Impl::schedule(int64_t now) {
    int active = 0;
    for (auto &[id, p] : dls)
        if (p->state == State::Probing || p->state == State::Downloading) ++active;
    for (auto &[id, p] : dls) {
        Download *dl = p.get();
        if (dl->state == State::Queued && active < maxActive) { begin(dl); ++active; }
        if (dl->state == State::Probing) {
            if (dl->conns.empty() && now >= dl->retryAtMs) startConn(dl, true, nullptr);
            continue;
        }
        if (dl->state != State::Downloading) continue;

        // Stall detection.
        for (size_t i = 0; i < dl->conns.size();) {
            Conn *c = dl->conns[i].get();
            if (now - c->lastByteMs > dl->opt.stallSeconds * 1000LL) {
                dropConn(dl, c, true);
                backoff(dl);
                if (dl->retries > dl->opt.maxRetries) { fail(dl, "No data received for a long time"); break; }
                continue;
            }
            ++i;
        }
        if (dl->state != State::Downloading) continue;

        if (!dl->sched) {
            if (dl->conns.empty()) restartSingle(dl);
            continue;
        }
        if (dl->sched->complete()) { finish(dl); continue; }

        // Adaptive connection count: ramp while extra connections still raise throughput.
        const double sp = dl->meter.smoothed(now);
        if (dl->conns.size() == 1 && sp > 0) dl->firstConnSpeed = std::max(dl->firstConnSpeed, dl->meter.current(now));
        if (now >= dl->nextRampMs && static_cast<int>(dl->conns.size()) >= dl->target) {
            if (dl->target < dl->opt.maxConnections && (dl->rampBase == 0 || sp > dl->rampBase * 1.05)) dl->target++;
            dl->rampBase = sp;
            dl->nextRampMs = now + 2000;
        }
        if (static_cast<int>(dl->conns.size()) < dl->target && now >= dl->retryAtMs && now >= dl->nextAddMs) {
            const int w = dl->nextWorker++;
            if (auto job = dl->sched->acquire(w)) startConn(dl, false, &*job);
            dl->nextAddMs = now + 300;
        }
    }

    // Speed limits: per download, plus the global limit split evenly over every connection.
    if (now - lastLimits >= 500) {
        lastLimits = now;
        int total = 0;
        for (auto &[id, p] : dls) total += static_cast<int>(p->conns.size());
        const int64_t share = (globalLimit > 0 && total > 0) ? std::max<int64_t>(1024, globalLimit / total) : 0;
        for (auto &[id, p] : dls) {
            const int n = static_cast<int>(p->conns.size());
            const int64_t own = (p->opt.speedLimit > 0 && n > 0) ? std::max<int64_t>(1024, p->opt.speedLimit / n) : 0;
            int64_t lim = 0;
            if (own && share) lim = std::min(own, share);
            else lim = own ? own : share;
            for (auto &c : p->conns)
                if (c->easy && c->limit != lim) {
                    curl_easy_setopt(c->easy, CURLOPT_MAX_RECV_SPEED_LARGE, static_cast<curl_off_t>(lim));
                    c->limit = lim;
                }
        }
    }
}

void Engine::Impl::publish() {
    const int64_t now = nowMs();
    std::vector<Status> out;
    out.reserve(dls.size());
    for (auto &[id, p] : dls) {
        Download *dl = p.get();
        dl->meter.tick(now);
        Status s;
        s.id = id;
        s.state = dl->state;
        s.url = dl->req.url;
        s.finalUrl = dl->finalUrl;
        s.dir = dl->req.dir;
        s.fileName = dl->fileName;
        s.size = dl->size;
        s.done = dl->done();
        s.speed = dl->meter.smoothed(now);
        s.speedNow = dl->meter.current(now);
        s.firstConnSpeed = dl->firstConnSpeed;
        s.resumable = dl->resumable;
        s.retries = dl->retries;
        s.etag = dl->etag;
        s.lastModified = dl->lastModified;
        s.error = dl->error;
        s.httpCode = dl->httpCode;
        if (dl->sched) s.doneRanges = dl->sched->done().serialize();
        for (auto &c : dl->conns) {
            c->meter.tick(now);
            Connection ci;
            if (dl->sched) {
                if (const Work *w = dl->sched->job(c->worker)) { ci.start = w->start; ci.cursor = w->cursor; ci.end = w->end; }
                else continue;
            } else {
                ci.start = 0; ci.cursor = dl->singleDone; ci.end = dl->size > 0 ? dl->size : dl->singleDone;
            }
            ci.speed = c->meter.smoothed(now);
            s.connections.push_back(ci);
        }
        out.push_back(std::move(s));
    }
    std::lock_guard<std::mutex> lk(mtx);
    snap.swap(out);
}

void Engine::Impl::run() {
    while (!stop.load()) {
        std::deque<std::function<void()>> todo;
        {
            std::lock_guard<std::mutex> lk(mtx);
            todo.swap(cmds);
        }
        for (auto &f : todo) f();
        const int64_t now = nowMs();
        schedule(now);
        int running = 0;
        curl_multi_perform(multi, &running);
        int left = 0;
        while (CURLMsg *m = curl_multi_info_read(multi, &left)) {
            if (m->msg != CURLMSG_DONE) continue;
            Conn *c = nullptr;
            curl_easy_getinfo(m->easy_handle, CURLINFO_PRIVATE, reinterpret_cast<char **>(&c));
            const CURLcode res = m->data.result;
            if (c) onDone(c, res);
        }
        if (now - lastTick >= 250 || !todo.empty()) {
            lastTick = now;
            publish();
        }
        curl_multi_poll(multi, nullptr, 0, 100, nullptr);
    }
    for (auto &[id, p] : dls) halt(p.get());
    publish();
}

// ---------------------------------------------------------------- public API

Engine::Engine() : d(std::make_unique<Impl>()) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    d->multi = curl_multi_init();
    curl_multi_setopt(d->multi, CURLMOPT_MAX_HOST_CONNECTIONS, 32L);
#if !defined(_WIN32) && !defined(__APPLE__)
    std::error_code ec;
    for (const char *p : kCaPaths)
        if (fs::exists(p, ec)) { d->caPath = p; break; }
#endif
    d->thread = std::thread([this] { d->run(); });
}

Engine::~Engine() {
    d->stop = true;
    curl_multi_wakeup(d->multi);
    if (d->thread.joinable()) d->thread.join();
    curl_multi_cleanup(d->multi);
    curl_global_cleanup();
}

void Engine::add(const AddRequest &req) {
    d->post([this, req] {
        auto dl = std::make_unique<Download>();
        dl->req = req;
        dl->opt = req.options;
        dl->opt.maxConnections = std::clamp(dl->opt.maxConnections, 1, 32);
        dl->finalUrl = req.finalUrl;
        dl->fileName = req.fileName.empty() ? std::string() : sanitizeFileName(req.fileName);
        dl->size = req.size;
        dl->resumable = req.resumable;
        dl->etag = req.etag;
        dl->lastModified = req.lastModified;
        if (dl->resumable && dl->size > 0) {
            dl->sched = std::make_unique<SegmentScheduler>(dl->size, dl->opt.minSegment);
            dl->sched->setDone(RangeSet::parse(req.doneRanges));
        }
        dl->state = req.startPaused ? State::Paused : State::Queued;
        if (dl->sched && dl->sched->complete()) dl->state = State::Completed;
        d->dls[req.id] = std::move(dl);
    });
}

void Engine::pause(uint64_t id) {
    d->post([this, id] {
        auto it = d->dls.find(id);
        if (it == d->dls.end()) return;
        Download *dl = it->second.get();
        if (dl->state == State::Completed) return;
        d->halt(dl);
        if (!dl->sched) dl->singleDone = 0; // no range support: next start is from zero
        dl->state = State::Paused;
    });
}

void Engine::resume(uint64_t id) {
    d->post([this, id] {
        auto it = d->dls.find(id);
        if (it == d->dls.end()) return;
        Download *dl = it->second.get();
        if (dl->state == State::Paused || dl->state == State::Error) { dl->error.clear(); dl->state = State::Queued; }
    });
}

void Engine::restart(uint64_t id) {
    d->post([this, id] {
        auto it = d->dls.find(id);
        if (it == d->dls.end()) return;
        Download *dl = it->second.get();
        d->halt(dl);
        std::error_code ec;
        if (!dl->fileName.empty()) fs::remove(dl->partPath(), ec);
        dl->sched.reset();
        dl->resumable = false;
        dl->size = -1;
        dl->singleDone = 0;
        dl->etag.clear();
        dl->lastModified.clear();
        dl->finalUrl.clear();
        dl->firstConnSpeed = 0;
        dl->meter.reset();
        dl->state = State::Queued;
    });
}

void Engine::remove(uint64_t id, bool deleteFile) {
    d->post([this, id, deleteFile] {
        auto it = d->dls.find(id);
        if (it == d->dls.end()) return;
        Download *dl = it->second.get();
        d->halt(dl);
        std::error_code ec;
        if (!dl->fileName.empty()) {
            if (dl->state != State::Completed) fs::remove(dl->partPath(), ec); // partial data is useless
            if (deleteFile) fs::remove(dl->finalPath(), ec);
        }
        d->dls.erase(it);
    });
}

void Engine::setOptions(uint64_t id, const Options &o) {
    d->post([this, id, o] {
        auto it = d->dls.find(id);
        if (it == d->dls.end()) return;
        Download *dl = it->second.get();
        dl->opt = o;
        dl->opt.maxConnections = std::clamp(dl->opt.maxConnections, 1, 32);
        dl->target = std::min(dl->target, dl->opt.maxConnections);
        d->lastLimits = 0;
    });
}

void Engine::setGlobalLimit(int64_t bps) {
    d->post([this, bps] { d->globalLimit = std::max<int64_t>(0, bps); d->lastLimits = 0; });
}

void Engine::setMaxActive(int n) {
    d->post([this, n] { d->maxActive = std::max(1, n); });
}

std::vector<Status> Engine::snapshot() const {
    std::lock_guard<std::mutex> lk(d->mtx);
    return d->snap;
}

std::string Engine::version() { return curl_version(); }

} // namespace rift::http
