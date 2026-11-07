// Controller: the single object QML talks to ("App" in QML). Owns both engines, storage, settings,
// the list model, tray and IPC. Polls the engines on a timer and pushes diffs to the model.
#pragma once
#include "downloadmodel.hpp"
#include "settings.hpp"
#include "storage.hpp"

#include <QHash>
#include <QJsonObject>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVariantList>
#include <memory>

namespace rift::bt { class Engine; }
namespace rift::http { class Engine; }
class Ipc;
class QMenu;
class QAction;

class Controller : public QObject {
    Q_OBJECT
    Q_PROPERTY(Settings *settings READ settings CONSTANT)
    Q_PROPERTY(DownloadFilter *list READ list CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString engineVersions READ engineVersions CONSTANT)
    Q_PROPERTY(bool portable READ portable CONSTANT)
    Q_PROPERTY(QString dataDir READ dataDir CONSTANT)
    Q_PROPERTY(double downSpeed READ downSpeed NOTIFY ticked)
    Q_PROPERTY(double upSpeed READ upSpeed NOTIFY ticked)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY ticked)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY ticked)
    Q_PROPERTY(int downloadingCount READ downloadingCount NOTIFY ticked)
    Q_PROPERTY(double overallProgress READ overallProgress NOTIFY ticked)
    Q_PROPERTY(int dhtNodes READ dhtNodes NOTIFY ticked)
    Q_PROPERTY(int listenPort READ listenPort NOTIFY ticked)
    Q_PROPERTY(QString netState READ netState NOTIFY ticked)
    Q_PROPERTY(qint64 freeBytes READ freeBytes NOTIFY ticked)
    Q_PROPERTY(qint64 sessionDown READ sessionDown NOTIFY ticked)
    Q_PROPERTY(qint64 sessionUp READ sessionUp NOTIFY ticked)
    Q_PROPERTY(QVariantList speedHistory READ speedHistory NOTIFY ticked)
    Q_PROPERTY(QVariantMap counts READ counts NOTIFY countsChanged)
    Q_PROPERTY(QVariantList categories READ categories NOTIFY countsChanged)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY countsChanged)
    Q_PROPERTY(QVariantList sites READ sites NOTIFY countsChanged)
    Q_PROPERTY(QStringList selection READ selection NOTIFY selectionChanged)
    Q_PROPERTY(QString current READ current NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap detail READ detail NOTIFY detailChanged)
    Q_PROPERTY(QVariantList files READ files NOTIFY detailChanged)
    Q_PROPERTY(QVariantList peers READ peers NOTIFY detailChanged)
    Q_PROPERTY(QVariantList trackers READ trackers NOTIFY detailChanged)
    Q_PROPERTY(QVariantList pieces READ pieces NOTIFY detailChanged)
    Q_PROPERTY(QString detailTab READ detailTab WRITE setDetailTab NOTIFY detailTabChanged)
    Q_PROPERTY(QString clipboardOffer READ clipboardOffer NOTIFY clipboardChanged)
    Q_PROPERTY(bool trayAvailable READ trayAvailable CONSTANT)

