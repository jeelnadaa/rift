// Unified list of torrents and HTTP downloads (PLAN 3, 4.14 "Unified list"). Rows are diffed on
// every refresh so only changed rows repaint. DownloadFilter adds sidebar filters, search and sort.
#pragma once
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QVariantList>
#include <QVector>
#include <QRegularExpression>

struct Row {
    QString id;        // "t:<infohash>" or "h:<n>"
    QString kind;      // "bt" | "http"
    QString name, state, category, site, path, error, source;
    QStringList tags;
    qint64 size = 0, done = 0, added = 0, completed = 0;
    double progress = 0, ratio = 0, availability = 0;
    double down = 0, up = 0;
    double eta = -1;
    int seeds = 0, peers = 0, seedsTotal = -1, peersTotal = -1, conns = 0, queue = -1;
    bool isPrivate = false, sequential = false, resumable = true;
    QVariantList segs; // HTTP: [{s, e, c}] fractions of the file per connection

    bool operator==(const Row &o) const;
};

class DownloadModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1, KindRole, NameRole, StateRole, CategoryRole, SiteRole, PathRole, ErrorRole,
        TagsRole, SizeRole, DoneRole, AddedRole, CompletedRole, ProgressRole, RatioRole, DownRole, UpRole, EtaRole,
        SeedsRole, PeersRole, SeedsTotalRole, PeersTotalRole, ConnsRole, QueueRole, PrivateRole, SequentialRole,
        SegsRole, SourceRole, AvailabilityRole, ResumableRole
    };
    using QAbstractListModel::QAbstractListModel;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void update(QVector<Row> rows);          // diffed replace
    const QVector<Row> &rows() const { return m_rows; }
    const Row *find(const QString &id) const;

private:
    QVector<Row> m_rows;
};

class DownloadFilter : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filterKey WRITE setFilterKey NOTIFY changed)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY changed)
    Q_PROPERTY(bool regex READ regex WRITE setRegex NOTIFY changed)
    Q_PROPERTY(QString sortKey READ sortKey WRITE setSortKey NOTIFY changed)
    Q_PROPERTY(bool ascending READ ascending WRITE setAscending NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool regexValid READ regexValid NOTIFY changed)
public:
    explicit DownloadFilter(QObject *parent = nullptr);
    static bool matches(const Row &r, const QString &filter);

    QString filterKey() const { return m_filter; }
    void setFilterKey(const QString &f);
    QString query() const { return m_query; }
    void setQuery(const QString &q);
    bool regex() const { return m_regex; }
    void setRegex(bool r);
    bool regexValid() const { return m_rx.isValid(); }
    QString sortKey() const { return m_sortKey; }
    void setSortKey(const QString &k);
    bool ascending() const { return m_asc; }
    void setAscending(bool a);
    int count() const { return rowCount(); }

    Q_INVOKABLE QString idAt(int row) const;
    Q_INVOKABLE int rowOf(const QString &id) const;
    Q_INVOKABLE QStringList idsBetween(int a, int b) const;
    Q_INVOKABLE QStringList allIds() const;

signals:
    void changed();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &a, const QModelIndex &b) const override;

private:
    void refresh();
    QString m_filter = QStringLiteral("all"), m_query, m_sortKey = QStringLiteral("added");
    bool m_regex = false, m_asc = false;
    QRegularExpression m_rx;
};
