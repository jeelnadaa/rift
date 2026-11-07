#include "ipc.hpp"

#include <QCryptographicHash>
#include <algorithm>
#include <QDir>
#include <QJsonDocument>
#include <QLocalSocket>

QString Ipc::serverName() {
    // Per-user name; the hash keeps it short and free of odd characters.
    const QByteArray who = qgetenv("USER") + qgetenv("USERNAME") + QDir::homePath().toUtf8();
    return QStringLiteral("rift-") + QString::fromLatin1(QCryptographicHash::hash(who, QCryptographicHash::Sha1).toHex().left(12));
}

bool Ipc::sendToRunning(const QJsonObject &msg, QJsonObject *reply, int timeoutMs) {
    QLocalSocket s;
    s.connectToServer(serverName());
    if (!s.waitForConnected(std::min(timeoutMs, 800))) return false;
    s.write(QJsonDocument(msg).toJson(QJsonDocument::Compact) + '\n');
    if (!s.waitForBytesWritten(timeoutMs)) return false;
    QByteArray buf;
    while (!buf.contains('\n')) {
        if (!s.waitForReadyRead(timeoutMs)) break;
        buf += s.readAll();
    }
    if (reply) *reply = QJsonDocument::fromJson(buf.left(buf.indexOf('\n'))).object();
    s.disconnectFromServer();
    return true;
}

Ipc::Ipc(QObject *parent) : QObject(parent) {
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *c = m_server.nextPendingConnection()) {
            connect(c, &QLocalSocket::disconnected, c, &QObject::deleteLater);
            connect(c, &QLocalSocket::readyRead, c, [this, c] {
                // Lines are capped at 1 MiB; a misbehaving client gets disconnected.
                if (c->bytesAvailable() > (1 << 20) && !c->canReadLine()) { c->abort(); return; }
                while (c->canReadLine()) {
                    const QByteArray line = c->readLine(1 << 20).trimmed();
                    if (line.isEmpty()) continue;
                    QJsonParseError err{};
                    const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
                    QJsonObject reply;
                    if (err.error != QJsonParseError::NoError || !doc.isObject())
                        reply = {{"ok", false}, {"error", "bad message"}};
                    else
                        reply = handler ? handler(doc.object()) : QJsonObject{{"ok", false}};
                    c->write(QJsonDocument(reply).toJson(QJsonDocument::Compact) + '\n');
                    c->flush();
                }
            });
        }
    });
}

bool Ipc::listen() {
    if (m_server.listen(serverName())) return true;
    // A stale socket file from a crash (Unix): remove and retry once.
    QLocalServer::removeServer(serverName());
    return m_server.listen(serverName());
}
