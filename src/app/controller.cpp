#include "controller.hpp"

#include "platform.hpp"
#include "version.hpp"

#include "../core/arbiter.hpp"
#include "../core/segments.hpp"
#include "../core/text.hpp"
#include "../engine/torrent.hpp"
#include "../http/engine.hpp"

#include <QAction>
#include <QIcon>
#include <QJsonArray>
#include <QMap>
#include <QRegularExpression>
#include <algorithm>
#include <cstdlib>
#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLocale>
#include <QMenu>
#include <QMutexLocker>
#include <QNetworkReply>
#include <QSet>
#include <QStandardPaths>
#include <QSysInfo>
#include <QThreadPool>
#include <QUrl>

using namespace rift;

namespace {
std::string u(const QString &s) { return s.toUtf8().toStdString(); }
QString q(const std::string &s) { return QString::fromUtf8(s.data(), static_cast<qsizetype>(s.size())); }
qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }

QString httpStateName(http::State s, double speed) {
    switch (s) {
    case http::State::Queued: return QStringLiteral("queued");
    case http::State::Probing: return QStringLiteral("probing");
    case http::State::Downloading: return speed > 0 ? QStringLiteral("downloading") : QStringLiteral("stalled");
    case http::State::Paused: return QStringLiteral("paused");
    case http::State::Completed: return QStringLiteral("completed");
    case http::State::Error: return QStringLiteral("error");
    }
    return QStringLiteral("queued");
}

QString localPath(const QString &s) {
    if (s.startsWith(QLatin1String("file:"))) return QUrl(s).toLocalFile();
    return s;
}
} // namespace

Controller::Controller(const QString &dataDir, bool portable, QObject *parent)
    : QObject(parent), m_dataDir(dataDir), m_portable(portable) {
    m_settings = new Settings(dataDir + "/settings.ini", this);
    m_filter = new DownloadFilter(this);
    m_filter->setSourceModel(&m_model);
    m_filter->setSortKey(m_settings->s("sortKey"));
    m_filter->setAscending(m_settings->b("sortAsc"));
    connect(m_filter, &DownloadFilter::changed, this, [this] {
        m_settings->set("sortKey", m_filter->sortKey());
        m_settings->set("sortAsc", m_filter->ascending());
    });
    connect(m_settings, &Settings::changed, this, [this](const QString &k) { applySettings(k); });
    connect(&m_timer, &QTimer::timeout, this, &Controller::tick);
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &Controller::checkClipboard);
}

Controller::~Controller() { shutdown(); }

QString Controller::version() const { return QStringLiteral(RIFT_VERSION); }

QString Controller::engineVersions() const {
    return QStringLiteral("libtorrent %1 · %2 · Qt %3").arg(q(bt::Engine::version()), q(http::Engine::version()).section(' ', 0, 0), QString::fromLatin1(qVersion()));
}

bool Controller::start(QString *error) {
    QDir().mkpath(m_dataDir);
    if (!m_store.open(m_dataDir + "/rift.db", error)) return false;

    // Torrent engine
    const QByteArray state = m_store.blob("session_state");
    m_bt = std::make_unique<bt::Engine>(bt::Settings{}, std::vector<char>(state.begin(), state.end()));
    m_bt->onResumeData = [this](const std::string &id, const std::vector<char> &data) {
        QMutexLocker lk(&m_resumeMutex);
        m_resumePending.insert(q(id), QByteArray(data.data(), static_cast<qsizetype>(data.size())));
    };
    applyBtSettings();
    for (const TorrentRecord &r : m_store.loadTorrents()) m_btRec.insert(r.id, r);
    for (const auto &[id, blob] : m_store.loadResume()) {
        std::string err;
        m_bt->addResume(std::vector<char>(blob.begin(), blob.end()), err);
        if (!err.empty()) qWarning("rift: could not restore %s: %s", qPrintable(id), err.c_str());
    }

    // HTTP engine
    m_http = std::make_unique<http::Engine>();
    applyHttpSettings();
    for (const HttpRecord &r : m_store.loadHttp()) {
        m_httpRec.insert(r.id, r);
        m_httpPrevState.insert(r.id, r.state);
        if (r.state == QLatin1String("completed")) continue; // finished: shown from the record
        http::AddRequest req;
        req.id = static_cast<uint64_t>(r.id);
        req.url = u(r.url);
        req.dir = u(r.dir);
        req.fileName = u(r.fileName);
        req.size = r.size;
        req.doneRanges = u(r.doneRanges);
        req.etag = u(r.etag);
        req.lastModified = u(r.lastModified);
        req.finalUrl = u(r.finalUrl);
        req.resumable = r.resumable;
        req.startPaused = r.state == QLatin1String("paused") || r.state == QLatin1String("error");
        req.options.maxConnections = r.connections > 0 ? r.connections : m_settings->i("httpConnections");
        req.options.speedLimit = r.speedLimit;
        req.options.referer = u(r.referer);
        req.options.userAgent = u(m_settings->s("httpUserAgent"));
        req.options.verifyTls = m_settings->b("httpVerifyTls");
        req.options.minSegment = static_cast<int64_t>(m_settings->i("httpMinSegmentMB")) << 20;
        req.options.stallSeconds = m_settings->i("httpStallSeconds");
        req.options.maxRetries = m_settings->i("httpRetries");
        m_http->add(req);
    }

    platform::registerNativeHost();
    setupTray();
    applySettings();
    m_timer.start(500);
    QTimer::singleShot(50, this, &Controller::tick);
    return true;
}

void Controller::shutdown() {
    if (!m_bt && !m_http) return;
    m_timer.stop();
    persistHttp(true);
    if (m_bt) {
        const std::vector<char> st = m_bt->shutdown(5000);
        if (!st.empty()) m_store.setBlob("session_state", QByteArray(st.data(), static_cast<qsizetype>(st.size())));
        QMutexLocker lk(&m_resumeMutex);
        m_store.begin();
        for (auto it = m_resumePending.cbegin(); it != m_resumePending.cend(); ++it)
            if (m_btRec.contains(it.key())) m_store.saveResume(it.key(), it.value());
        m_store.commit();
        m_resumePending.clear();
    }
    m_store.addStats(m_statDown, m_statUp);
    m_bt.reset();
    m_http.reset();
    platform::preventSleep(false);
    if (m_tray) m_tray->hide();
    m_store.close();
}

// ================================================================ settings → engines

void Controller::applySettings(const QString &key) {
    static const QSet<QString> btKeys = {"listenPort", "upnp", "natpmp", "maxConnections", "maxConnectionsPerTorrent",
                                         "maxUploads", "proxyType", "proxyHost", "proxyPort", "networkInterface", "dht",
                                         "pex", "lsd", "encryption", "anonymous", "queueing", "activeDownloads",
                                         "activeSeeds", "activeLimit", "upLimit", "altUpLimit"};
    static const QSet<QString> httpKeys = {"httpMaxActive", "httpConnections", "httpUserAgent", "httpVerifyTls"};
    if (key.isEmpty() || btKeys.contains(key) || key == QLatin1String("altSpeed")) applyBtSettings();
    if (key.isEmpty() || httpKeys.contains(key)) applyHttpSettings();
    if (key == QLatin1String("altSpeed") || key == QLatin1String("downLimit") || key == QLatin1String("altDownLimit")) {
        m_btLimitApplied = m_httpLimitApplied = -1;
        arbitrate();
        if (m_altAction) m_altAction->setChecked(m_settings->b("altSpeed"));
    }
    if (key.isEmpty() || key == QLatin1String("launchAtLogin")) platform::setLaunchAtLogin(m_settings->b("launchAtLogin"));
    if (key == QLatin1String("savePath")) m_lastFree = 0;
}

