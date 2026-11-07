// OS-specific helpers. All #ifdef Q_OS_* code lives in platform.cpp (AGENTS.md rule 8).
#pragma once
#include <QString>
#include <QStringList>

namespace platform {

// Chrome/Edge/Brave extension IDs allowed to use the native host, and the Firefox add-on ID.
// Add the store-assigned Chrome Web Store / Edge Add-ons IDs here when the listings exist.
inline const QStringList kChromiumExtensionIds = {QStringLiteral("llhjnffjocckdmklgdcpnllflgcacppo")};
inline const QString kFirefoxExtensionId = QStringLiteral("browser@rift.app");
inline const QString kNativeHostName = QStringLiteral("app.rift.native");
// Where releases (app + browser extension zips) are published, and the website.
inline const QString kGithubRepo = QStringLiteral("jeelnadaa/rift");
inline const QString kWebsite = QStringLiteral("https://rift.solarquack.in");

void preventSleep(bool on);                       // while anything downloads
void revealInFolder(const QString &path);         // select the file in Explorer/Finder, or open its folder
void openPath(const QString &path);               // open file with the default app
qint64 freeSpace(const QString &path);            // bytes, -1 if unknown
QString nativeHostPath();                         // rift-native-host next to the executable
// Writes the native-messaging manifests for every supported browser (per user). Idempotent.
void registerNativeHost();
// magnet: and .torrent handlers for the current user (Windows registry, Linux xdg-mime). macOS uses Info.plist.
bool registerHandlers();
bool isDefaultMagnetHandler();
void setLaunchAtLogin(bool on);

} // namespace platform
