#include "storage.hpp"

#include <QDate>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
QSqlDatabase db(const QString &c) { return QSqlDatabase::database(c); }

bool exec(QSqlQuery &q, QString *err = nullptr) {
    if (q.exec()) return true;
    if (err) *err = q.lastError().text();
    qWarning("rift.storage: %s", qPrintable(q.lastError().text()));
    return false;
}
} // namespace

bool Storage::open(const QString &path, QString *error) {
    m_conn = QStringLiteral("rift-main");
    QSqlDatabase d = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_conn);
    d.setDatabaseName(path);
    if (!d.open()) {
        if (error) *error = d.lastError().text();
        return false;
    }
    QSqlQuery q(d);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    return migrate(error);
}

void Storage::close() {
    if (m_conn.isEmpty()) return;
    {
        QSqlDatabase d = db(m_conn);
        if (d.isOpen()) d.close();
    }
    QSqlDatabase::removeDatabase(m_conn);
    m_conn.clear();
}

bool Storage::migrate(QString *error) {
    QSqlDatabase d = db(m_conn);
    QSqlQuery q(d);
    q.exec(QStringLiteral("PRAGMA user_version"));
    int version = q.next() ? q.value(0).toInt() : 0;
    const QStringList v1 = {
        "CREATE TABLE IF NOT EXISTS http_downloads (id INTEGER PRIMARY KEY, url TEXT NOT NULL, final_url TEXT, dir TEXT,"
        " file_name TEXT, size INTEGER DEFAULT -1, etag TEXT, last_modified TEXT, done_ranges TEXT, resumable INTEGER DEFAULT 0,"
        " state TEXT, category TEXT, tags TEXT, referer TEXT, error TEXT, checksum TEXT, added_at INTEGER, completed_at INTEGER,"
        " connections INTEGER DEFAULT 0, speed_limit INTEGER DEFAULT 0)",
        "CREATE TABLE IF NOT EXISTS torrents (id TEXT PRIMARY KEY, name TEXT, category TEXT, tags TEXT, source TEXT,"
        " added_at INTEGER, completed_at INTEGER)",
        "CREATE TABLE IF NOT EXISTS resume_data (id TEXT PRIMARY KEY, blob BLOB, updated_at INTEGER)",
        "CREATE TABLE IF NOT EXISTS categories (name TEXT PRIMARY KEY, save_path TEXT)",
        "CREATE TABLE IF NOT EXISTS history (id INTEGER PRIMARY KEY AUTOINCREMENT, kind TEXT, name TEXT, size INTEGER,"
        " source TEXT, path TEXT, completed_at INTEGER)",
        "CREATE INDEX IF NOT EXISTS history_name ON history(name)",
        "CREATE TABLE IF NOT EXISTS stats_daily (date TEXT PRIMARY KEY, down INTEGER DEFAULT 0, up INTEGER DEFAULT 0)",
        "CREATE TABLE IF NOT EXISTS blobs (key TEXT PRIMARY KEY, value BLOB)",
        "INSERT OR IGNORE INTO categories(name, save_path) VALUES ('Video',''),('Music',''),('Programs',''),"
        "('Archives',''),('Documents',''),('Disc images','')",
    };
    if (version < 1) {
        d.transaction();
        for (const QString &sql : v1) {
            QSqlQuery m(d);
            m.prepare(sql);
            if (!exec(m, error)) { d.rollback(); return false; }
        }
        QSqlQuery m(d);
        m.exec(QStringLiteral("PRAGMA user_version=1"));
        d.commit();
        version = 1;
    }
    return true;
}

void Storage::begin() { db(m_conn).transaction(); }
void Storage::commit() { db(m_conn).commit(); }

qint64 Storage::nextHttpId() {
    QSqlQuery q(db(m_conn));
    q.exec(QStringLiteral("SELECT COALESCE(MAX(id),0)+1 FROM http_downloads"));
    return q.next() ? q.value(0).toLongLong() : 1;
}

void Storage::saveHttp(const HttpRecord &r) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO http_downloads(id,url,final_url,dir,file_name,size,etag,last_modified,done_ranges,resumable,"
        "state,category,tags,referer,error,checksum,added_at,completed_at,connections,speed_limit) VALUES"
        "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
    q.addBindValue(r.id); q.addBindValue(r.url); q.addBindValue(r.finalUrl); q.addBindValue(r.dir);
    q.addBindValue(r.fileName); q.addBindValue(r.size); q.addBindValue(r.etag); q.addBindValue(r.lastModified);
    q.addBindValue(r.doneRanges); q.addBindValue(r.resumable ? 1 : 0); q.addBindValue(r.state); q.addBindValue(r.category);
    q.addBindValue(r.tags.join(QLatin1Char(','))); q.addBindValue(r.referer); q.addBindValue(r.error);
    q.addBindValue(r.checksum); q.addBindValue(r.addedAt); q.addBindValue(r.completedAt); q.addBindValue(r.connections);
    q.addBindValue(r.speedLimit);
    exec(q);
}