void Controller::applyBtSettings() {
    if (!m_bt) return;
    bt::Settings s;
    s.listenPort = m_settings->i("listenPort");
    s.upnp = m_settings->b("upnp");
    s.natpmp = m_settings->b("natpmp");
    s.maxConnections = m_settings->i("maxConnections");
    s.maxConnectionsPerTorrent = m_settings->i("maxConnectionsPerTorrent");
    s.maxUploads = m_settings->i("maxUploads");
    s.proxyType = m_settings->i("proxyType");
    s.proxyHost = u(m_settings->s("proxyHost"));
    s.proxyPort = m_settings->i("proxyPort");
    s.networkInterface = u(m_settings->s("networkInterface"));
    s.dht = m_settings->b("dht");
    s.pex = m_settings->b("pex");
    s.lsd = m_settings->b("lsd");
    s.encryption = m_settings->i("encryption");
    s.anonymous = m_settings->b("anonymous");
    s.queueing = m_settings->b("queueing");
    s.activeDownloads = m_settings->i("activeDownloads");
    s.activeSeeds = m_settings->i("activeSeeds");
    s.activeLimit = m_settings->i("activeLimit");
    const bool alt = m_settings->b("altSpeed");
    s.uploadLimit = (alt ? m_settings->i("altUpLimit") : m_settings->i("upLimit")) * 1000;
    s.downloadLimit = m_btLimitApplied > 0 ? static_cast<int>(m_btLimitApplied) : 0;
    s.version = RIFT_VERSION;
    m_bt->apply(s);
}

void Controller::applyHttpSettings() {
    if (!m_http) return;
    m_http->setMaxActive(m_settings->i("httpMaxActive"));
}

// Unified bandwidth arbiter: one download limit shared by both engines by measured demand.
void Controller::arbitrate() {
    if (!m_bt || !m_http) return;
    const bool alt = m_settings->b("altSpeed");
    const int64_t limit = static_cast<int64_t>(alt ? m_settings->i("altDownLimit") : m_settings->i("downLimit")) * 1000;
    const auto [btL, httpL] = core::splitLimit(limit, m_btDown, m_httpDown);
    auto differs = [](int64_t a, int64_t b) { return a < 0 || (a == 0) != (b == 0) || std::llabs(a - b) > std::max<int64_t>(b / 20, 2048); };
    if (differs(m_httpLimitApplied, httpL)) {
        m_http->setGlobalLimit(httpL);
        m_httpLimitApplied = httpL;
    }
    if (differs(m_btLimitApplied, btL)) {
        m_btLimitApplied = btL;
        applyBtSettings();
    }
}

// ================================================================ tick

void Controller::tick() {
    if (!m_bt || !m_http) return;
    const qint64 now = nowMs();
    ++m_tickCount;
    QVector<Row> rows;

    // ---- Torrents
    const auto bts = m_bt->snapshot();
    double btDown = 0, btUp = 0;
    for (const bt::TorrentStatus &t : bts) {
        const QString hash = q(t.id);
        if (!m_btRec.contains(hash)) {
            TorrentRecord r;
            r.id = hash;
            r.name = q(t.name);
            r.addedAt = t.addedTime > 0 ? t.addedTime * 1000 : now;
            r.source = m_btPending.value(hash);
            m_btRec.insert(hash, r);
            m_store.saveTorrent(r);
        }
        TorrentRecord &rec = m_btRec[hash];
        Row r;
        r.id = QStringLiteral("t:") + hash;
        r.kind = QStringLiteral("bt");
        r.name = t.hasMetadata ? q(t.name) : (rec.name.isEmpty() ? q(t.name) : rec.name);
        if (t.hasMetadata && rec.name != r.name) { rec.name = r.name; m_store.saveTorrent(rec); }
        r.state = q(t.state);
        r.category = rec.category;
        r.tags = rec.tags;
        r.source = rec.source;
        r.site = t.currentTracker.empty() ? (t.isPrivate ? QString() : QStringLiteral("DHT")) : q(core::hostFromUrl(t.currentTracker));
        r.path = QDir(q(t.savePath)).filePath(q(t.name));
        r.error = q(t.error);
        r.size = t.size;
        r.done = t.done;
        r.added = rec.addedAt;
        r.completed = t.completedTime > 0 ? t.completedTime * 1000 : rec.completedAt;
        r.progress = t.progress;
        r.ratio = t.ratio;
        r.availability = t.availability;
        r.down = t.down;
        r.up = t.up;
        r.eta = (t.state == "downloading" || t.state == "stalled") ? core::eta(t.size - t.done, t.down) : (t.progress >= 1 ? 0 : -1);
        r.seeds = t.seeds;
        r.peers = t.peers;
        r.seedsTotal = t.seedsTotal;
        r.peersTotal = t.peersTotal;
        r.queue = t.queuePos;
        r.isPrivate = t.isPrivate;
        r.sequential = t.sequential;
        btDown += t.down;
        btUp += t.up;
        // Seeding limit
        const double lim = m_settings->d("seedRatioLimit");
        if (lim > 0 && t.state == "seeding" && t.ratio >= lim) {
            if (m_settings->s("seedRatioAction") == QLatin1String("remove")) m_bt->remove(t.id, false);
            else m_bt->pause(t.id);
        }
        rows.push_back(std::move(r));
    }

    // ---- HTTP
    const auto hs = m_http->snapshot();
    double httpDown = 0;
    QSet<qint64> live;
    for (const http::Status &s : hs) {
        const qint64 id = static_cast<qint64>(s.id);
        live.insert(id);
        HttpRecord &rec = m_httpRec[id];
        Row r;
        r.id = QStringLiteral("h:%1").arg(id);
        r.kind = QStringLiteral("http");
        r.name = !s.fileName.empty() ? q(s.fileName) : (!rec.fileName.isEmpty() ? rec.fileName : q(core::fileNameFromUrl(s.url)));
        if (r.name.isEmpty()) r.name = q(s.url);
        r.state = httpStateName(s.state, s.speed);
        if (s.state == http::State::Paused && !rec.error.isEmpty() && rec.state == QLatin1String("error")) r.state = QStringLiteral("error");
        r.category = rec.category;
        r.tags = rec.tags;
        r.site = q(core::hostFromUrl(s.finalUrl.empty() ? s.url : s.finalUrl));
        r.path = QDir(q(s.dir)).filePath(r.name);
        r.error = q(s.error).isEmpty() ? rec.error : q(s.error);
        r.source = rec.referer.isEmpty() ? rec.url : rec.referer;
        r.size = s.size;
        r.done = s.done;
        r.added = rec.addedAt;
        r.completed = rec.completedAt;
        r.progress = s.size > 0 ? static_cast<double>(s.done) / static_cast<double>(s.size) : (s.state == http::State::Completed ? 1.0 : 0.0);
        r.down = s.speed;
        r.eta = s.state == http::State::Downloading ? (s.size > 0 ? core::eta(s.size - s.done, s.speed) : -1) : (s.state == http::State::Completed ? 0 : -1);
        r.conns = static_cast<int>(s.connections.size());
        r.resumable = s.resumable;
        if (s.size > 0) {
            for (const http::Connection &c : s.connections) {
                const double sz = static_cast<double>(s.size);
                r.segs.push_back(QVariantMap{{"s", c.start / sz}, {"e", c.end / sz}, {"c", c.cursor / sz}, {"v", c.speed}});
            }
        }
        httpDown += s.speed;
        rows.push_back(std::move(r));
    }
    // Finished HTTP downloads (not in the engine anymore)
    for (auto it = m_httpRec.cbegin(); it != m_httpRec.cend(); ++it) {
        if (live.contains(it.key())) continue;
        const HttpRecord &rec = it.value();
        if (rec.state != QLatin1String("completed")) continue;
        Row r;
        r.id = QStringLiteral("h:%1").arg(rec.id);
        r.kind = QStringLiteral("http");
        r.name = rec.fileName.isEmpty() ? q(core::fileNameFromUrl(u(rec.url))) : rec.fileName;
        r.state = QStringLiteral("completed");
        r.category = rec.category;
        r.tags = rec.tags;
        r.site = q(core::hostFromUrl(u(rec.finalUrl.isEmpty() ? rec.url : rec.finalUrl)));
        r.path = QDir(rec.dir).filePath(r.name);
        r.source = rec.referer.isEmpty() ? rec.url : rec.referer;
        r.size = rec.size;
        r.done = rec.size;
        r.progress = 1.0;
        r.added = rec.addedAt;
        r.completed = rec.completedAt;
        r.eta = 0;
        rows.push_back(std::move(r));
    }

    m_model.update(std::move(rows));
    m_btDown = btDown;
    m_httpDown = httpDown;
    m_down = btDown + httpDown;
    m_up = btUp;
    m_statDown += static_cast<qint64>(m_down * 0.5);
    m_statUp += static_cast<qint64>(m_up * 0.5);

    int active = 0, downloading = 0;
    qint64 wanted = 0, have = 0;
    for (const Row &r : m_model.rows()) {
        if (r.down > 0 || r.up > 0) ++active;
        const bool dl = r.state == QLatin1String("downloading") || r.state == QLatin1String("stalled") ||
                        r.state == QLatin1String("probing") || r.state == QLatin1String("metadata");
        if (dl) { ++downloading; wanted += std::max<qint64>(r.size, 0); have += r.done; }
    }
    m_active = active;
    m_downloading = downloading;
    m_overall = wanted > 0 ? static_cast<double>(have) / static_cast<double>(wanted) : 0;

    // Session info
    const bt::SessionInfo si = m_bt->session();
    m_dht = si.dhtNodes;
    m_port = si.listenPort;
    m_netState = !si.listening ? QStringLiteral("offline")
                 : (si.portMapped == 1 || m_dht > 50) ? QStringLiteral("ok")
                                                       : QStringLiteral("firewalled");
    m_sessionDown += static_cast<qint64>(m_down * 0.5);
    m_sessionUp += static_cast<qint64>(m_up * 0.5);

    m_history.append(QVariantMap{{"d", m_down}, {"u", m_up}});
    while (m_history.size() > 240) m_history.removeFirst();

    handleBtEvents();
    handleHttpTransitions();

    // Resume blobs from the alert thread → SQLite in one transaction.
    {
        QMutexLocker lk(&m_resumeMutex);
        if (!m_resumePending.isEmpty()) {
            m_store.begin();
            for (auto it = m_resumePending.cbegin(); it != m_resumePending.cend(); ++it)
                if (m_btRec.contains(it.key())) m_store.saveResume(it.key(), it.value());
            m_store.commit();
            m_resumePending.clear();
        }
    }
    if (now - m_lastResumeSave > 5 * 60 * 1000) { m_lastResumeSave = now; m_bt->saveResumeData(); }
    if (now - m_lastPersist > 2000) persistHttp(false);
    if (now - m_lastStats > 60 * 1000) {
        m_lastStats = now;
        m_store.addStats(m_statDown, m_statUp);
        m_statDown = m_statUp = 0;
    }
    if (now - m_lastFree > 10 * 1000) { m_lastFree = now; m_free = platform::freeSpace(m_settings->s("savePath")); }

    arbitrate();
    platform::preventSleep(m_settings->b("preventSleep") && downloading > 0);
    if (m_tickCount % 2 == 0) rebuildCounts();
    refreshDetail();
    updateTray();
    emit ticked();
}

