// UI film recorder: plays scripted "takes" of the real QML UI (same module and mock controller as the
// harness) and saves every frame as a PNG. Qt's animation clock is replaced by a fixed 1/30 s step, so
// every frame is deterministic and the app's own springs, staggers and fades are captured exactly.
// Used to make the launch video (brag-output/). Not part of the app or the tests.
//   QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2 rift_ui_film <take> <outDir> [seconds]
//   takes: list | lanes | add | stats | light
#include "app/downloadmodel.hpp"
#include "app/fonts.hpp"
#include "app/settings.hpp"

#include <QAnimationDriver>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <cmath>
#include <cstdio>
#include <functional>

Q_IMPORT_QML_PLUGIN(RiftPlugin)

namespace {
constexpr double kFps = 30.0;

class StepDriver : public QAnimationDriver {
public:
    qint64 elapsed() const override { return static_cast<qint64>(m_ms); }
    void step() { m_ms += 1000.0 / kFps; advance(); }
private:
    double m_ms = 0;
};

constexpr double GB = 1e9;

// Deterministic wobble so speeds look alive but every render is identical.
double wob(double t, double seed) { return 0.5 * std::sin(t * 2.3 + seed) + 0.3 * std::sin(t * 5.1 + seed * 1.7) + 0.2 * std::sin(t * 9.7 + seed * 3.1); }

struct Item {
    QString id, kind, name, state, category, site;
    double size, start, rate; // bytes, initial fraction, fraction per second
    int seeds, peers, conns;
};

const QList<Item> kItems = {
    {"t:a1", "bt", "ubuntu-26.04-desktop-amd64.iso", "downloading", "Disc images", "torrent.ubuntu.com", 6.1 * GB, 0.31, 0.004, 48, 112, 0},
    {"h:1", "http", "debian-13.2.0-amd64-DVD-1.iso", "downloading", "Disc images", "cdimage.debian.org", 3.9 * GB, 0.12, 0.016, 0, 0, 8},
    {"t:b2", "bt", "Big Buck Bunny (4K, 60 fps)", "downloading", "Video", "webtorrent.io", 8.4 * GB, 0.58, 0.0012, 22, 61, 0},
    {"h:2", "http", "blender-5.0-windows-x64.msi", "downloading", "Programs", "download.blender.org", 0.42 * GB, 0.44, 0.05, 0, 0, 8},
    {"t:c3", "bt", "Sintel (open movie)", "seeding", "Video", "webtorrent.io", 1.3 * GB, 1.0, 0, 9, 14, 0},
    {"h:3", "http", "fedora-workstation-43.iso", "completed", "Disc images", "download.fedoraproject.org", 2.6 * GB, 1.0, 0, 0, 0, 0},
    {"t:d4", "bt", "Tears of Steel (open movie)", "paused", "Video", "webtorrent.io", 3.4 * GB, 0.27, 0, 0, 0, 0},
};

double frac(const Item &it, double t) { return std::min(1.0, it.start + it.rate * t); }

// HTTP lanes: n connections, each owns an equal slice and fills at its own pace.
QVariantList lanes(const Item &it, double t, int n, bool bytes) {
    QVariantList out;
    const double f = frac(it, t);
    for (int i = 0; i < n; ++i) {
        const double s = double(i) / n, e = double(i + 1) / n;
        const double pace = 0.85 + 0.3 * std::fabs(std::sin(i * 1.9 + 0.4));
        const double c = std::min(e, s + (e - s) * std::min(1.0, f * pace));
        const double v = (it.rate * it.size / n) * (0.75 + 0.25 * wob(t, i));
        if (bytes) out.push_back(QVariantMap{{"start", s * it.size}, {"cursor", c * it.size}, {"end", e * it.size}, {"speed", v}});
        else out.push_back(QVariantMap{{"s", s}, {"e", e}, {"c", c}, {"v", v}});
    }
    return out;
}

Row makeRow(const Item &it, double t) {
    Row r;
    r.id = it.id; r.kind = it.kind; r.name = it.name; r.category = it.category; r.site = it.site;
    r.size = static_cast<qint64>(it.size);
    r.progress = frac(it, t);
    r.state = it.state == QLatin1String("downloading") && r.progress >= 1.0 ? QStringLiteral("completed") : it.state;
    r.done = static_cast<qint64>(r.size * r.progress);
    r.added = QDateTime(QDate(2026, 11, 7), QTime(12, 0)).toMSecsSinceEpoch();
    if (r.state == QLatin1String("downloading")) {
        r.down = it.rate * it.size * (1.0 + 0.12 * wob(t, it.size / GB));
        r.eta = (1.0 - r.progress) / it.rate;
        r.seeds = it.seeds; r.peers = it.peers; r.conns = it.conns;
        if (it.kind == QLatin1String("http")) r.segs = lanes(it, t, it.conns, false);
    }
    if (r.state == QLatin1String("seeding")) { r.up = 4.1e5 * (1.0 + 0.2 * wob(t, 3)); r.ratio = 2.31; r.seeds = it.seeds; r.peers = it.peers; }
    if (it.kind == QLatin1String("bt")) r.tags = QStringList{QStringLiteral("open")};
    return r;
}
} // namespace

