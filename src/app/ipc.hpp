// Single-instance channel (PLAN 3, 4.15). A local socket (named pipe on Windows) that only the
// current user can open. Messages are one JSON object per line; every request gets one reply line.
// Used by: a second Rift launch (forwards its arguments) and rift-native-host (browser extension).
#pragma once
#include <QJsonObject>
#include <QLocalServer>
#include <QObject>
#include <functional>

class Ipc : public QObject {
    Q_OBJECT
public:
    static QString serverName();
    // Sends `msg` to a running instance. Returns false if none is running.
    static bool sendToRunning(const QJsonObject &msg, QJsonObject *reply = nullptr, int timeoutMs = 3000);

    explicit Ipc(QObject *parent = nullptr);
    bool listen();
    // Handler returns the reply object for a request.
    std::function<QJsonObject(const QJsonObject &)> handler;

private:
    QLocalServer m_server;
};