void Controller::rebuildCounts() {
    static const QStringList keys = {"all", "downloading", "seeding", "completed", "paused", "active",
                                     "inactive", "stalled", "checking", "error", "k:bt", "k:http"};
    QVariantMap counts;
    for (const QString &k : keys) {
        int n = 0;
        for (const Row &r : m_model.rows())
            if (DownloadFilter::matches(r, k)) ++n;
        counts.insert(k, n);
    }
    QMap<QString, int> cats, tags, sites;
    for (const QVariant &c : m_store.categories()) cats.insert(c.toMap().value("name").toString(), 0);
    for (const Row &r : m_model.rows()) {
        if (!r.category.isEmpty()) cats[r.category]++;
        for (const QString &t : r.tags) tags[t]++;
        if (!r.site.isEmpty()) sites[r.site]++;
    }
    QVariantList cl, tl, sl;
    for (auto it = cats.cbegin(); it != cats.cend(); ++it) cl.push_back(QVariantMap{{"name", it.key()}, {"count", it.value()}});
    for (auto it = tags.cbegin(); it != tags.cend(); ++it) tl.push_back(QVariantMap{{"name", it.key()}, {"count", it.value()}});
    QVector<QPair<int, QString>> sv;
    for (auto it = sites.cbegin(); it != sites.cend(); ++it) sv.push_back({it.value(), it.key()});
    std::sort(sv.begin(), sv.end(), [](const auto &a, const auto &b) { return a.first > b.first || (a.first == b.first && a.second < b.second); });
    for (int i = 0; i < sv.size() && i < 8; ++i) sl.push_back(QVariantMap{{"name", sv[i].second}, {"count", sv[i].first}});
    if (counts != m_counts || cl != m_categories || tl != m_tags || sl != m_sites) {
        m_counts = counts;
        m_categories = cl;
        m_tags = tl;
        m_sites = sl;
        emit countsChanged();
    }
}

void Controller::handleBtEvents() {
    for (const bt::Event &e : m_bt->takeEvents()) {
        const QString hash = q(e.id);
        switch (e.type) {
        case bt::Event::Added:
            m_btPending.remove(hash);
            break;
        case bt::Event::AddFailed:
            m_btPending.remove(hash);
            emit toast(tr("Could not add torrent: %1").arg(q(e.message)), QStringLiteral("danger"), {});
            break;
        case bt::Event::Metadata:
            break;
        case bt::Event::Finished: {
            if (!m_btRec.contains(hash)) break;
            TorrentRecord &rec = m_btRec[hash];
            if (rec.completedAt > 0) break; // already counted (e.g. a recheck)
            rec.completedAt = nowMs();
            m_store.saveTorrent(rec);
            const Row *r = row(QStringLiteral("t:") + hash);
            m_store.addHistory("bt", rec.name, r ? r->size : 0, rec.source, r ? r->path : QString());
            if (m_settings->b("notifications")) notify(tr("Download complete"), rec.name);
            break;
        }
        case bt::Event::Error: {
            const QString name = m_btRec.value(hash).name;
            if (m_settings->b("notifyErrors")) notify(tr("Download error"), name + QStringLiteral(": ") + q(e.message));
            break;
        }
        case bt::Event::Removed:
            break;
        case bt::Event::Moved:
            emit toast(tr("Moved to %1").arg(q(e.message)), QStringLiteral("ok"), {});
            break;
        case bt::Event::MoveFailed:
            emit toast(tr("Move failed: %1").arg(q(e.message)), QStringLiteral("danger"), {});
            break;
        }
    }
}

void Controller::handleHttpTransitions() {
    for (const http::Status &s : m_http->snapshot()) {
        const qint64 id = static_cast<qint64>(s.id);
        const QString st = httpStateName(s.state, 1);
        const QString prev = m_httpPrevState.value(id);
        if (st == prev) continue;
        m_httpPrevState.insert(id, st);
        HttpRecord &rec = m_httpRec[id];
        if (s.state == http::State::Completed) {
            rec.state = QStringLiteral("completed");
            rec.completedAt = nowMs();
            rec.fileName = q(s.fileName);
            rec.size = s.size;
            rec.error.clear();
            persistHttp(true);
            m_store.addHistory("http", rec.fileName, s.size, rec.referer.isEmpty() ? rec.url : rec.referer, QDir(rec.dir).filePath(rec.fileName));
            if (m_settings->b("notifications")) notify(tr("Download complete"), rec.fileName);
            if (!rec.checksum.isEmpty()) verifyChecksum(QStringLiteral("h:%1").arg(id), rec.checksum);
            // Drop it from the engine; the finished row is rendered from the record.
            m_http->remove(s.id, false);
        } else if (s.state == http::State::Error) {
            rec.state = QStringLiteral("error");
            rec.error = q(s.error);
            persistHttp(true);
            if (m_settings->b("notifyErrors")) notify(tr("Download failed"), rec.fileName + QStringLiteral(": ") + rec.error);
        }
    }
}

void Controller::persistHttp(bool force) {
    if (!m_http) return;
    m_lastPersist = nowMs();
    const auto hs = m_http->snapshot();
    m_store.begin();
    for (const http::Status &s : hs) {
        const qint64 id = static_cast<qint64>(s.id);
        if (!m_httpRec.contains(id)) continue;
        HttpRecord &rec = m_httpRec[id];
        QString state = s.state == http::State::Completed ? QStringLiteral("completed")
                        : s.state == http::State::Paused  ? (rec.state == QLatin1String("error") ? QStringLiteral("error") : QStringLiteral("paused"))
                        : s.state == http::State::Error   ? QStringLiteral("error")
                                                          : QStringLiteral("queued");
        const QString ranges = q(s.doneRanges);
        if (!force && ranges == rec.doneRanges && state == rec.state && rec.size == s.size) continue;
        rec.doneRanges = ranges;
        rec.state = state;
        rec.size = s.size;
        if (!s.fileName.empty()) rec.fileName = q(s.fileName);
        rec.finalUrl = q(s.finalUrl);
        rec.etag = q(s.etag);
        rec.lastModified = q(s.lastModified);
        rec.resumable = s.resumable;
        if (!s.error.empty()) rec.error = q(s.error);
        m_store.saveHttpProgress(id, rec.doneRanges, rec.size, rec.state, rec.fileName, rec.finalUrl, rec.etag,
                                 rec.lastModified, rec.resumable, rec.error, rec.completedAt);
    }
    m_store.commit();
}

