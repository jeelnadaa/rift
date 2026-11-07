#include "updater.hpp"

#include "core/update.hpp"
#include "platform.hpp"
#include "settings.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>

namespace {
constexpr int kCheckEveryMs = 6 * 60 * 60 * 1000;
constexpr int kFirstCheckMs = 20 * 1000;      // let startup (restoring torrents) settle first
constexpr int kQuietApplyEveryMs = 5 * 60 * 1000;
} // namespace

Updater::Updater(const QString &version, bool portable, Settings *settings, const QString &workDir, QObject *parent)
    : QObject(parent), m_version(version), m_portable(portable), m_settings(settings), m_dir(workDir) {
    // Only CI release builds (x.y.z, not 0.0.0) update; isNewerVersion is false for anything else.
    m_release = rift::core::isNewerVersion("999999.0.0", version.toStdString());
    m_target = platform::updateTarget(portable);
    m_net.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_pageUrl = QStringLiteral("https://github.com/%1/releases/latest").arg(platform::kGithubRepo);

    // Leftovers from an earlier run (an installed update, or a download cut short).
    QDir d(m_dir);
    for (const QFileInfo &fi : d.entryInfoList(QDir::Files)) QFile::remove(fi.absoluteFilePath());

    m_checkTimer.setSingleShot(true);
    connect(&m_checkTimer, &QTimer::timeout, this, [this] {
        if (mode() != QLatin1String("off")) checkNow(false);
        m_checkTimer.start(kCheckEveryMs);
    });
    m_applyTimer.setInterval(kQuietApplyEveryMs);
    connect(&m_applyTimer, &QTimer::timeout, this, &Updater::maybeApplyQuietly);
    if (!m_release) m_state = QStringLiteral("dev");
}

void Updater::start() {
    if (!m_release) return;
    m_checkTimer.start(kFirstCheckMs);
    connect(m_settings, &Settings::changed, this, [this](const QString &key) {
        if (key != QLatin1String("updateMode")) return;
        // Turning updates on (or to auto) checks soon instead of waiting for the next 6-hour tick.
        if (mode() != QLatin1String("off") && m_state != QLatin1String("checking") && m_state != QLatin1String("downloading"))
            m_checkTimer.start(2000);
        if (mode() == QLatin1String("auto") && m_state == QLatin1String("available") && canInstall()) download();
    });
}

QString Updater::mode() const {
    const QString m = m_settings ? m_settings->s(QStringLiteral("updateMode")) : QString();
    return m == QLatin1String("notify") || m == QLatin1String("off") ? m : QStringLiteral("auto");
}

QString Updater::installNote() const {
    if (!m_release) return tr("This is a development build, so it doesn't update itself.");
    if (m_portable) return tr("Portable copy: get the new version from the release page and replace this folder.");
    if (m_target.isEmpty())
        return tr("This copy can't replace itself (installed by a package manager, or run from a read-only place). "
                  "Get new versions from the release page.");
    return {};
}

QNetworkRequest Updater::request(const QUrl &url) const {
    QNetworkRequest r(url);
    r.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Rift/%1").arg(m_version));
    r.setTransferTimeout(60 * 1000); // inactivity
    return r;
}

void Updater::setState(const QString &s) {
    m_state = s;
    emit changed();
}

void Updater::fail(const QString &message) {
    m_error = message;
    m_progress = 0;
    if (m_out.isOpen()) {
        m_out.close();
        m_out.remove();
    }
    setState(QStringLiteral("error"));
}

void Updater::check() {
    if (!m_release) {
        setState(QStringLiteral("dev"));
        return;
    }
    checkNow(true);
}