public:
    Controller(const QString &dataDir, bool portable, QObject *parent = nullptr);
    ~Controller() override;

    bool start(QString *error);   // open storage, start engines, restore downloads
    void shutdown();              // flush everything; called once before quit
    QJsonObject handleIpc(const QJsonObject &msg);
    void handleArguments(const QStringList &args);

    Settings *settings() const { return m_settings; }
    DownloadFilter *list() const { return m_filter; }
    QString version() const;
    QString engineVersions() const;
    bool portable() const { return m_portable; }
    QString dataDir() const { return m_dataDir; }
    double downSpeed() const { return m_down; }
    double upSpeed() const { return m_up; }
    int activeCount() const { return m_active; }
    int totalCount() const { return static_cast<int>(m_model.rows().size()); }
    int downloadingCount() const { return m_downloading; }
    double overallProgress() const { return m_overall; }
    int dhtNodes() const { return m_dht; }
    int listenPort() const { return m_port; }
    QString netState() const { return m_netState; }
    qint64 freeBytes() const { return m_free; }
    qint64 sessionDown() const { return m_sessionDown; }
    qint64 sessionUp() const { return m_sessionUp; }
    QVariantList speedHistory() const { return m_history; }
    QVariantMap counts() const { return m_counts; }
    QVariantList categories() const { return m_categories; }
    QVariantList tags() const { return m_tags; }
    QVariantList sites() const { return m_sites; }
    QStringList selection() const { return m_selection; }
    QString current() const { return m_current; }
    QVariantMap detail() const { return m_detail; }
    QVariantList files() const { return m_files; }
    QVariantList peers() const { return m_peers; }
    QVariantList trackers() const { return m_trackers; }
    QVariantList pieces() const { return m_pieces; }
    QString detailTab() const { return m_detailTab; }
    void setDetailTab(const QString &t);
    QString clipboardOffer() const { return m_clipOffer; }
    bool trayAvailable() const;

    // ---- Adding
    Q_INVOKABLE int addLinks(const QString &text, const QVariantMap &opts);
    Q_INVOKABLE QVariantMap inspectTorrent(const QString &pathOrUrl);
    Q_INVOKABLE bool addTorrentFile(const QString &pathOrUrl, const QVariantMap &opts);
    Q_INVOKABLE QVariantMap classify(const QString &text); // {magnets, urls, torrents, invalid, names[]}
    Q_INVOKABLE QString defaultFolderFor(const QString &category) const;

    // ---- Actions on ids
    Q_INVOKABLE void pause(const QStringList &ids);
    Q_INVOKABLE void resume(const QStringList &ids);
    Q_INVOKABLE void togglePause(const QStringList &ids);
    Q_INVOKABLE void forceStart(const QStringList &ids);
    Q_INVOKABLE void remove(const QStringList &ids, bool deleteFiles);
    Q_INVOKABLE void undoRemove();
    Q_INVOKABLE void recheck(const QStringList &ids);
    Q_INVOKABLE void reannounce(const QStringList &ids);
    Q_INVOKABLE void restart(const QStringList &ids);
    Q_INVOKABLE void queueMove(const QStringList &ids, const QString &op); // up, down, top, bottom
    Q_INVOKABLE void setSequential(const QStringList &ids, bool on);
    Q_INVOKABLE void setFirstLast(const QStringList &ids, bool on);
    Q_INVOKABLE void setFilePriority(const QString &id, int index, int priority);
    Q_INVOKABLE void setCategory(const QStringList &ids, const QString &category);
    Q_INVOKABLE void toggleTag(const QStringList &ids, const QString &tag);
    Q_INVOKABLE void setLimits(const QStringList &ids, int downKB, int upKB);
    Q_INVOKABLE void setConnections(const QStringList &ids, int n);
    Q_INVOKABLE void moveTo(const QStringList &ids, const QString &folder);
    Q_INVOKABLE void addTracker(const QString &id, const QString &url);
    Q_INVOKABLE void openFolder(const QString &id);
    Q_INVOKABLE void openFile(const QString &id);
    Q_INVOKABLE void openFileAt(const QString &id, const QString &relativePath);
    Q_INVOKABLE QString linkFor(const QString &id) const; // magnet or URL
    Q_INVOKABLE void copyLinks(const QStringList &ids);
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE bool exportTorrent(const QString &id, const QString &path);
    Q_INVOKABLE void verifyChecksum(const QString &id, const QString &expected);
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();
    Q_INVOKABLE void setAltSpeed(bool on);

    // ---- Selection
    Q_INVOKABLE void select(const QStringList &ids, const QString &current);

    // ---- Misc
    Q_INVOKABLE QString fmtBytes(double bytes) const;
    Q_INVOKABLE QString fmtSpeed(double bps) const;
    Q_INVOKABLE QString fmtEta(double seconds) const;
    Q_INVOKABLE QString fmtDate(double ms) const;
    Q_INVOKABLE QString fmtRelative(double ms) const;
    Q_INVOKABLE QString urlToPath(const QUrl &url) const;
    Q_INVOKABLE QUrl pathToUrl(const QString &path) const;
    Q_INVOKABLE void checkClipboard();
    Q_INVOKABLE void dismissClipboard();
    Q_INVOKABLE bool registerHandlers();
    Q_INVOKABLE bool isDefaultHandler() const;
    Q_INVOKABLE QVariantList history(const QString &query);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE QVariantList dailyStats(int days);
    Q_INVOKABLE void saveCategory(const QString &name, const QString &savePath);
    Q_INVOKABLE void removeCategory(const QString &name);
    Q_INVOKABLE QString diagnostics() const;
    Q_INVOKABLE void quit();
    Q_INVOKABLE void notify(const QString &title, const QString &body);