// ================================================================ details

void Controller::setDetailTab(const QString &t) {
    if (t == m_detailTab) return;
    m_detailTab = t;
    emit detailTabChanged();
    refreshDetail();
}

void Controller::refreshDetail() {
    const Row *r = m_current.isEmpty() ? nullptr : row(m_current);
    if (!r) {
        if (!m_detail.isEmpty()) {
            m_detail.clear(); m_files.clear(); m_peers.clear(); m_trackers.clear(); m_pieces.clear();
            emit detailChanged();
        }
        return;
    }
    QVariantMap d{{"id", r->id}, {"kind", r->kind}, {"name", r->name}, {"state", r->state}, {"size", r->size},
                  {"done", r->done}, {"progress", r->progress}, {"down", r->down}, {"up", r->up}, {"eta", r->eta},
                  {"ratio", r->ratio}, {"seeds", r->seeds}, {"peers", r->peers}, {"seedsTotal", r->seedsTotal},
                  {"peersTotal", r->peersTotal}, {"conns", r->conns}, {"category", r->category}, {"tags", r->tags},
                  {"site", r->site}, {"path", r->path}, {"error", r->error}, {"added", r->added},
                  {"completed", r->completed}, {"availability", r->availability}, {"isPrivate", r->isPrivate},
                  {"sequential", r->sequential}, {"segs", r->segs}, {"source", r->source}, {"resumable", r->resumable},
                  {"queuePos", r->queue}};
    m_files.clear(); m_peers.clear(); m_trackers.clear(); m_pieces.clear();
    if (r->kind == QLatin1String("bt")) {
        const std::string hash = u(r->id.mid(2));
        for (const bt::TorrentStatus &t : m_bt->snapshot()) {
            if (t.id != hash) continue;
            d.insert("hashV1", q(t.hashV1)); d.insert("hashV2", q(t.hashV2)); d.insert("comment", q(t.comment));
            d.insert("creator", q(t.creator)); d.insert("downloaded", static_cast<qint64>(t.downloaded));
            d.insert("uploaded", static_cast<qint64>(t.uploaded)); d.insert("numFiles", t.numFiles);
            d.insert("numPieces", t.numPieces); d.insert("pieceLength", t.pieceLength);
            d.insert("savePath", q(t.savePath)); d.insert("tracker", q(t.currentTracker));
            break;
        }
        if (m_detailTab == QLatin1String("files") || m_detailTab == QLatin1String("overview")) {
            for (const bt::FileEntry &f : m_bt->files(hash))
                m_files.push_back(QVariantMap{{"index", f.index}, {"path", q(f.path)}, {"size", static_cast<qint64>(f.size)},
                                              {"done", static_cast<qint64>(f.done)}, {"priority", f.priority},
                                              {"progress", f.size > 0 ? static_cast<double>(f.done) / static_cast<double>(f.size) : 1.0}});
        }
        if (m_detailTab == QLatin1String("peers")) {
            for (const bt::Peer &p : m_bt->peers(hash))
                m_peers.push_back(QVariantMap{{"ip", q(p.ip)}, {"port", p.port}, {"client", q(p.client)}, {"flags", q(p.flags)},
                                              {"progress", p.progress}, {"down", p.down}, {"up", p.up}, {"utp", p.utp},
                                              {"encrypted", p.encrypted}, {"seed", p.seed}});
        }
        if (m_detailTab == QLatin1String("trackers")) {
            for (const bt::Tracker &t : m_bt->trackers(hash))
                m_trackers.push_back(QVariantMap{{"url", q(t.url)}, {"status", q(t.status)}, {"message", q(t.message)},
                                                 {"tier", t.tier}, {"seeds", t.seeds}, {"peers", t.peers}, {"downloaded", t.downloaded}});
        }
        if (m_detailTab == QLatin1String("overview")) {
            for (uint8_t v : m_bt->pieceMap(hash, 120)) m_pieces.push_back(static_cast<int>(v));
            d.insert("magnet", q(m_bt->magnet(hash)));
        }
    } else {
        const qint64 id = r->id.mid(2).toLongLong();
        const HttpRecord rec = m_httpRec.value(id);
        d.insert("url", rec.url);
        d.insert("referer", rec.referer);
        d.insert("checksum", rec.checksum);
        d.insert("savePath", rec.dir);
        for (const http::Status &s : m_http->snapshot()) {
            if (static_cast<qint64>(s.id) != id) continue;
            d.insert("finalUrl", q(s.finalUrl));
            d.insert("speedNow", s.speedNow);
            d.insert("firstConnSpeed", s.firstConnSpeed);
            d.insert("retries", s.retries);
            d.insert("httpCode", s.httpCode);
            d.insert("etag", q(s.etag));
            QVariantList conns;
            for (const http::Connection &c : s.connections)
                conns.push_back(QVariantMap{{"start", static_cast<qint64>(c.start)}, {"cursor", static_cast<qint64>(c.cursor)},
                                            {"end", static_cast<qint64>(c.end)}, {"speed", c.speed}});
            d.insert("connections", conns);
            // Done map in 120 buckets for the piece strip.
            if (s.size > 0) {
                const core::RangeSet done = core::RangeSet::parse(s.doneRanges);
                for (int b = 0; b < 120; ++b) {
                    const int64_t a = s.size * b / 120, e = s.size * (b + 1) / 120;
                    int64_t have = 0;
                    for (const core::Range &x : done.ranges()) {
                        const int64_t lo = std::max(a, x.start), hi = std::min(e, x.end);
                        if (hi > lo) have += hi - lo;
                    }
                    m_pieces.push_back(e > a ? static_cast<int>(have * 255 / (e - a)) : 0);
                }
            }
            break;
        }
        if (rec.state == QLatin1String("completed")) for (int b = 0; b < 120; ++b) m_pieces.push_back(255);
    }
    m_detail = d;
    emit detailChanged();
}

void Controller::select(const QStringList &ids, const QString &current) {
    m_selection = ids;
    m_current = current.isEmpty() && ids.size() == 1 ? ids.first() : current;
    emit selectionChanged();
    refreshDetail();
}

// ================================================================ adding

QString Controller::savePathFor(const QVariantMap &opts, const QString &category) const {
    QString p = opts.value("savePath").toString();
    if (p.isEmpty()) p = defaultFolderFor(category);
    return localPath(p);
}

QString Controller::defaultFolderFor(const QString &category) const {
    if (!category.isEmpty()) {
        for (const QVariant &c : const_cast<Storage &>(m_store).categories()) {
            const QVariantMap m = c.toMap();
            if (m.value("name").toString() == category && !m.value("savePath").toString().isEmpty()) return m.value("savePath").toString();
        }
        if (m_settings->b("categoryFolders")) return QDir(m_settings->s("savePath")).filePath(category);
    }
    return m_settings->s("savePath");
}

QVariantMap Controller::classify(const QString &text) {
    int magnets = 0, urls = 0, torrents = 0, invalid = 0;
    QStringList names;
    for (QString line : text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts)) {
        line = line.trimmed();
        if (line.isEmpty()) continue;
        const std::string s = u(line);
        if (core::isMagnet(s)) {
            ++magnets;
            const QString dn = q(core::magnetDisplayName(s));
            names << (dn.isEmpty() ? QStringLiteral("magnet: ") + q(core::magnetInfoHash(s)).left(16) + QStringLiteral("…") : dn);
        } else if (core::isHttpUrl(s)) {
            for (const std::string &x : core::expandBatchPattern(s, 2000)) {
                const QString name = q(core::fileNameFromUrl(x));
                if (name.endsWith(QLatin1String(".torrent"), Qt::CaseInsensitive)) ++torrents; else ++urls;
                if (names.size() < 200) names << (name.isEmpty() ? q(core::hostFromUrl(x)) : name);
            }
        } else if (QFileInfo(localPath(line)).isFile() && line.endsWith(QLatin1String(".torrent"), Qt::CaseInsensitive)) {
            ++torrents;
            names << QFileInfo(localPath(line)).fileName();
        } else {
            ++invalid;
        }
    }
    QString category;
    if (urls > 0 && magnets == 0 && torrents == 0 && !names.isEmpty()) category = q(core::categoryForFileName(u(names.first())));
    return {{"magnets", magnets}, {"urls", urls}, {"torrents", torrents}, {"invalid", invalid}, {"names", names},
            {"total", magnets + urls + torrents}, {"category", category == QLatin1String("Other") ? QString() : category}};
}