void Updater::checkNow(bool manual) {
    // Never interrupt a download, and don't throw away a verified installer.
    if (m_state == QLatin1String("checking") || m_state == QLatin1String("downloading")) return;
    if (m_state == QLatin1String("ready")) return; // already holding the newest verified installer
    setState(QStringLiteral("checking"));
    QNetworkRequest r = request(QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(platform::kGithubRepo)));
    r.setRawHeader("Accept", "application/vnd.github+json");
    r.setTransferTimeout(30 * 1000);
    m_reply = m_net.get(r);
    connect(m_reply, &QNetworkReply::finished, this, [this, manual, reply = m_reply.data()] {
        reply->deleteLater();
        m_lastChecked = QDateTime::currentMSecsSinceEpoch();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 404) { // no releases yet
            setState(QStringLiteral("uptodate"));
            return;
        }
        if (status == 403 || status == 429) {
            fail(tr("GitHub is limiting update checks right now. Rift will try again later."));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            fail(tr("Couldn't reach GitHub to check for updates."));
            return;
        }
        onReleaseJson(reply->readAll(), manual);
    });
}

void Updater::onReleaseJson(const QByteArray &body, bool manual) {
    const QJsonObject rel = QJsonDocument::fromJson(body).object();
    const QString tag = rel.value(QStringLiteral("tag_name")).toString();
    const auto parsed = rift::core::parseVersion(tag.toStdString());
    if (!parsed || rel.value(QStringLiteral("draft")).toBool() || rel.value(QStringLiteral("prerelease")).toBool()) {
        fail(tr("The latest release on GitHub doesn't look like a Rift release."));
        return;
    }
    const QString v = QStringLiteral("%1.%2.%3").arg((*parsed)[0]).arg((*parsed)[1]).arg((*parsed)[2]);
    if (!rift::core::isNewerVersion(v.toStdString(), m_version.toStdString())) {
        m_latest = v;
        m_error.clear();
        setState(QStringLiteral("uptodate"));
        return;
    }
    const bool fresh = v != m_latest;
    m_latest = v;
    m_error.clear();
    if (fresh) m_dismissed = false;
    const QString page = rel.value(QStringLiteral("html_url")).toString();
    if (page.startsWith(QLatin1String("https://github.com/"))) m_pageUrl = page;
    QString notes = rel.value(QStringLiteral("body")).toString().trimmed();
    if (notes.size() > 800) notes = notes.left(800).trimmed() + QStringLiteral("…");
    m_notes = notes;

    m_assetName = QString::fromStdString(rift::core::updateAssetName(v.toStdString(), m_target.toStdString()));
    m_assetUrl.clear();
    m_sumsUrl.clear();
    for (const QJsonValue &a : rel.value(QStringLiteral("assets")).toArray()) {
        const QJsonObject o = a.toObject();
        const QString name = o.value(QStringLiteral("name")).toString();
        const QString url = o.value(QStringLiteral("browser_download_url")).toString();
        if (!url.startsWith(QLatin1String("https://"))) continue;
        if (name == QLatin1String("SHA256SUMS")) m_sumsUrl = url;
        else if (!m_assetName.isEmpty() && name == m_assetName) m_assetUrl = url;
    }
    setState(QStringLiteral("available"));
    // Auto mode fetches it right away (also after a manual check); notify mode waits for "Update".
    Q_UNUSED(manual);
    if (canInstall() && !m_assetUrl.isEmpty() && mode() == QLatin1String("auto")) download();
}

void Updater::download() {
    if (m_state == QLatin1String("downloading") || m_state == QLatin1String("ready")) return;
    if (!canInstall()) return;
    if (m_assetUrl.isEmpty() || m_sumsUrl.isEmpty()) {
        if (m_latest.isEmpty()) {
            checkNow(true);
            return;
        }
        fail(tr("Rift %1 has no file for this system yet. Try again later, or use the release page.").arg(m_latest));
        return;
    }
    m_progress = 0;
    setState(QStringLiteral("downloading"));
    fetchSums();
}

