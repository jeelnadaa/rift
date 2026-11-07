// In-app updates from GitHub Releases. Checks /releases/latest at startup and every 6 hours, downloads the
// asset this build installs from (core::updateAssetName), verifies it against the release's SHA256SUMS and
// hands it to platform::launchUpdate once Rift exits: on "Restart now", on quit, or (mode "auto") on its own
// while Rift sits in the tray with nothing downloading. Builds that can't replace themselves (portable,
// deb/rpm, dev) only say a new version exists and link to the release page.
#pragma once
#include <QCryptographicHash>
#include <QFile>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <functional>

class Settings;
class QNetworkReply;

class Updater : public QObject {
    Q_OBJECT
    // idle | checking | uptodate | available | downloading | ready | error | dev
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(QString latest READ latest NOTIFY changed)       // "1.4.0", empty until a check finds one
    Q_PROPERTY(QString notes READ notes NOTIFY changed)         // release notes, trimmed
    Q_PROPERTY(QString pageUrl READ pageUrl NOTIFY changed)     // the release page
    Q_PROPERTY(double progress READ progress NOTIFY changed)    // 0..1 while downloading
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(double lastChecked READ lastChecked NOTIFY changed) // ms since epoch, 0 = never
    Q_PROPERTY(bool dismissed READ dismissed NOTIFY changed)    // banner hidden for this session
    Q_PROPERTY(bool canInstall READ canInstall CONSTANT)
    Q_PROPERTY(QString installNote READ installNote CONSTANT)   // why it can't, when it can't

public:
    Updater(const QString &version, bool portable, Settings *settings, const QString &workDir, QObject *parent = nullptr);

    // Starts the automatic checks (unless the "updateMode" setting is "off"). Not called by the smoke test.
    void start();
    // Asked before a quiet install: true when nothing is downloading and no Rift window is on screen.
    std::function<bool()> idleProbe;
    // Called by main() after Controller::shutdown(): starts the installer if an update is ready.
    bool applyOnExit();

    QString state() const { return m_state; }
    QString latest() const { return m_latest; }
    QString notes() const { return m_notes; }
    QString pageUrl() const { return m_pageUrl; }
    double progress() const { return m_progress; }
    QString error() const { return m_error; }
    double lastChecked() const { return static_cast<double>(m_lastChecked); }
    bool dismissed() const { return m_dismissed; }
    bool canInstall() const { return m_release && !m_target.isEmpty(); }
    QString installNote() const;

    Q_INVOKABLE void check();       // manual "Check now"
    Q_INVOKABLE void download();    // "Update" (notify mode, or retry after an error)
    Q_INVOKABLE void restartNow();  // quit, install, start again
    Q_INVOKABLE void dismiss();

signals:
    void changed();
    void restartRequested();        // Controller turns this into quitRequested
    void readyToInstall(const QString &version);

private:
    void checkNow(bool manual);
    void onReleaseJson(const QByteArray &body, bool manual);
    void fetchSums();
    void startAssetDownload(const QByteArray &expected);
    void fail(const QString &message);
    void setState(const QString &s);
    void maybeApplyQuietly();
    QString mode() const;
    QNetworkRequest request(const QUrl &url) const;

    QString m_version;
    bool m_portable = false;
    bool m_release = false;
    QString m_target;
    Settings *m_settings = nullptr;
    QString m_dir;
    QNetworkAccessManager m_net;
    QPointer<QNetworkReply> m_reply;
    QTimer m_checkTimer, m_applyTimer;

    QString m_state = QStringLiteral("idle");
    QString m_latest, m_notes, m_pageUrl, m_error;
    QString m_assetName, m_assetUrl, m_sumsUrl, m_file;
    QByteArray m_expected;
    double m_progress = 0;
    qint64 m_lastChecked = 0;
    bool m_dismissed = false;
    bool m_relaunch = false;
    QStringList m_relaunchArgs;
    QFile m_out;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
};