void Storage::saveHttpProgress(qint64 id, const QString &doneRanges, qint64 size, const QString &state,
                               const QString &fileName, const QString &finalUrl, const QString &etag,
                               const QString &lastModified, bool resumable, const QString &error, qint64 completedAt) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("UPDATE http_downloads SET done_ranges=?, size=?, state=?, file_name=?, final_url=?, etag=?,"
                             " last_modified=?, resumable=?, error=?, completed_at=? WHERE id=?"));
    q.addBindValue(doneRanges); q.addBindValue(size); q.addBindValue(state); q.addBindValue(fileName);
    q.addBindValue(finalUrl); q.addBindValue(etag); q.addBindValue(lastModified); q.addBindValue(resumable ? 1 : 0);
    q.addBindValue(error); q.addBindValue(completedAt); q.addBindValue(id);
    exec(q);
}

QVector<HttpRecord> Storage::loadHttp() {
    QVector<HttpRecord> out;
    QSqlQuery q(db(m_conn));
    q.exec(QStringLiteral("SELECT id,url,final_url,dir,file_name,size,etag,last_modified,done_ranges,resumable,state,category,"
                          "tags,referer,error,checksum,added_at,completed_at,connections,speed_limit FROM http_downloads ORDER BY id"));
    while (q.next()) {
        HttpRecord r;
        r.id = q.value(0).toLongLong(); r.url = q.value(1).toString(); r.finalUrl = q.value(2).toString();
        r.dir = q.value(3).toString(); r.fileName = q.value(4).toString(); r.size = q.value(5).toLongLong();
        r.etag = q.value(6).toString(); r.lastModified = q.value(7).toString(); r.doneRanges = q.value(8).toString();
        r.resumable = q.value(9).toInt() != 0; r.state = q.value(10).toString(); r.category = q.value(11).toString();
        r.tags = q.value(12).toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
        r.referer = q.value(13).toString(); r.error = q.value(14).toString(); r.checksum = q.value(15).toString();
        r.addedAt = q.value(16).toLongLong(); r.completedAt = q.value(17).toLongLong();
        r.connections = q.value(18).toInt(); r.speedLimit = q.value(19).toLongLong();
        out.push_back(r);
    }
    return out;
}

void Storage::removeHttp(qint64 id) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("DELETE FROM http_downloads WHERE id=?"));
    q.addBindValue(id);
    exec(q);
}

void Storage::saveTorrent(const TorrentRecord &r) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("INSERT INTO torrents(id,name,category,tags,source,added_at,completed_at) VALUES(?,?,?,?,?,?,?)"
                             " ON CONFLICT(id) DO UPDATE SET name=excluded.name, completed_at=MAX(COALESCE(torrents.completed_at,0),"
                             " COALESCE(excluded.completed_at,0))"));
    q.addBindValue(r.id); q.addBindValue(r.name); q.addBindValue(r.category);
    q.addBindValue(r.tags.join(QLatin1Char(','))); q.addBindValue(r.source); q.addBindValue(r.addedAt);
    q.addBindValue(r.completedAt);
    exec(q);
}

QVector<TorrentRecord> Storage::loadTorrents() {
    QVector<TorrentRecord> out;
    QSqlQuery q(db(m_conn));
    q.exec(QStringLiteral("SELECT id,name,category,tags,source,added_at,completed_at FROM torrents"));
    while (q.next()) {
        TorrentRecord r;
        r.id = q.value(0).toString(); r.name = q.value(1).toString(); r.category = q.value(2).toString();
        r.tags = q.value(3).toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
        r.source = q.value(4).toString(); r.addedAt = q.value(5).toLongLong(); r.completedAt = q.value(6).toLongLong();
        out.push_back(r);
    }
    return out;
}

void Storage::removeTorrent(const QString &id) {
    for (const QString &sql : {QStringLiteral("DELETE FROM torrents WHERE id=?"), QStringLiteral("DELETE FROM resume_data WHERE id=?")}) {
        QSqlQuery q(db(m_conn));
        q.prepare(sql);
        q.addBindValue(id);
        exec(q);
    }
}

void Storage::saveResume(const QString &id, const QByteArray &blob) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("INSERT OR REPLACE INTO resume_data(id,blob,updated_at) VALUES(?,?,?)"));
    q.addBindValue(id); q.addBindValue(blob); q.addBindValue(QDateTime::currentSecsSinceEpoch());
    exec(q);
}

