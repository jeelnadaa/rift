#include "platform.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_MACOS
#include <IOKit/pwr_mgt/IOPMLib.h>
#endif

namespace platform {

namespace {
#ifdef Q_OS_MACOS
IOPMAssertionID g_assertion = 0;
#endif
#ifdef Q_OS_LINUX
QProcess *g_inhibit = nullptr;
#endif
bool g_sleepBlocked = false;

QString exePath() { return QCoreApplication::applicationFilePath(); }

QJsonObject manifest(bool firefox) {
    QJsonObject m{{"name", kNativeHostName},
                  {"description", "Rift download manager bridge"},
                  {"path", QDir::toNativeSeparators(nativeHostPath())},
                  {"type", "stdio"}};
    if (firefox) {
        m.insert("allowed_extensions", QJsonArray{kFirefoxExtensionId});
    } else {
        QJsonArray origins;
        for (const QString &id : kChromiumExtensionIds) origins.append(QStringLiteral("chrome-extension://%1/").arg(id));
        m.insert("allowed_origins", origins);
    }
    return m;
}

bool writeJson(const QString &path, const QJsonObject &o) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return true;
}
} // namespace

void preventSleep(bool on) {
    if (on == g_sleepBlocked) return;
    g_sleepBlocked = on;
#if defined(Q_OS_WIN)
    SetThreadExecutionState(on ? (ES_CONTINUOUS | ES_SYSTEM_REQUIRED) : ES_CONTINUOUS);
#elif defined(Q_OS_MACOS)
    if (on) {
        IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleSystemSleep, kIOPMAssertionLevelOn,
                                    CFSTR("Rift is downloading"), &g_assertion);
    } else if (g_assertion) {
        IOPMAssertionRelease(g_assertion);
        g_assertion = 0;
    }
#elif defined(Q_OS_LINUX)
    if (on) {
        if (!g_inhibit && !QStandardPaths::findExecutable("systemd-inhibit").isEmpty()) {
            g_inhibit = new QProcess();
            g_inhibit->start("systemd-inhibit", {"--what=sleep:idle", "--who=Rift", "--why=Downloading", "--mode=block",
                                                 "sleep", "infinity"});
        }
    } else if (g_inhibit) {
        g_inhibit->kill();
        g_inhibit->waitForFinished(1000);
        delete g_inhibit;
        g_inhibit = nullptr;
    }
#endif
}

void revealInFolder(const QString &path) {
    const QFileInfo fi(path);
#if defined(Q_OS_WIN)
    if (fi.exists()) {
        QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(fi.absoluteFilePath())});
        return;
    }
#elif defined(Q_OS_MACOS)
    if (fi.exists()) {
        QProcess::startDetached("open", {"-R", fi.absoluteFilePath()});
        return;
    }
#endif
    const QString dir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void openPath(const QString &path) { QDesktopServices::openUrl(QUrl::fromLocalFile(path)); }

qint64 freeSpace(const QString &path) {
    QString p = path;
    while (!p.isEmpty() && !QFileInfo::exists(p)) p = QFileInfo(p).absolutePath() == p ? QString() : QFileInfo(p).absolutePath();
    if (p.isEmpty()) return -1;
    const QStorageInfo si(p);
    return si.isValid() ? si.bytesAvailable() : -1;
}

QString nativeHostPath() {
    const QString dir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    return dir + "/rift-native-host.exe";
#else
    return dir + "/rift-native-host";
#endif
}

void registerNativeHost() {
    if (!QFileInfo::exists(nativeHostPath())) return;
    const QJsonObject chromium = manifest(false), firefox = manifest(true);
#if defined(Q_OS_WIN)
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/NativeMessaging/";
    const QString cPath = base + kNativeHostName + ".json", fPath = base + kNativeHostName + ".firefox.json";
    writeJson(cPath, chromium);
    writeJson(fPath, firefox);
    const QStringList chromiumKeys = {"Google\\Chrome", "Microsoft\\Edge", "BraveSoftware\\Brave-Browser", "Chromium",
                                      "Vivaldi"};
    for (const QString &k : chromiumKeys) {
        QSettings reg("HKEY_CURRENT_USER\\Software\\" + k + "\\NativeMessagingHosts\\" + kNativeHostName, QSettings::NativeFormat);
        reg.setValue(".", QDir::toNativeSeparators(cPath));
    }
    QSettings ff("HKEY_CURRENT_USER\\Software\\Mozilla\\NativeMessagingHosts\\" + kNativeHostName, QSettings::NativeFormat);
    ff.setValue(".", QDir::toNativeSeparators(fPath));
#elif defined(Q_OS_MACOS)
    const QString home = QDir::homePath() + "/Library/Application Support/";
    const QStringList dirs = {"Google/Chrome", "Microsoft Edge", "BraveSoftware/Brave-Browser", "Chromium", "Vivaldi"};
    for (const QString &d : dirs)
        if (QFileInfo::exists(home + d)) writeJson(home + d + "/NativeMessagingHosts/" + kNativeHostName + ".json", chromium);
    writeJson(home + "Mozilla/NativeMessagingHosts/" + kNativeHostName + ".json", firefox);
#else
    const QString cfg = QDir::homePath() + "/.config/";
    const QStringList dirs = {"google-chrome", "chromium", "microsoft-edge", "BraveSoftware/Brave-Browser", "vivaldi"};
    for (const QString &d : dirs)
        if (QFileInfo::exists(cfg + d)) writeJson(cfg + d + "/NativeMessagingHosts/" + kNativeHostName + ".json", chromium);
    writeJson(QDir::homePath() + "/.mozilla/native-messaging-hosts/" + kNativeHostName + ".json", firefox);
#endif
}