int Controller::addLinks(const QString &text, const QVariantMap &opts) {
    int added = 0;
    for (QString line : text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts)) {
        line = line.trimmed();
        if (line.isEmpty()) continue;
        const std::string s = u(line);
        if (core::isMagnet(s)) {
            if (addMagnet(line, opts)) ++added;
        } else if (core::isHttpUrl(s)) {
            for (const std::string &x : core::expandBatchPattern(s, 2000)) {
                const QString url = q(x);
                if (q(core::fileNameFromUrl(x)).endsWith(QLatin1String(".torrent"), Qt::CaseInsensitive)) fetchTorrentUrl(url, opts);
                else if (addHttp(url, opts) > 0) ++added;
            }
        } else if (QFileInfo(localPath(line)).isFile()) {
            if (addTorrentFile(line, opts)) ++added;
        }
    }
    if (added > 0) {
        if (!opts.value("category").toString().isEmpty()) m_settings->set("lastCategory", opts.value("category").toString());
        emit toast(added == 1 ? tr("Added 1 download") : tr("Added %1 downloads").arg(added), QStringLiteral("ok"), {});
    }
    return added;
}

qint64 Controller::addHttp(const QString &url, const QVariantMap &opts) {
    HttpRecord rec;
    rec.id = std::max<qint64>(m_store.nextHttpId(), m_httpRec.isEmpty() ? 1 : *std::max_element(m_httpRec.keyBegin(), m_httpRec.keyEnd()) + 1);
    rec.url = url;
    rec.fileName = opts.value("fileName").toString();
    QString cat = opts.value("category").toString();
    if (cat.isEmpty()) {
        const QString guess = q(core::categoryForFileName(u(rec.fileName.isEmpty() ? q(core::fileNameFromUrl(u(url))) : rec.fileName)));
        if (guess != QLatin1String("Other")) cat = guess;
    }
    rec.category = cat;
    rec.dir = savePathFor(opts, cat);
    rec.referer = opts.value("referer").toString();
    rec.checksum = opts.value("checksum").toString().trimmed();
    rec.connections = opts.value("connections").toInt();
    rec.addedAt = nowMs();
    const bool paused = opts.contains("paused") ? opts.value("paused").toBool() : m_settings->b("startPaused");
    rec.state = paused ? QStringLiteral("paused") : QStringLiteral("queued");
    m_httpRec.insert(rec.id, rec);
    m_httpPrevState.insert(rec.id, rec.state);
    m_store.saveHttp(rec);

    http::AddRequest req;
    req.id = static_cast<uint64_t>(rec.id);
    req.url = u(url);
    req.dir = u(rec.dir);
    req.fileName = u(rec.fileName);
    req.startPaused = paused;
    req.options.maxConnections = rec.connections > 0 ? rec.connections : m_settings->i("httpConnections");
    req.options.minSegment = static_cast<int64_t>(m_settings->i("httpMinSegmentMB")) << 20;
    req.options.stallSeconds = m_settings->i("httpStallSeconds");
    req.options.maxRetries = m_settings->i("httpRetries");
    req.options.verifyTls = m_settings->b("httpVerifyTls");
    req.options.userAgent = u(opts.value("userAgent").toString().isEmpty() ? m_settings->s("httpUserAgent") : opts.value("userAgent").toString());
    req.options.referer = u(rec.referer);
    req.options.cookie = u(opts.value("cookies").toString()); // memory only, never stored
    m_http->add(req);
    return rec.id;
}

bool Controller::addMagnet(const QString &magnet, const QVariantMap &opts) {
    bt::Engine::AddOptions o;
    const QString cat = opts.value("category").toString();
    o.savePath = u(savePathFor(opts, cat));
    o.paused = opts.contains("paused") ? opts.value("paused").toBool() : m_settings->b("startPaused");
    o.sequential = opts.contains("sequential") ? opts.value("sequential").toBool() : m_settings->b("sequentialDefault");
    std::string err;
    const std::string id = m_bt->addMagnet(u(magnet), o, err);
    if (!err.empty()) {
        emit toast(q(err), id.empty() ? QStringLiteral("danger") : QStringLiteral("info"), {});
        return false;
    }
    TorrentRecord r;
    r.id = q(id);
    r.name = q(core::magnetDisplayName(u(magnet)));
    r.category = cat;
    r.source = magnet;
    r.addedAt = nowMs();
    m_btRec.insert(r.id, r);
    m_btPending.insert(r.id, magnet);
    m_store.saveTorrent(r);
    return true;
}

bool Controller::addTorrentData(const QByteArray &data, const QVariantMap &opts, const QString &source) {
    bt::Engine::AddOptions o;
    const QString cat = opts.value("category").toString();
    o.savePath = u(savePathFor(opts, cat));
    o.paused = opts.contains("paused") ? opts.value("paused").toBool() : m_settings->b("startPaused");
    o.sequential = opts.contains("sequential") ? opts.value("sequential").toBool() : m_settings->b("sequentialDefault");
    for (const QVariant &v : opts.value("priorities").toList()) o.filePriorities.push_back(v.toInt());
    std::string err;
    const std::string id = m_bt->addTorrent(std::vector<char>(data.begin(), data.end()), o, err);
    if (!err.empty()) {
        emit toast(q(err), id.empty() ? QStringLiteral("danger") : QStringLiteral("info"), {});
        return false;
    }
    bt::Preview pv;
    std::string perr;
    bt::Engine::inspect(std::vector<char>(data.begin(), data.end()), pv, perr);
    TorrentRecord r;
    r.id = q(id);
    r.name = q(pv.name);
    r.category = cat;
    r.source = source;
    r.addedAt = nowMs();
    m_btRec.insert(r.id, r);
    m_btPending.insert(r.id, source);
    m_store.saveTorrent(r);
    return true;
}

QVariantMap Controller::inspectTorrent(const QString &pathOrUrl) {
    QFile f(localPath(pathOrUrl));
    if (!f.open(QIODevice::ReadOnly)) return {{"ok", false}, {"error", tr("Could not open the file")}};
    if (f.size() > 64 * 1024 * 1024) return {{"ok", false}, {"error", tr("The file is too large to be a .torrent")}};
    const QByteArray data = f.readAll();
    bt::Preview pv;
    std::string err;
    if (!bt::Engine::inspect(std::vector<char>(data.begin(), data.end()), pv, err)) return {{"ok", false}, {"error", q(err)}};
    QVariantList files;
    for (const bt::FileEntry &fe : pv.files)
        files.push_back(QVariantMap{{"index", fe.index}, {"path", q(fe.path)}, {"size", static_cast<qint64>(fe.size)}});
    return {{"ok", true}, {"name", q(pv.name)}, {"hash", q(pv.hash)}, {"size", static_cast<qint64>(pv.size)},
            {"comment", q(pv.comment)}, {"creator", q(pv.creator)}, {"isPrivate", pv.isPrivate}, {"files", files},
            {"exists", m_btRec.contains(q(pv.hash)) && m_bt->has(pv.hash)}};
}

bool Controller::addTorrentFile(const QString &pathOrUrl, const QVariantMap &opts) {
    const QString path = localPath(pathOrUrl);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly) || f.size() > 64 * 1024 * 1024) {
        emit toast(tr("Could not open %1").arg(QFileInfo(path).fileName()), QStringLiteral("danger"), {});
        return false;
    }
    const bool ok = addTorrentData(f.readAll(), opts, QFileInfo(path).fileName());
    f.close();
    if (ok && m_settings->b("deleteTorrentAfterAdd")) QFile::remove(path);
    return ok;
}

