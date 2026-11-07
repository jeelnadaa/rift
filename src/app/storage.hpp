// SQLite storage (PLAN 6) through QtSql. WAL mode, versioned migrations, used from the UI thread only.
#pragma once
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

struct HttpRecord {
    qint64 id = 0;
    QString url, finalUrl, dir, fileName, etag, lastModified, doneRanges, category, referer, error, checksum;
    QStringList tags;
    qint64 size = -1;
    bool resumable = false;
    QString state = "queued"; // queued, paused, completed, error
    qint64 addedAt = 0, completedAt = 0;
    int connections = 0;      // 0 = default from settings
    qint64 speedLimit = 0;
};

struct TorrentRecord {
    QString id, name, category, source;
    QStringList tags;
    qint64 addedAt = 0, completedAt = 0;
};

class Storage {
public:
    bool open(const QString &path, QString *error = nullptr);
    void close();

    // HTTP downloads
    qint64 nextHttpId();
    void saveHttp(const HttpRecord &r);
    void saveHttpProgress(qint64 id, const QString &doneRanges, qint64 size, const QString &state, const QString &fileName,
                          const QString &finalUrl, const QString &etag, const QString &lastModified, bool resumable,
                          const QString &error, qint64 completedAt);
    QVector<HttpRecord> loadHttp();
    void removeHttp(qint64 id);

    // Torrents (app-side metadata; libtorrent state is in resume_data)
    void saveTorrent(const TorrentRecord &r);
    QVector<TorrentRecord> loadTorrents();
    void removeTorrent(const QString &id);
    void saveResume(const QString &id, const QByteArray &blob);
    QVector<QPair<QString, QByteArray>> loadResume();

    // Categories/tags
    void setCategory(const QString &id, const QString &category); // id = "t:<hash>" or "h:<n>"
    void setTags(const QString &id, const QStringList &tags);
    QVariantList categories();
    void saveCategory(const QString &name, const QString &savePath);
    void removeCategory(const QString &name);

    // History and stats
    void addHistory(const QString &kind, const QString &name, qint64 size, const QString &source, const QString &path);
    QVariantList history(const QString &query, int limit = 500);
    void clearHistory();
    void addStats(qint64 down, qint64 up);       // added to today's row
    QVariantList stats(int days);                 // [{date, down, up}]

    QByteArray blob(const QString &key);
    void setBlob(const QString &key, const QByteArray &value);

    void begin();
    void commit();

private:
    bool migrate(QString *error);
    QString m_conn;
};
