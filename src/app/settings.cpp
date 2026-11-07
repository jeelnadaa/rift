#include "settings.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>

QVariantMap Settings::defaults() {
    const QString dl = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return {
        // General
        {"theme", "system"}, {"accent", "cobalt"}, {"density", "compact"}, {"reducedMotion", false},
        {"closeToTray", true}, {"startMinimized", false}, {"launchAtLogin", false}, {"confirmExit", false},
        {"clipboardWatch", true}, {"notifications", true}, {"notifyErrors", true}, {"preventSleep", true},
        {"speedUnits", "bytes"}, {"binaryUnits", false}, {"showSpeedInTitle", true},
        // Downloads
        {"savePath", dl}, {"incompletePath", ""}, {"useIncompletePath", false}, {"startPaused", false},
        {"sequentialDefault", false}, {"deleteTorrentAfterAdd", false}, {"pickFiles", true}, {"updateMode", QStringLiteral("auto")}, {"categoryFolders", false},
        {"minFreeSpaceGB", 2},
        // Connection
        {"listenPort", 0}, {"upnp", true}, {"natpmp", true}, {"maxConnections", 500},
        {"maxConnectionsPerTorrent", 100}, {"maxUploads", 20}, {"proxyType", 0}, {"proxyHost", ""},
        {"proxyPort", 1080}, {"networkInterface", ""},
        // Speed (KB/s; 0 = unlimited)
        {"downLimit", 0}, {"upLimit", 0}, {"altDownLimit", 1000}, {"altUpLimit", 200}, {"altSpeed", false},
        // BitTorrent
        {"dht", true}, {"pex", true}, {"lsd", true}, {"encryption", 0}, {"anonymous", false},
        {"queueing", true}, {"activeDownloads", 5}, {"activeSeeds", 5}, {"activeLimit", 15},
        {"seedRatioLimit", 0.0}, {"seedRatioAction", "pause"},
        // HTTP downloader
        {"httpConnections", 8}, {"httpMaxActive", 4}, {"httpMinSegmentMB", 1}, {"httpStallSeconds", 15},
        {"httpRetries", 12}, {"httpUserAgent", ""}, {"httpVerifyTls", true},
        // Browser integration
        {"browserEnabled", true}, {"browserTakeOver", true},
        {"browserTypes", "zip,rar,7z,tar,gz,xz,iso,img,exe,msi,dmg,pkg,deb,rpm,appimage,apk,mp4,mkv,avi,mov,webm,mp3,flac,wav,pdf,epub,torrent"},
        {"browserMinMB", 1}, {"browserConfirm", true},
        // Layout state
        {"sidebarVisible", true}, {"detailsVisible", true}, {"detailsHeight", 320}, {"sidebarWidth", 264},
        {"winW", 1320}, {"winH", 860}, {"winX", -1}, {"winY", -1}, {"winMax", false}, {"lastFilter", "all"},
        {"sortKey", "added"}, {"sortAsc", false}, {"firstRun", true},
        {"addConnections", 0}, {"lastCategory", ""},
    };
}

Settings::Settings(const QString &iniPath, QObject *parent)
    : QQmlPropertyMap(this, parent), m_store(iniPath, QSettings::IniFormat) {
    const QVariantMap def = defaults();
    for (auto it = def.cbegin(); it != def.cend(); ++it) {
        QVariant v = m_store.value(it.key(), it.value());
        // INI loses types: coerce to the default's type.
        if (it.value().typeId() != QMetaType::QString && v.typeId() == QMetaType::QString) v.convert(it.value().metaType());
        insert(it.key(), v);
    }
    if (value("listenPort").toInt() <= 0) {
        // Random port once, then keep it so port forwarding keeps working.
        const int port = static_cast<int>(QRandomGenerator::global()->bounded(20000, 60000));
        insert("listenPort", port);
        m_store.setValue("listenPort", port);
    }
}

QVariant Settings::updateValue(const QString &key, const QVariant &input) {
    QVariant v = input;
    const QVariant def = defaults().value(key);
    if (def.isValid() && def.typeId() != v.typeId()) v.convert(def.metaType());
    m_store.setValue(key, v);
    // Emit after QQmlPropertyMap stores the value.
    QMetaObject::invokeMethod(this, [this, key] { emit changed(key); }, Qt::QueuedConnection);
    return v;
}

void Settings::set(const QString &key, const QVariant &v) {
    const QVariant nv = updateValue(key, v);
    insert(key, nv);
}

void Settings::reset(const QString &key) { set(key, defaults().value(key)); }

void Settings::resetAll() {
    const QVariantMap def = defaults();
    for (auto it = def.cbegin(); it != def.cend(); ++it)
        if (it.key() != "listenPort") set(it.key(), it.value());
}

QString Settings::exportJson() const {
    QJsonObject o;
    for (const QString &k : keys()) o.insert(k, QJsonValue::fromVariant(value(k)));
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented));
}

bool Settings::importJson(const QString &json) {
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) return false;
    const QVariantMap def = defaults();
    const QJsonObject o = doc.object();
    for (auto it = o.begin(); it != o.end(); ++it)
        if (def.contains(it.key())) set(it.key(), it.value().toVariant());
    return true;
}