void Controller::fetchTorrentUrl(const QString &url, const QVariantMap &opts) {
    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Rift/") + version());
    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, opts] {
        reply->deleteLater();
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError || data.isEmpty() || data.size() > 64 * 1024 * 1024) {
            emit toast(tr("Could not download the .torrent: %1").arg(reply->errorString()), QStringLiteral("danger"), {});
            return;
        }
        if (addTorrentData(data, opts, url)) emit toast(tr("Added torrent from link"), QStringLiteral("ok"), {});
    });
}

// ================================================================ actions

QStringList Controller::btIds(const QStringList &ids) const {
    QStringList out;
    for (const QString &id : ids)
        if (id.startsWith(QLatin1String("t:"))) out << id.mid(2);
    return out;
}

QList<qint64> Controller::httpIds(const QStringList &ids) const {
    QList<qint64> out;
    for (const QString &id : ids)
        if (id.startsWith(QLatin1String("h:"))) out << id.mid(2).toLongLong();
    return out;
}

void Controller::pause(const QStringList &ids) {
    for (const QString &h : btIds(ids)) m_bt->pause(u(h));
    for (qint64 id : httpIds(ids)) {
        if (m_httpRec.value(id).state == QLatin1String("completed")) continue;
        m_http->pause(static_cast<uint64_t>(id));
        m_httpRec[id].state = QStringLiteral("paused");
        m_httpPrevState.insert(id, QStringLiteral("paused"));
    }
    persistHttp(true);
}

void Controller::resume(const QStringList &ids) {
    for (const QString &h : btIds(ids)) m_bt->resume(u(h));
    for (qint64 id : httpIds(ids)) {
        if (m_httpRec.value(id).state == QLatin1String("completed")) continue;
        m_http->resume(static_cast<uint64_t>(id));
        m_httpRec[id].state = QStringLiteral("queued");
        m_httpRec[id].error.clear();
    }
    persistHttp(true);
}

void Controller::togglePause(const QStringList &ids) {
    QStringList toPause, toResume;
    for (const QString &id : ids) {
        const Row *r = row(id);
        if (!r) continue;
        const bool paused = r->state == QLatin1String("paused") || r->state == QLatin1String("error") ||
                            (r->state == QLatin1String("queued") && r->kind == QLatin1String("bt"));
        (paused ? toResume : toPause) << id;
    }
    if (!toPause.isEmpty()) pause(toPause);
    if (!toResume.isEmpty()) resume(toResume);
}

void Controller::forceStart(const QStringList &ids) {
    for (const QString &h : btIds(ids)) m_bt->forceStart(u(h));
    for (qint64 id : httpIds(ids)) m_http->resume(static_cast<uint64_t>(id));
}

void Controller::remove(const QStringList &ids, bool deleteFiles) {
    m_undoHttp.clear();
    m_undoBt.clear();
    const auto resume = m_store.loadResume();
    for (const QString &h : btIds(ids)) {
        if (!deleteFiles) {
            for (const auto &[id, blob] : resume)
                if (id == h) m_undoBt.push_back({m_btRec.value(h), blob});
        }
        m_bt->remove(u(h), deleteFiles);
        m_btRec.remove(h);
        m_store.removeTorrent(h);
    }
    for (qint64 id : httpIds(ids)) {
        if (!deleteFiles) m_undoHttp.push_back(m_httpRec.value(id));
        m_http->remove(static_cast<uint64_t>(id), deleteFiles);
        if (deleteFiles && m_httpRec.value(id).state == QLatin1String("completed")) {
            const HttpRecord &rec = m_httpRec[id];
            QFile::remove(QDir(rec.dir).filePath(rec.fileName));
        }
        m_httpRec.remove(id);
        m_httpPrevState.remove(id);
        m_store.removeHttp(id);
    }
    m_selection.clear();
    m_current.clear();
    emit selectionChanged();
    const int n = static_cast<int>(ids.size());
    if (!deleteFiles && (!m_undoBt.isEmpty() || !m_undoHttp.isEmpty()))
        emit toast(n == 1 ? tr("Removed 1 download. Files kept.") : tr("Removed %1 downloads. Files kept.").arg(n), QStringLiteral("info"), QStringLiteral("undo"));
    else
        emit toast(n == 1 ? tr("Removed 1 download and its files") : tr("Removed %1 downloads and their files").arg(n), QStringLiteral("info"), {});
    QTimer::singleShot(8500, this, [this] { m_undoBt.clear(); m_undoHttp.clear(); });
}

void Controller::undoRemove() {
    for (const auto &[rec, blob] : m_undoBt) {
        std::string err;
        m_bt->addResume(std::vector<char>(blob.begin(), blob.end()), err);
        m_btRec.insert(rec.id, rec);
        m_store.saveTorrent(rec);
        m_store.saveResume(rec.id, blob);
    }
    for (const HttpRecord &r : m_undoHttp) {
        QVariantMap opts{{"savePath", r.dir}, {"category", r.category}, {"fileName", r.fileName}, {"referer", r.referer},
                         {"paused", r.state != QLatin1String("completed")}};
        if (r.state == QLatin1String("completed")) {
            HttpRecord copy = r;
            m_httpRec.insert(copy.id, copy);
            m_store.saveHttp(copy);
        } else {
            addHttp(r.url, opts);
        }
    }
    m_undoBt.clear();
    m_undoHttp.clear();
    emit toast(tr("Restored"), QStringLiteral("ok"), {});
}

void Controller::recheck(const QStringList &ids) { for (const QString &h : btIds(ids)) m_bt->recheck(u(h)); }
void Controller::reannounce(const QStringList &ids) { for (const QString &h : btIds(ids)) m_bt->reannounce(u(h)); }

void Controller::restart(const QStringList &ids) {
    for (qint64 id : httpIds(ids)) {
        HttpRecord &rec = m_httpRec[id];
        if (rec.state == QLatin1String("completed")) {
            // Download it again as a new item.
            addHttp(rec.url, {{"savePath", rec.dir}, {"category", rec.category}, {"referer", rec.referer}});
            continue;
        }
        rec.doneRanges.clear();
        rec.error.clear();
        m_http->restart(static_cast<uint64_t>(id));
    }
}

void Controller::queueMove(const QStringList &ids, const QString &op) {
    const auto o = op == QLatin1String("up") ? bt::Engine::QueueOp::Up
                   : op == QLatin1String("down") ? bt::Engine::QueueOp::Down
                   : op == QLatin1String("top") ? bt::Engine::QueueOp::Top
                                                : bt::Engine::QueueOp::Bottom;
    for (const QString &h : btIds(ids)) m_bt->queue(u(h), o);
}

void Controller::setSequential(const QStringList &ids, bool on) { for (const QString &h : btIds(ids)) m_bt->setSequential(u(h), on); }
void Controller::setFirstLast(const QStringList &ids, bool on) { for (const QString &h : btIds(ids)) m_bt->setFirstLast(u(h), on); }

void Controller::setFilePriority(const QString &id, int index, int priority) {
    if (id.startsWith(QLatin1String("t:"))) m_bt->setFilePriority(u(id.mid(2)), index, priority);
    refreshDetail();
}

void Controller::setCategory(const QStringList &ids, const QString &category) {
    for (const QString &id : ids) {
        m_store.setCategory(id, category);
        if (id.startsWith(QLatin1String("t:"))) m_btRec[id.mid(2)].category = category;
        else m_httpRec[id.mid(2).toLongLong()].category = category;
    }
    if (!category.isEmpty()) m_store.saveCategory(category, QString());
    tick();
}

void Controller::toggleTag(const QStringList &ids, const QString &tag) {
    if (tag.trimmed().isEmpty()) return;
    bool allHave = true;
    for (const QString &id : ids) {
        const Row *r = row(id);
        if (r && !r->tags.contains(tag)) allHave = false;
    }
    for (const QString &id : ids) {
        QStringList *tags = id.startsWith(QLatin1String("t:")) ? &m_btRec[id.mid(2)].tags : &m_httpRec[id.mid(2).toLongLong()].tags;
        if (allHave) tags->removeAll(tag);
        else if (!tags->contains(tag)) tags->append(tag);
        m_store.setTags(id, *tags);
    }
    tick();
}

