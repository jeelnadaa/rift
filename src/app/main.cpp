// Rift entry point: single instance, portable mode, QML bootstrap, and the CI smoke test.
#include "controller.hpp"
#include "fonts.hpp"
#include "ipc.hpp"
#include "version.hpp"

#include <QApplication>
#include <QtQml/qqmlextensionplugin.h>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QFontDatabase>
#include <QIcon>
#include <QJsonArray>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdio>

Q_IMPORT_QML_PLUGIN(RiftPlugin)

namespace {
int g_qmlProblems = 0;
QtMessageHandler g_prevHandler = nullptr;

void smokeHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
    const bool qml = msg.contains(QLatin1String(".qml")) || msg.contains(QLatin1String("qrc:")) ||
                     (ctx.category && QByteArray(ctx.category).startsWith("qml"));
    if (type != QtDebugMsg && type != QtInfoMsg && qml) ++g_qmlProblems;
    if (g_prevHandler) g_prevHandler(type, ctx, msg);
}

// macOS delivers magnet links and opened .torrent files as events, not argv.
class OpenFilter : public QObject {
public:
    explicit OpenFilter(Controller *c) : m_c(c) {}
    bool eventFilter(QObject *o, QEvent *e) override {
        if (e->type() == QEvent::FileOpen) {
            auto *fe = static_cast<QFileOpenEvent *>(e);
            const QString arg = fe->url().isLocalFile() ? fe->url().toLocalFile() : fe->url().toString();
            m_c->handleArguments({arg});
            emit m_c->showWindow();
            return true;
        }
        return QObject::eventFilter(o, e);
    }

private:
    Controller *m_c;
};
} // namespace

int main(int argc, char *argv[]) {
    QApplication::setApplicationName(QStringLiteral("Rift"));
    QApplication::setOrganizationName(QStringLiteral("Rift"));
    QApplication::setApplicationVersion(QStringLiteral(RIFT_VERSION));
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    const bool fontsViaSystem = platform::registerAppFontsWithSystem(); // before the app object
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/rift-256.png")));
    app.setQuitOnLastWindowClosed(false);

    const QStringList args = app.arguments().mid(1);
    if (args.contains(QStringLiteral("--version")) || args.contains(QStringLiteral("-v"))) {
        std::printf("Rift %s\n", RIFT_VERSION);
        return 0;
    }
    const bool smoke = args.contains(QStringLiteral("--smoke-test"));

    // Data folder: portable when portable.flag sits next to the executable (or --portable).
    const QString appDir = QCoreApplication::applicationDirPath();
    const bool portable = !smoke && (args.contains(QStringLiteral("--portable")) || QFileInfo::exists(appDir + "/portable.flag"));
    QTemporaryDir smokeDir;
    QString dataDir = portable ? appDir + "/Data" : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (smoke) dataDir = smokeDir.path();

    // Single instance: hand our arguments to the running Rift and exit.
    if (!smoke) {
        QJsonArray a;
        for (const QString &s : args) a.append(s);
        if (Ipc::sendToRunning({{"type", "args"}, {"args", a}})) return 0;
    }

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    installAppFonts(fontsViaSystem);

    if (smoke) g_prevHandler = qInstallMessageHandler(smokeHandler);

    Controller ctrl(dataDir, portable);
    QString error;
    if (!ctrl.start(&error)) {
        std::fprintf(stderr, "Rift could not start: %s\n", qPrintable(error));
        return 2;
    }
    Ipc ipc;
    ipc.handler = [&ctrl](const QJsonObject &m) { return ctrl.handleIpc(m); };
    if (!smoke) ipc.listen();

    OpenFilter openFilter(&ctrl);
    app.installEventFilter(&openFilter);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("App"), &ctrl);
    engine.rootContext()->setContextProperty(QStringLiteral("StartMinimized"),
                                             args.contains(QStringLiteral("--minimized")) || ctrl.settings()->b("startMinimized"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(3); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Rift", "Main");
    if (engine.rootObjects().isEmpty()) return 3;

    bool quitting = false;
    auto doQuit = [&] {
        if (quitting) return;
        quitting = true;
        for (QObject *o : engine.rootObjects())
            if (auto *w = qobject_cast<QQuickWindow *>(o)) w->hide();
        ctrl.shutdown();
        QCoreApplication::quit();
    };
    QObject::connect(&ctrl, &Controller::quitRequested, &app, doQuit);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&] { ctrl.shutdown(); });

    if (smoke) {
        // CI: load everything, exercise the main views, and fail on any QML warning.
        QTimer::singleShot(2500, &app, [&] {
            QMetaObject::invokeMethod(engine.rootObjects().first(), "smokeTest");
        });
        QTimer::singleShot(6000, &app, [&] {
            std::printf("smoke test: %d QML problem(s)\n", g_qmlProblems);
            ctrl.shutdown();
            QCoreApplication::exit(g_qmlProblems ? 4 : 0);
        });
    } else {
        QTimer::singleShot(400, &ctrl, [&ctrl, args] { ctrl.handleArguments(args); });
    }
    return app.exec();
}
