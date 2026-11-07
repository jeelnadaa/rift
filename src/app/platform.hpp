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
// Windows 10/11: dark or light native title bar for every top-level window. No-op elsewhere (macOS and Linux
// title bars follow the system / window manager).
void setDarkTitleBar(bool dark);
// Font setup that must happen before the Q*Application exists. Windows (Qt < 6.12): selects the FreeType font
// engine, which renders our variable fonts' weights. Linux: hands the bundled fonts (:/fonts) to fontconfig.
// Returns true when the system font list now includes them, so installAppFonts(true) must not add them again.
bool registerAppFontsWithSystem();

// Self-update. The release asset kind this build can replace itself from (core::updateAssetName targets):
// "windows-setup" for an installed Windows build, "macos-arm64"/"macos-x64" for a writable Rift.app,
// "linux-appimage" for a writable AppImage, or "" (portable zip, deb/rpm, tarball, a build run in place).
QString updateTarget(bool portable);
// Starts a detached helper that waits for this process to exit, installs `file` (already verified by the
// caller) and, if `relaunch`, starts Rift again with `relaunchArgs`. False if the helper could not start.
bool launchUpdate(const QString &target, const QString &file, bool relaunch, const QStringList &relaunchArgs);

} // namespace platform