void Updater::fetchSums() {
    m_reply = m_net.get(request(QUrl(m_sumsUrl)));
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply.data()] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            fail(tr("Couldn't download the update's checksums."));
            return;
        }
        const std::string sums = reply->read(1 << 20).toStdString();
        const QByteArray expected = QByteArray::fromStdString(rift::core::sha256ForFile(sums, m_assetName.toStdString()));
        if (expected.isEmpty()) {
            fail(tr("The release doesn't list a checksum for %1, so Rift won't install it.").arg(m_assetName));
            return;
        }
        startAssetDownload(expected);
    });
}

void Updater::startAssetDownload(const QByteArray &expected) {
    m_expected = expected;
    QDir().mkpath(m_dir);
    m_file = QDir(m_dir).filePath(m_assetName);
    const QString part = m_file + QStringLiteral(".part");
    QFile::remove(part);
    m_out.setFileName(part);
    if (!m_out.open(QIODevice::WriteOnly)) {
        fail(tr("Couldn't save the update to %1.").arg(QDir::toNativeSeparators(m_dir)));
        return;
    }
    m_hash.reset();
    m_reply = m_net.get(request(QUrl(m_assetUrl)));
    QNetworkReply *reply = m_reply;
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] {
        const QByteArray chunk = reply->readAll();
        m_hash.addData(chunk);
        if (m_out.write(chunk) != chunk.size()) {
            reply->abort();
            fail(tr("Couldn't write the update to disk (is it full?)."));
        }
    });
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 got, qint64 total) {
        if (total <= 0 || m_state != QLatin1String("downloading")) return;
        const double p = static_cast<double>(got) / static_cast<double>(total);
        if (p - m_progress >= 0.01 || p >= 1.0) {
            m_progress = p;
            emit changed();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, part] {
        reply->deleteLater();
        if (m_state != QLatin1String("downloading")) return; // already failed (write error)
        if (reply->error() != QNetworkReply::NoError) {
            fail(tr("The update download stopped: %1").arg(reply->errorString()));
            return;
        }
        const QByteArray rest = reply->readAll();
        m_hash.addData(rest);
        m_out.write(rest);
        m_out.close();
        if (m_hash.result().toHex() != m_expected) {
            QFile::remove(part);
            fail(tr("The downloaded update didn't match its checksum, so Rift deleted it."));
            return;
        }
        QFile::remove(m_file);
        if (!QFile::rename(part, m_file)) {
            QFile::remove(part);
            fail(tr("Couldn't save the update to %1.").arg(QDir::toNativeSeparators(m_dir)));
            return;
        }
        m_progress = 1;
        setState(QStringLiteral("ready"));
        emit readyToInstall(m_latest);
        if (mode() == QLatin1String("auto")) m_applyTimer.start();
    });
}

void Updater::restartNow() {
    if (m_state != QLatin1String("ready")) return;
    m_relaunch = true;
    m_relaunchArgs.clear();
    emit restartRequested();
}

void Updater::maybeApplyQuietly() {
    if (m_state != QLatin1String("ready") || mode() != QLatin1String("auto")) {
        m_applyTimer.stop();
        return;
    }
    if (!idleProbe || !idleProbe()) return;
    // Rift is in the tray doing nothing: update and come back to the tray.
    m_relaunch = true;
    m_relaunchArgs = {QStringLiteral("--minimized")};
    emit restartRequested();
}

bool Updater::applyOnExit() {
    if (m_state != QLatin1String("ready") || m_file.isEmpty() || !QFileInfo::exists(m_file)) return false;
    // Quitting on purpose with mode "notify" or "auto" installs the update the user already has; relaunch only
    // when the quit came from "Restart now" or a quiet tray update.
    const bool ok = platform::launchUpdate(m_target, m_file, m_relaunch, m_relaunchArgs);
    if (!ok) qWarning("rift: could not start the updater for %s", qPrintable(m_file));
    return ok;
}

void Updater::dismiss() {
    m_dismissed = true;
    emit changed();
}