void Controller::setLimits(const QStringList &ids, int downKB, int upKB) {
    for (const QString &h : btIds(ids)) m_bt->setLimits(u(h), downKB * 1000, upKB * 1000);
    for (qint64 id : httpIds(ids)) {
        HttpRecord &rec = m_httpRec[id];
        rec.speedLimit = static_cast<qint64>(downKB) * 1000;
        m_store.saveHttp(rec);
        http::Options o;
        o.maxConnections = rec.connections > 0 ? rec.connections : m_settings->i("httpConnections");
        o.speedLimit = rec.speedLimit;
        o.referer = u(rec.referer);
        o.userAgent = u(m_settings->s("httpUserAgent"));
        o.verifyTls = m_settings->b("httpVerifyTls");
        o.minSegment = static_cast<int64_t>(m_settings->i("httpMinSegmentMB")) << 20;
        o.stallSeconds = m_settings->i("httpStallSeconds");
        o.maxRetries = m_settings->i("httpRetries");
        m_http->setOptions(static_cast<uint64_t>(id), o);
    }
}

void Controller::setConnections(const QStringList &ids, int n) {
    for (qint64 id : httpIds(ids)) {
        HttpRecord &rec = m_httpRec[id];
        rec.connections = n;
        m_store.saveHttp(rec);
    }
    for (qint64 id : httpIds(ids)) {
        const HttpRecord &rec = m_httpRec[id];
        http::Options o;
        o.maxConnections = n > 0 ? n : m_settings->i("httpConnections");
        o.speedLimit = rec.speedLimit;
        o.referer = u(rec.referer);
        o.userAgent = u(m_settings->s("httpUserAgent"));
        o.verifyTls = m_settings->b("httpVerifyTls");
        o.minSegment = static_cast<int64_t>(m_settings->i("httpMinSegmentMB")) << 20;
        o.stallSeconds = m_settings->i("httpStallSeconds");
        o.maxRetries = m_settings->i("httpRetries");
        m_http->setOptions(static_cast<uint64_t>(id), o);
    }
}

void Controller::moveTo(const QStringList &ids, const QString &folder) {
    const QString dir = localPath(folder);
    for (const QString &h : btIds(ids)) m_bt->moveStorage(u(h), u(dir));
    for (qint64 id : httpIds(ids)) {
        HttpRecord &rec = m_httpRec[id];
        if (rec.state != QLatin1String("completed")) {
            emit toast(tr("Pause or finish the download before moving it"), QStringLiteral("info"), {});
            continue;
        }
        const QString from = QDir(rec.dir).filePath(rec.fileName), to = QDir(dir).filePath(rec.fileName);
        if (QFile::rename(from, to) || (QFile::copy(from, to) && QFile::remove(from))) {
            rec.dir = dir;
            m_store.saveHttp(rec);
        } else {
            emit toast(tr("Could not move %1").arg(rec.fileName), QStringLiteral("danger"), {});
        }
    }
}

void Controller::addTracker(const QString &id, const QString &url) {
    if (id.startsWith(QLatin1String("t:")) && !url.trimmed().isEmpty()) m_bt->addTracker(u(id.mid(2)), u(url.trimmed()));
}

void Controller::openFolder(const QString &id) {
    if (const Row *r = row(id)) platform::revealInFolder(r->path);
}

void Controller::openFile(const QString &id) {
    const Row *r = row(id);
    if (!r) return;
    if (r->progress < 1.0) { emit toast(tr("The download is not finished yet"), QStringLiteral("info"), {}); return; }
    platform::openPath(r->path);
}

void Controller::openFileAt(const QString &id, const QString &relativePath) {
    const Row *r = row(id);
    if (!r) return;
    const QString base = m_detail.value("savePath").toString();
    platform::openPath(QDir(base.isEmpty() ? QFileInfo(r->path).absolutePath() : base).filePath(relativePath));
}

QVariantMap Controller::extensionDownloads() const {
    // Release builds (x.y.z from CI) have matching extension zips attached to release vX.Y.Z.
    const QString v = version();
    static const QRegularExpression semver(QStringLiteral(R"(^\d+\.\d+\.\d+$)"));
    const bool release = semver.match(v).hasMatch() && v != QLatin1String("0.0.0");
    const QString base = QStringLiteral("https://github.com/%1/releases").arg(platform::kGithubRepo);
    const QString chromium = QStringLiteral("Rift-%1-browser-chromium.zip").arg(v), firefox = QStringLiteral("Rift-%1-browser-firefox.zip").arg(v);
    return {{"release", release},
            {"chromiumFile", chromium},
            {"firefoxFile", firefox},
            {"chromium", release ? QStringLiteral("%1/download/v%2/%3").arg(base, v, chromium) : QString()},
            {"firefox", release ? QStringLiteral("%1/download/v%2/%3").arg(base, v, firefox) : QString()},
            {"page", release ? QStringLiteral("%1/tag/v%2").arg(base, v) : base + QStringLiteral("/latest")}};
}

QString Controller::linkFor(const QString &id) const {
    if (id.startsWith(QLatin1String("t:"))) return q(m_bt->magnet(u(id.mid(2))));
    return m_httpRec.value(id.mid(2).toLongLong()).url;
}

void Controller::copyLinks(const QStringList &ids) {
    QStringList out;
    for (const QString &id : ids) {
        const QString l = linkFor(id);
        if (!l.isEmpty()) out << l;
    }
    copyText(out.join(QLatin1Char('\n')));
    emit toast(out.size() == 1 ? tr("Link copied") : tr("%1 links copied").arg(out.size()), QStringLiteral("ok"), {});
}

void Controller::copyText(const QString &text) {
    m_clipSeen = text.trimmed(); // don't offer what we just copied
    QGuiApplication::clipboard()->setText(text);
}

bool Controller::exportTorrent(const QString &id, const QString &path) {
    std::vector<char> data;
    if (!id.startsWith(QLatin1String("t:")) || !m_bt->exportTorrent(u(id.mid(2)), data)) return false;
    QFile f(localPath(path));
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(data.data(), static_cast<qint64>(data.size()));
    emit toast(tr("Saved %1").arg(QFileInfo(f).fileName()), QStringLiteral("ok"), {});
    return true;
}

void Controller::verifyChecksum(const QString &id, const QString &expected) {
    const Row *r = row(id);
    if (!r) return;
    const QString want = expected.trimmed().toLower().section(QLatin1Char(':'), -1);
    QCryptographicHash::Algorithm algo = QCryptographicHash::Sha256;
    if (want.size() == 32) algo = QCryptographicHash::Md5;
    else if (want.size() == 40) algo = QCryptographicHash::Sha1;
    else if (want.size() == 128) algo = QCryptographicHash::Sha512;
    const QString path = r->path, name = r->name;
    emit toast(tr("Verifying %1…").arg(name), QStringLiteral("info"), {});
    QThreadPool::globalInstance()->start([this, path, name, want, algo] {
        QFile f(path);
        QString got;
        if (f.open(QIODevice::ReadOnly)) {
            QCryptographicHash h(algo);
            h.addData(&f);
            got = QString::fromLatin1(h.result().toHex());
        }
        QMetaObject::invokeMethod(this, [this, name, want, got] {
            if (got.isEmpty()) emit toast(tr("Could not read %1").arg(name), QStringLiteral("danger"), {});
            else if (got == want) emit toast(tr("Checksum matches: %1").arg(name), QStringLiteral("ok"), {});
            else emit toast(tr("Checksum does NOT match: %1").arg(name), QStringLiteral("danger"), {});
        }, Qt::QueuedConnection);
    });
}

void Controller::pauseAll() {
    m_bt->pauseAll();
    QStringList ids;
    for (const Row &r : m_model.rows())
        if (r.kind == QLatin1String("http")) ids << r.id;
    pause(ids);
}

void Controller::resumeAll() {
    m_bt->resumeAll();
    QStringList ids;
    for (const Row &r : m_model.rows())
        if (r.kind == QLatin1String("http") && (r.state == QLatin1String("paused"))) ids << r.id;
    resume(ids);
}

void Controller::setAltSpeed(bool on) { m_settings->set("altSpeed", on); }

// ================================================================ misc