QVector<QPair<QString, QByteArray>> Storage::loadResume() {
    QVector<QPair<QString, QByteArray>> out;
    QSqlQuery q(db(m_conn));
    q.exec(QStringLiteral("SELECT id, blob FROM resume_data"));
    while (q.next()) out.push_back({q.value(0).toString(), q.value(1).toByteArray()});
    return out;
}

void Storage::setCategory(const QString &id, const QString &category) {
    QSqlQuery q(db(m_conn));
    if (id.startsWith(QLatin1String("h:"))) {
        q.prepare(QStringLiteral("UPDATE http_downloads SET category=? WHERE id=?"));
        q.addBindValue(category); q.addBindValue(id.mid(2).toLongLong());
    } else {
        q.prepare(QStringLiteral("UPDATE torrents SET category=? WHERE id=?"));
        q.addBindValue(category); q.addBindValue(id.mid(2));
    }
    exec(q);
}

void Storage::setTags(const QString &id, const QStringList &tags) {
    QSqlQuery q(db(m_conn));
    if (id.startsWith(QLatin1String("h:"))) {
        q.prepare(QStringLiteral("UPDATE http_downloads SET tags=? WHERE id=?"));
        q.addBindValue(tags.join(QLatin1Char(','))); q.addBindValue(id.mid(2).toLongLong());
    } else {
        q.prepare(QStringLiteral("UPDATE torrents SET tags=? WHERE id=?"));
        q.addBindValue(tags.join(QLatin1Char(','))); q.addBindValue(id.mid(2));
    }
    exec(q);
}

QVariantList Storage::categories() {
    QVariantList out;
    QSqlQuery q(db(m_conn));
    q.exec(QStringLiteral("SELECT name, save_path FROM categories ORDER BY name COLLATE NOCASE"));
    while (q.next()) out.push_back(QVariantMap{{"name", q.value(0)}, {"savePath", q.value(1)}});
    return out;
}

void Storage::saveCategory(const QString &name, const QString &savePath) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("INSERT OR REPLACE INTO categories(name, save_path) VALUES(?,?)"));
    q.addBindValue(name); q.addBindValue(savePath);
    exec(q);
}

void Storage::removeCategory(const QString &name) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("DELETE FROM categories WHERE name=?"));
    q.addBindValue(name);
    exec(q);
}

void Storage::addHistory(const QString &kind, const QString &name, qint64 size, const QString &source, const QString &path) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("INSERT INTO history(kind,name,size,source,path,completed_at) VALUES(?,?,?,?,?,?)"));
    q.addBindValue(kind); q.addBindValue(name); q.addBindValue(size); q.addBindValue(source); q.addBindValue(path);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    exec(q);
}

QVariantList Storage::history(const QString &query, int limit) {
    QVariantList out;
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("SELECT kind,name,size,source,path,completed_at FROM history WHERE name LIKE ? OR source LIKE ?"
                             " ORDER BY completed_at DESC LIMIT ?"));
    const QString like = QLatin1Char('%') + query + QLatin1Char('%');
    q.addBindValue(like); q.addBindValue(like); q.addBindValue(limit);
    exec(q);
    while (q.next())
        out.push_back(QVariantMap{{"kind", q.value(0)}, {"name", q.value(1)}, {"size", q.value(2)},
                                  {"source", q.value(3)}, {"path", q.value(4)}, {"completedAt", q.value(5)}});
    return out;
}

void Storage::clearHistory() {
    QSqlQuery q(db(m_conn));
    q.exec(QStringLiteral("DELETE FROM history"));
}

void Storage::addStats(qint64 down, qint64 up) {
    if (down <= 0 && up <= 0) return;
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("INSERT INTO stats_daily(date,down,up) VALUES(?,?,?) ON CONFLICT(date) DO UPDATE SET"
                             " down=down+excluded.down, up=up+excluded.up"));
    q.addBindValue(QDate::currentDate().toString(Qt::ISODate)); q.addBindValue(down); q.addBindValue(up);
    exec(q);
}

QVariantList Storage::stats(int days) {
    QVariantList out;
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("SELECT date, down, up FROM stats_daily WHERE date >= ? ORDER BY date"));
    q.addBindValue(QDate::currentDate().addDays(-days + 1).toString(Qt::ISODate));
    exec(q);
    while (q.next()) out.push_back(QVariantMap{{"date", q.value(0)}, {"down", q.value(1)}, {"up", q.value(2)}});
    return out;
}

QByteArray Storage::blob(const QString &key) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("SELECT value FROM blobs WHERE key=?"));
    q.addBindValue(key);
    exec(q);
    return q.next() ? q.value(0).toByteArray() : QByteArray();
}

void Storage::setBlob(const QString &key, const QByteArray &value) {
    QSqlQuery q(db(m_conn));
    q.prepare(QStringLiteral("INSERT OR REPLACE INTO blobs(key,value) VALUES(?,?)"));
    q.addBindValue(key); q.addBindValue(value);
    exec(q);
}
