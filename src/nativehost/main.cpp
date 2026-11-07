// rift-native-host: the browser's Native Messaging bridge (PLAN 4.15).
// The browser starts this process and talks JSON over stdin/stdout, each message prefixed by its
// 32-bit length in native byte order. We forward each message to the running Rift over the
// user-only local socket (starting Rift if needed) and write Rift's reply back. No network port.
#include "../app/ipc.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QThread>

#include <cstdint>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
constexpr uint32_t kMaxMessage = 1024 * 1024; // browsers send at most 4 GB; we accept 1 MB

bool readExact(void *buf, size_t n) { return std::fread(buf, 1, n, stdin) == n; }

void writeMessage(const QJsonObject &o) {
    const QByteArray data = QJsonDocument(o).toJson(QJsonDocument::Compact);
    const uint32_t len = static_cast<uint32_t>(data.size());
    std::fwrite(&len, sizeof len, 1, stdout);
    std::fwrite(data.constData(), 1, static_cast<size_t>(data.size()), stdout);
    std::fflush(stdout);
}

QString riftExecutable() {
    const QString dir = QCoreApplication::applicationDirPath();
#if defined(_WIN32)
    return dir + "/Rift.exe";
#elif defined(__APPLE__)
    return dir + "/Rift"; // both live in Rift.app/Contents/MacOS
#else
    const QString appimage = qEnvironmentVariable("APPIMAGE");
    if (!appimage.isEmpty()) return appimage;
    return QFileInfo::exists(dir + "/rift") ? dir + "/rift" : dir + "/Rift";
#endif
}

// Only these request types are forwarded; the host cannot change settings or touch files.
bool allowed(const QString &type) {
    return type == QLatin1String("ping") || type == QLatin1String("config") || type == QLatin1String("download");
}
} // namespace

int main(int argc, char *argv[]) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    QCoreApplication app(argc, argv);
    for (;;) {
        uint32_t len = 0;
        if (!readExact(&len, sizeof len)) return 0; // browser closed the pipe
        if (len == 0 || len > kMaxMessage) return 1;
        QByteArray buf(static_cast<qsizetype>(len), Qt::Uninitialized);
        if (!readExact(buf.data(), len)) return 1;
        const QJsonDocument doc = QJsonDocument::fromJson(buf);
        if (!doc.isObject()) { writeMessage({{"ok", false}, {"error", "bad message"}}); continue; }
        QJsonObject msg = doc.object();
        const QString type = msg.value("type").toString();
        if (!allowed(type)) { writeMessage({{"ok", false}, {"error", "not allowed"}}); continue; }

        QJsonObject reply;
        if (Ipc::sendToRunning(msg, &reply, 4000)) { writeMessage(reply); continue; }
        if (type == QLatin1String("ping")) { writeMessage({{"ok", false}, {"running", false}}); continue; }
        // Rift is not running: start it in the tray and retry for up to 10 s.
        QProcess::startDetached(riftExecutable(), {QStringLiteral("--minimized")});
        bool sent = false;
        for (int i = 0; i < 40 && !sent; ++i) {
            QThread::msleep(250);
            sent = Ipc::sendToRunning(msg, &reply, 4000);
        }
        writeMessage(sent ? reply : QJsonObject{{"ok", false}, {"error", "Rift did not start"}});
    }
}
