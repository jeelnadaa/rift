// UI harness: loads the real QML module "Rift" against a mock controller (tests/ui/MockApp.qml) backed by
// the real Settings and DownloadModel/DownloadFilter, calls Main.smokeTest(), and fails on any QML warning.
// Needs only Qt, so it runs anywhere (CI on every OS, and locally without libtorrent/libcurl).
//   QT_QPA_PLATFORM=offscreen rift_ui_harness [--show]
#include "app/downloadmodel.hpp"
#include "app/fonts.hpp"
#include "app/settings.hpp"

#include <QApplication>
#include <QDateTime>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QImage>
#include <QTemporaryDir>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>
#include <cstdio>

Q_IMPORT_QML_PLUGIN(RiftPlugin)

namespace {
int g_problems = 0;
QtMessageHandler g_prev = nullptr;

void handler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
    const bool qml = msg.contains(QLatin1String(".qml")) || msg.contains(QLatin1String("qrc:")) ||
                     (ctx.category && QByteArray(ctx.category).startsWith("qml"));
    if (type != QtDebugMsg && type != QtInfoMsg && qml) ++g_problems;
    if (g_prev) g_prev(type, ctx, msg);
}

Row row(const QString &id, const QString &kind, const QString &name, const QString &state, double progress) {
    Row r;
    r.id = id;
    r.kind = kind;
    r.name = name;
    r.state = state;
    r.size = 4LL * 1024 * 1024 * 1024;
    r.done = static_cast<qint64>(r.size * progress);
    r.progress = progress;
    r.added = QDateTime::currentMSecsSinceEpoch() - 3600000;
    r.category = kind == QLatin1String("bt") ? QStringLiteral("Video") : QStringLiteral("Archives");
    r.site = kind == QLatin1String("bt") ? QStringLiteral("tracker.example.org") : QStringLiteral("example.com");
    if (state == QLatin1String("downloading")) { r.down = 5.4e6; r.eta = 420; r.seeds = 12; r.peers = 30; r.conns = 6; }
    if (state == QLatin1String("seeding")) { r.up = 2.2e5; r.ratio = 1.4; }
    if (state == QLatin1String("error")) r.error = QStringLiteral("Disk full");
    if (kind == QLatin1String("http") && state == QLatin1String("downloading"))
        for (int i = 0; i < 4; ++i)
            r.segs.push_back(QVariantMap{{"s", i / 4.0}, {"e", (i + 1) / 4.0}, {"c", i / 4.0 + 0.1}, {"v", 1.2e6}});
    if (kind == QLatin1String("bt")) r.tags = QStringList{QStringLiteral("linux")};
    return r;
}
} // namespace

int main(int argc, char *argv[]) {
    qputenv("QT_FORCE_STDERR_LOGGING", "1");
    const bool fontsViaSystem = platform::registerAppFontsWithSystem();
    QApplication app(argc, argv);
    const bool show = app.arguments().contains(QStringLiteral("--show"));
    // --shots <dir>: save screenshots of a few states (light + dark) for visual review.
    const int shotsIdx = app.arguments().indexOf(QStringLiteral("--shots"));
    const QString shots = shotsIdx > 0 && shotsIdx + 1 < app.arguments().size() ? app.arguments().at(shotsIdx + 1) : QString();
    g_prev = qInstallMessageHandler(handler);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    installAppFonts(fontsViaSystem);

    QTemporaryDir dir;
    Settings settings(dir.filePath(QStringLiteral("settings.ini")));
    settings.set(QStringLiteral("savePath"), dir.path());

    DownloadModel model;
    model.update({row("t:aaaa", "bt", "ubuntu-26.04-desktop-amd64.iso", "downloading", 0.42),
                  row("h:1", "http", "dataset-2026.tar.gz", "downloading", 0.63),
                  row("t:bbbb", "bt", "Big Buck Bunny 4K", "seeding", 1.0),
                  row("h:2", "http", "installer.exe", "error", 0.1),
                  row("h:3", "http", "paper.pdf", "completed", 1.0),
                  row("t:cccc", "bt", "Sintel", "paused", 0.2)});
    DownloadFilter filter;
    filter.setSourceModel(&model);

    QQmlApplicationEngine engine;
    QQmlComponent mockComp(&engine, QUrl(QStringLiteral("qrc:/harness/tests/ui/MockApp.qml")));
    QObject *mock = mockComp.create();
    if (!mock) {
        std::fprintf(stderr, "MockApp.qml failed: %s\n", qPrintable(mockComp.errorString()));
        return 2;
    }
    mock->setProperty("settings", QVariant::fromValue<QObject *>(&settings));
    mock->setProperty("list", QVariant::fromValue<QObject *>(&filter));
    engine.rootContext()->setContextProperty(QStringLiteral("App"), mock);
    engine.rootContext()->setContextProperty(QStringLiteral("StartMinimized"), false);
    engine.loadFromModule("Rift", "Main");
    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr, "Main.qml did not load\n");
        return 3;
    }
    QObject *root = engine.rootObjects().first();
    if (show) return app.exec();

    if (!shots.isEmpty()) {
        auto *win = qobject_cast<QQuickWindow *>(root);
        QMetaObject::invokeMethod(mock, "selectFirst");
        int t = 900;
        const QList<QPair<QString, QString>> steps = {{"stone", "main-light"}, {"obsidian", "main-dark"}};
        for (const auto &st : steps) {
            QTimer::singleShot(t, root, [&settings, st] { settings.set(QStringLiteral("theme"), st.first); });
            QTimer::singleShot(t + 700, root, [win, shots, st] { win->grabWindow().save(shots + "/" + st.second + ".png"); });
            t += 1400;
        }
        QTimer::singleShot(t, root, [root] { QMetaObject::invokeMethod(root, "openAdd", Q_ARG(QVariant, QString("https://example.com/a.zip")), Q_ARG(QVariant, QVariantMap())); });
        QTimer::singleShot(t + 900, root, [win, shots] { win->grabWindow().save(shots + "/add-dark.png"); });
        QTimer::singleShot(t + 1000, root, [root] { QMetaObject::invokeMethod(root, "openSettings", Q_ARG(QVariant, QString("appearance"))); });
        QTimer::singleShot(t + 1900, root, [win, shots] { win->grabWindow().save(shots + "/settings-dark.png"); });
        QTimer::singleShot(t + 2000, root, [root] { QMetaObject::invokeMethod(root, "openExtension"); });
        QTimer::singleShot(t + 2900, root, [win, shots] { win->grabWindow().save(shots + "/extension-dark.png"); });
        QTimer::singleShot(t + 3100, &app, [&app] { app.exit(g_problems == 0 ? 0 : 4); });
        return app.exec();
    }

    QTimer::singleShot(800, root, [root, mock] {
        QMetaObject::invokeMethod(mock, "selectFirst");
        QMetaObject::invokeMethod(root, "smokeTest");
    });
    QTimer::singleShot(3500, &app, [&app] {
        std::printf("ui harness: %d QML problem(s)\n", g_problems);
        app.exit(g_problems == 0 ? 0 : 4);
    });
    return app.exec();
}