QString Controller::fmtBytes(double b) const { return q(core::formatBytes(static_cast<int64_t>(b), m_settings->b("binaryUnits"))); }
QString Controller::fmtSpeed(double bps) const {
    return q(core::formatSpeed(bps, m_settings->s("speedUnits") == QLatin1String("bits"), m_settings->b("binaryUnits")));
}
QString Controller::fmtEta(double s) const { return q(core::formatEta(s)); }
QString Controller::fmtDate(double ms) const {
    if (ms <= 0) return QStringLiteral("—");
    return QLocale().toString(QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(ms)), QLocale::ShortFormat);
}
QString Controller::fmtRelative(double ms) const {
    if (ms <= 0) return QStringLiteral("—");
    const QDateTime t = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(ms));
    const qint64 s = t.secsTo(QDateTime::currentDateTime());
    if (s < 60) return tr("Just now");
    if (s < 3600) return tr("%1 min ago").arg(s / 60);
    const QDate d = t.date(), today = QDate::currentDate();
    const QString hm = QLocale().toString(t.time(), QLocale::ShortFormat);
    if (d == today) return tr("Today %1").arg(hm);
    if (d == today.addDays(-1)) return tr("Yesterday %1").arg(hm);
    if (d.daysTo(today) < 7) return QLocale().toString(d, QStringLiteral("ddd ")) + hm;
    return QLocale().toString(d, QLocale::ShortFormat);
}

QString Controller::urlToPath(const QUrl &url) const { return url.isLocalFile() ? url.toLocalFile() : url.toString(); }
QUrl Controller::pathToUrl(const QString &path) const { return QUrl::fromLocalFile(path); }

void Controller::checkClipboard() {
    if (!m_settings->b("clipboardWatch")) return;
    const QString text = QGuiApplication::clipboard()->text().trimmed();
    if (text.isEmpty() || text == m_clipSeen || text.size() > 8192) return;
    const QString first = text.section(QLatin1Char('\n'), 0, 0).trimmed();
    const std::string s = u(first);
    bool offer = core::isMagnet(s);
    if (!offer && core::isHttpUrl(s)) {
        const QString name = q(core::fileNameFromUrl(s)).toLower();
        const QStringList types = m_settings->s("browserTypes").split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString &t : types)
            if (name.endsWith(QLatin1Char('.') + t.trimmed().toLower())) { offer = true; break; }
    }
    if (!offer) return;
    m_clipSeen = text;
    m_clipOffer = text;
    emit clipboardChanged();
}

void Controller::dismissClipboard() {
    m_clipOffer.clear();
    emit clipboardChanged();
}

bool Controller::registerHandlers() {
    const bool ok = platform::registerHandlers();
    emit toast(ok ? tr("Rift now opens magnet links and .torrent files") : tr("Could not register file handlers"),
               ok ? QStringLiteral("ok") : QStringLiteral("danger"), {});
    return ok;
}
bool Controller::isDefaultHandler() const { return platform::isDefaultMagnetHandler(); }

QVariantList Controller::history(const QString &query) { return m_store.history(query); }
void Controller::clearHistory() { m_store.clearHistory(); }
QVariantList Controller::dailyStats(int days) { return m_store.stats(days); }
void Controller::saveCategory(const QString &name, const QString &savePath) {
    if (name.trimmed().isEmpty()) return;
    m_store.saveCategory(name.trimmed(), localPath(savePath));
    rebuildCounts();
}
void Controller::removeCategory(const QString &name) {
    m_store.removeCategory(name);
    rebuildCounts();
}

QString Controller::diagnostics() const {
    QStringList l;
    l << QStringLiteral("Rift %1 (%2)").arg(version(), m_portable ? QStringLiteral("portable") : QStringLiteral("installed"));
    l << engineVersions();
    l << QStringLiteral("OS: %1 %2").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    l << QStringLiteral("Data: %1").arg(m_dataDir);
    l << QStringLiteral("Port: %1 · %2 · DHT nodes: %3").arg(m_port).arg(m_netState).arg(m_dht);
    l << QStringLiteral("Downloads: %1 total, %2 active").arg(m_model.rows().size()).arg(m_active);
    l << QStringLiteral("Native host: %1").arg(QFileInfo::exists(platform::nativeHostPath()) ? QStringLiteral("present") : QStringLiteral("missing"));
    return l.join(QLatin1Char('\n'));
}

void Controller::quit() { emit quitRequested(); }

bool Controller::trayAvailable() const { return QSystemTrayIcon::isSystemTrayAvailable(); }

void Controller::notify(const QString &title, const QString &body) {
    if (m_tray && m_tray->isVisible()) m_tray->showMessage(title, body, QSystemTrayIcon::Information, 6000);
}

void Controller::setupTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable() || m_tray) return;
    m_tray = new QSystemTrayIcon(QIcon(QStringLiteral(":/icons/tray.png")), this);
    m_trayMenu = new QMenu();
    m_trayMenu->addAction(tr("Show Rift"), this, &Controller::showWindow);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(tr("Resume all"), this, &Controller::resumeAll);
    m_trayMenu->addAction(tr("Pause all"), this, &Controller::pauseAll);
    m_altAction = m_trayMenu->addAction(tr("Alternative speed limits"));
    m_altAction->setCheckable(true);
    m_altAction->setChecked(m_settings->b("altSpeed"));
    connect(m_altAction, &QAction::toggled, this, &Controller::setAltSpeed);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(tr("Quit Rift"), this, &Controller::quit);
    m_tray->setContextMenu(m_trayMenu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick) emit showWindow();
    });
    connect(m_tray, &QSystemTrayIcon::messageClicked, this, &Controller::showWindow);
    m_tray->setToolTip(QStringLiteral("Rift"));
    m_tray->show();
}

void Controller::updateTray() {
    if (!m_tray || m_tickCount % 2) return;
    m_tray->setToolTip(tr("Rift\n↓ %1 · ↑ %2\n%3 active of %4").arg(fmtSpeed(m_down), fmtSpeed(m_up)).arg(m_active).arg(m_model.rows().size()));
}

// ================================================================ IPC + arguments

void Controller::handleArguments(const QStringList &args) {
    QStringList links;
    for (const QString &a : args) {
        if (a.startsWith(QLatin1String("--"))) continue;
        const std::string s = u(a);
        if (core::isMagnet(s) || core::isHttpUrl(s)) links << a;
        else if (QFileInfo(localPath(a)).isFile() && a.endsWith(QLatin1String(".torrent"), Qt::CaseInsensitive)) {
            emit requestAddTorrent(localPath(a));
        }
    }
    if (!links.isEmpty()) emit requestAdd(links.join(QLatin1Char('\n')), {});
}

QJsonObject Controller::handleIpc(const QJsonObject &msg) {
    const QString type = msg.value("type").toString();
    if (type == QLatin1String("args")) {
        QStringList args;
        for (const QJsonValue &v : msg.value("args").toArray()) args << v.toString();
        handleArguments(args);
        if (!args.contains(QStringLiteral("--minimized"))) emit showWindow();
        return {{"ok", true}};
    }
    if (type == QLatin1String("ping")) return {{"ok", true}, {"version", version()}};
    if (type == QLatin1String("config")) {
        return {{"ok", true}, {"version", version()}, {"enabled", m_settings->b("browserEnabled")},
                {"takeOver", m_settings->b("browserTakeOver")}, {"types", m_settings->s("browserTypes")},
                {"minMB", m_settings->i("browserMinMB")}};
    }
    if (type == QLatin1String("download")) {
        if (!m_settings->b("browserEnabled")) return {{"ok", false}, {"error", "Browser integration is turned off in Rift"}};
        const QString url = msg.value("url").toString().trimmed();
        const std::string s = u(url);
        if (url.size() > 8192 || (!core::isHttpUrl(s) && !core::isMagnet(s))) return {{"ok", false}, {"error", "Unsupported link"}};
        QVariantMap meta{{"referer", msg.value("referer").toString().left(4096)},
                         {"cookies", msg.value("cookies").toString().left(16384)},
                         {"userAgent", msg.value("userAgent").toString().left(512)},
                         {"fileName", q(core::sanitizeFileName(u(msg.value("filename").toString().section(QRegularExpression("[/\\\\]"), -1))))},
                         {"fromBrowser", true}};
        if (msg.value("filename").toString().isEmpty()) meta.remove("fileName");
        if (m_settings->b("browserConfirm")) {
            emit requestAdd(url, meta);
            emit showWindow();
        } else {
            QVariantMap opts = meta;
            addLinks(url, opts);
        }
        return {{"ok", true}};
    }
    return {{"ok", false}, {"error", "unknown request"}};
}