signals:
    void ticked();
    void countsChanged();
    void selectionChanged();
    void detailChanged();
    void detailTabChanged();
    void clipboardChanged();
    void toast(const QString &text, const QString &kind, const QString &action);
    void requestAdd(const QString &text, const QVariantMap &meta);   // open the add dialog prefilled
    void requestAddTorrent(const QString &path);                     // open the torrent dialog
    void showWindow();
    void quitRequested();

private:
    void tick();
    void applySettings(const QString &key = {});
    void applyBtSettings();
    void applyHttpSettings();
    void arbitrate();
    void persistHttp(bool force);
    void refreshDetail();
    void rebuildCounts();
    void handleBtEvents();
    void handleHttpTransitions();
    void setupTray();
    void updateTray();
    qint64 addHttp(const QString &url, const QVariantMap &opts);
    bool addMagnet(const QString &magnet, const QVariantMap &opts);
    bool addTorrentData(const QByteArray &data, const QVariantMap &opts, const QString &source);
    void fetchTorrentUrl(const QString &url, const QVariantMap &opts);
    QString savePathFor(const QVariantMap &opts, const QString &category) const;
    QStringList btIds(const QStringList &ids) const;
    QList<qint64> httpIds(const QStringList &ids) const;
    const Row *row(const QString &id) const { return m_model.find(id); }

    QString m_dataDir;
    bool m_portable = false;
    Settings *m_settings = nullptr;
    Storage m_store;
    std::unique_ptr<rift::bt::Engine> m_bt;
    std::unique_ptr<rift::http::Engine> m_http;
    DownloadModel m_model;
    DownloadFilter *m_filter = nullptr;
    QTimer m_timer;
    QNetworkAccessManager m_net;

    QHash<qint64, HttpRecord> m_httpRec;            // metadata for every HTTP download
    QHash<QString, TorrentRecord> m_btRec;          // metadata for torrents, by hash
    QHash<qint64, QString> m_httpPrevState;
    QHash<QString, QString> m_btPrevState;
    QHash<QString, QString> m_btPending;            // hash → name while being added
    QMutex m_resumeMutex;
    QHash<QString, QByteArray> m_resumePending;     // filled on the libtorrent alert thread

    // Undo for removal (8 s)
    QVector<HttpRecord> m_undoHttp;
    QVector<QPair<TorrentRecord, QByteArray>> m_undoBt;

    double m_down = 0, m_up = 0, m_btDown = 0, m_httpDown = 0, m_overall = 0;
    int m_active = 0, m_downloading = 0, m_dht = 0, m_port = 0;
    QString m_netState = QStringLiteral("starting");
    qint64 m_free = -1, m_sessionDown = 0, m_sessionUp = 0;
    qint64 m_statDown = 0, m_statUp = 0;
    QVariantList m_history;
    QVariantMap m_counts;
    QVariantList m_categories, m_tags, m_sites;
    QStringList m_selection;
    QString m_current;
    QVariantMap m_detail;
    QVariantList m_files, m_peers, m_trackers, m_pieces;
    QString m_detailTab = QStringLiteral("overview");
    QString m_clipOffer, m_clipSeen;
    qint64 m_lastPersist = 0, m_lastResumeSave = 0, m_lastStats = 0, m_lastFree = 0, m_tickCount = 0;
    int64_t m_btLimitApplied = -1, m_httpLimitApplied = -1;

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_altAction = nullptr;
};