int main(int argc, char *argv[]) {
    qputenv("QT_FORCE_STDERR_LOGGING", "1");
    QApplication app(argc, argv);
    const QStringList a = app.arguments();
    if (a.size() < 3) { std::fprintf(stderr, "usage: rift_ui_film <take> <outDir> [seconds]\n"); return 1; }
    const QString take = a.at(1), outDir = a.at(2);
    const double seconds = a.size() > 3 ? a.at(3).toDouble() : 4.0;
    QDir().mkpath(outDir);

    auto *driver = new StepDriver;
    driver->install();
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    installAppFonts();

    QTemporaryDir dir;
    Settings settings(dir.filePath(QStringLiteral("settings.ini")));
    settings.set(QStringLiteral("savePath"), QStringLiteral("D:/Downloads"));
    settings.set(QStringLiteral("winW"), 1440);
    settings.set(QStringLiteral("winH"), 810);
    settings.set(QStringLiteral("theme"), take == QLatin1String("light") ? QStringLiteral("stone") : QStringLiteral("obsidian"));
    settings.set(QStringLiteral("density"), QStringLiteral("comfortable"));
    settings.set(QStringLiteral("detailsHeight"), 300);

    DownloadModel model;
    DownloadFilter filter;
    filter.setSourceModel(&model);

    QQmlApplicationEngine engine;
    QQmlComponent mockComp(&engine, QUrl(QStringLiteral("qrc:/harness/tests/ui/MockApp.qml")));
    QObject *mock = mockComp.create();
    if (!mock) { std::fprintf(stderr, "MockApp.qml failed: %s\n", qPrintable(mockComp.errorString())); return 2; }
    mock->setProperty("settings", QVariant::fromValue<QObject *>(&settings));
    mock->setProperty("list", QVariant::fromValue<QObject *>(&filter));
    mock->setProperty("clipboardOffer", QString());
    mock->setProperty("netState", QStringLiteral("ok"));
    mock->setProperty("dhtNodes", 418);
    mock->setProperty("freeBytes", 812e9);
    mock->setProperty("categories", QVariantList{QVariantMap{{"name", "Disc images"}, {"count", 3}}, QVariantMap{{"name", "Video"}, {"count", 3}},
                                                 QVariantMap{{"name", "Programs"}, {"count", 1}}});
    mock->setProperty("tags", QVariantList{QVariantMap{{"name", "open"}, {"count", 4}}});
    mock->setProperty("sites", QVariantList{});
    engine.rootContext()->setContextProperty(QStringLiteral("App"), mock);
    engine.rootContext()->setContextProperty(QStringLiteral("StartMinimized"), false);
    engine.loadFromModule("Rift", "Main");
    if (engine.rootObjects().isEmpty()) return 3;
    QObject *root = engine.rootObjects().first();
    auto *win = qobject_cast<QQuickWindow *>(root);
    win->setWidth(1440);
    win->setHeight(810);

    const bool emptyStart = take == QLatin1String("list");
    const double rowsAt = emptyStart ? 0.7 : -1.0;
    double speedHist[160];
    for (int i = 0; i < 160; ++i) speedHist[i] = 0;

    auto apply = [&](double t) {
        const double tt = t + 4.0; // shows downloads already under way
        QVector<Row> rows;
        if (t >= rowsAt)
            for (const auto &it : kItems) rows.push_back(makeRow(it, tt));
        model.update(rows);
        double down = 0, up = 0;
        int dl = 0;
        for (const auto &r : rows) { down += r.down; up += r.up; if (r.state == QLatin1String("downloading")) ++dl; }
        mock->setProperty("downSpeed", down);
        mock->setProperty("upSpeed", up);
        mock->setProperty("downloadingCount", dl);
        mock->setProperty("activeCount", dl + (rows.isEmpty() ? 0 : 1));
        mock->setProperty("totalCount", int(rows.size()));
        const int n = rows.size();
        mock->setProperty("counts", QVariantMap{{"all", n}, {"downloading", n ? dl : 0}, {"seeding", n ? 1 : 0}, {"completed", n ? 1 : 0},
                                                {"paused", n ? 1 : 0}, {"active", n ? dl + 1 : 0}, {"inactive", n ? 2 : 0}, {"stalled", 0},
                                                {"checking", 0}, {"error", 0}, {"k:bt", n ? 4 : 0}, {"k:http", n ? 3 : 0}});
        // Speed history: one sample per frame, newest last.
        QVariantList hist;
        for (int i = 0; i < 160; ++i) {
            const double ts = tt - (159 - i) * 0.12;
            const double d = 1.15e8 + 3.2e7 * std::sin(ts * 0.9) + 1.4e7 * std::sin(ts * 2.7 + 1) + 6e6 * std::sin(ts * 6.1);
            hist.push_back(QVariantMap{{"d", std::max(0.0, d)}, {"u", 4e5 + 1.5e5 * std::sin(ts * 1.3)}});
        }
        mock->setProperty("speedHistory", hist);
        mock->setProperty("sessionDown", 41.7e9 + tt * down);
        mock->setProperty("sessionUp", 6.2e9 + tt * up);
        if (take == QLatin1String("lanes") || take == QLatin1String("light")) {
            const Item &it = kItems.at(1);
            const Row r = makeRow(it, tt);
            QVariantMap d{{"id", it.id}, {"kind", "http"}, {"name", it.name}, {"state", r.state}, {"size", it.size}, {"done", double(r.done)},
                          {"progress", r.progress}, {"down", r.down}, {"up", 0}, {"eta", r.eta}, {"ratio", 0}, {"conns", 8},
                          {"category", it.category}, {"tags", QStringList{}}, {"site", it.site}, {"path", "D:/Downloads"},
                          {"error", ""}, {"added", double(r.added)}, {"completed", 0}, {"segs", QVariantList{}}, {"source", ""},
                          {"resumable", true}, {"queuePos", 0}, {"connections", lanes(it, tt, 8, true)}, {"speedNow", r.down},
                          {"firstConnSpeed", r.down / 6.4}, {"url", "https://cdimage.debian.org/debian-cd/current/amd64/iso-dvd/" + it.name},
                          {"finalUrl", "https://cdimage.debian.org/debian-cd/current/amd64/iso-dvd/" + it.name},
                          {"referer", ""}, {"retries", 0}, {"savePath", "D:/Downloads"}, {"downloaded", double(r.done)}};
            mock->setProperty("detail", d);
        }
    };

    // One-time setup per take.
    QList<QPair<double, std::function<void()>>> cues;
    if (take == QLatin1String("lanes") || take == QLatin1String("light")) {
        cues.push_back({0.0, [&] {
            mock->setProperty("selection", QStringList{QStringLiteral("h:1")});
            mock->setProperty("current", QStringLiteral("h:1"));
            mock->setProperty("detailTab", QStringLiteral("overview"));
        }});
    }
    if (take == QLatin1String("stats")) cues.push_back({0.0, [&] { root->setProperty("page", QStringLiteral("stats")); }});
    QVariantMap addMeta;
    addMeta.insert(QStringLiteral("referer"), QStringLiteral("https://www.kernel.org/"));
    if (take == QLatin1String("add")) {
        cues.push_back({0.5, [&] {
            QMetaObject::invokeMethod(root, "openAdd", Q_ARG(QVariant, QStringLiteral("https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.17.tar.xz")),
                                      Q_ARG(QVariant, QVariant(addMeta)));
        }});
        cues.push_back({2.6, [&] {
            QKeyEvent p(QEvent::KeyPress, Qt::Key_Return, Qt::ControlModifier);
            QKeyEvent r(QEvent::KeyRelease, Qt::Key_Return, Qt::ControlModifier);
            QGuiApplication::sendEvent(win, &p);
            QGuiApplication::sendEvent(win, &r);
        }});
    }

    // Settle the window (fonts, first layout) before frame 0.
    apply(0);
    for (int i = 0; i < 20; ++i) { app.processEvents(); driver->step(); }
    win->grabWindow();

    const int frames = static_cast<int>(std::lround(seconds * kFps));
    int cue = 0;
    for (int f = 0; f < frames; ++f) {
        const double t = f / kFps;
        while (cue < cues.size() && cues.at(cue).first <= t + 1e-6) cues.at(cue++).second();
        apply(t);
        app.processEvents();
        driver->step();
        app.processEvents();
        const QImage img = win->grabWindow();
        img.save(QStringLiteral("%1/%2.png").arg(outDir).arg(f, 4, 10, QLatin1Char('0')), nullptr, 1);
        if (f % 30 == 0) std::fprintf(stderr, "%s: frame %d/%d\n", qPrintable(take), f, frames);
    }
    return 0;
}