bool registerHandlers() {
#if defined(Q_OS_WIN)
    const QString exe = QDir::toNativeSeparators(exePath());
    const QString cmd = QStringLiteral("\"%1\" \"%2\"").arg(exe, "%1");
    QSettings magnet("HKEY_CURRENT_USER\\Software\\Classes\\magnet", QSettings::NativeFormat);
    magnet.setValue(".", "URL:Magnet link");
    magnet.setValue("URL Protocol", "");
    magnet.setValue("DefaultIcon/.", exe + ",0");
    magnet.setValue("shell/open/command/.", cmd);
    QSettings prog("HKEY_CURRENT_USER\\Software\\Classes\\Rift.Torrent", QSettings::NativeFormat);
    prog.setValue(".", "Torrent file");
    prog.setValue("DefaultIcon/.", exe + ",0");
    prog.setValue("shell/open/command/.", cmd);
    QSettings ext("HKEY_CURRENT_USER\\Software\\Classes\\.torrent", QSettings::NativeFormat);
    ext.setValue(".", "Rift.Torrent");
    return true;
#elif defined(Q_OS_LINUX)
    if (QStandardPaths::findExecutable("xdg-mime").isEmpty()) return false;
    QProcess::execute("xdg-mime", {"default", "rift.desktop", "x-scheme-handler/magnet"});
    QProcess::execute("xdg-mime", {"default", "rift.desktop", "application/x-bittorrent"});
    return true;
#else
    return true; // macOS: declared in Info.plist; the user picks the default in Finder.
#endif
}

bool isDefaultMagnetHandler() {
#if defined(Q_OS_WIN)
    QSettings magnet("HKEY_CURRENT_USER\\Software\\Classes\\magnet\\shell\\open\\command", QSettings::NativeFormat);
    return magnet.value(".").toString().contains(QDir::toNativeSeparators(exePath()), Qt::CaseInsensitive);
#elif defined(Q_OS_LINUX)
    QProcess p;
    p.start("xdg-mime", {"query", "default", "x-scheme-handler/magnet"});
    if (!p.waitForFinished(1500)) return false;
    return p.readAllStandardOutput().contains("rift");
#else
    return true;
#endif
}

void setLaunchAtLogin(bool on) {
#if defined(Q_OS_WIN)
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (on) run.setValue("Rift", QStringLiteral("\"%1\" --minimized").arg(QDir::toNativeSeparators(exePath())));
    else run.remove("Rift");
#elif defined(Q_OS_MACOS)
    const QString plist = QDir::homePath() + "/Library/LaunchAgents/app.rift.Rift.plist";
    if (!on) { QFile::remove(plist); return; }
    QString app = exePath();
    QFile f(plist);
    QDir().mkpath(QFileInfo(plist).absolutePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QStringLiteral(
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                    "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<plist version=\"1.0\"><dict>"
                    "<key>Label</key><string>app.rift.Rift</string><key>ProgramArguments</key><array><string>%1</string>"
                    "<string>--minimized</string></array><key>RunAtLoad</key><true/></dict></plist>\n")
                    .arg(app.toHtmlEscaped())
                    .toUtf8());
    }
#else
    const QString file = QDir::homePath() + "/.config/autostart/rift.desktop";
    if (!on) { QFile::remove(file); return; }
    QString exe = qEnvironmentVariable("APPIMAGE");
    if (exe.isEmpty()) exe = exePath();
    QDir().mkpath(QFileInfo(file).absolutePath());
    QFile f(file);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QStringLiteral("[Desktop Entry]\nType=Application\nName=Rift\nExec=\"%1\" --minimized\nIcon=rift\n"
                               "X-GNOME-Autostart-enabled=true\n").arg(exe).toUtf8());
#endif
}

} // namespace platform
