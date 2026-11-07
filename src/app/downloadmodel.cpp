#include "downloadmodel.hpp"

#include <QHash>
#include <algorithm>
#include <utility>
#include <QSet>

bool Row::operator==(const Row &o) const {
    return id == o.id && kind == o.kind && name == o.name && state == o.state && category == o.category && site == o.site &&
           path == o.path && error == o.error && tags == o.tags && size == o.size && done == o.done && added == o.added &&
           completed == o.completed && progress == o.progress && ratio == o.ratio && down == o.down && up == o.up &&
           eta == o.eta && seeds == o.seeds && peers == o.peers && seedsTotal == o.seedsTotal &&
           peersTotal == o.peersTotal && conns == o.conns && queue == o.queue && isPrivate == o.isPrivate &&
           sequential == o.sequential && segs == o.segs && source == o.source && availability == o.availability &&
           resumable == o.resumable;
}

int DownloadModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : static_cast<int>(m_rows.size()); }

QVariant DownloadModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) return {};
    const Row &r = m_rows.at(index.row());
    switch (role) {
    case IdRole: return r.id;
    case KindRole: return r.kind;
    case Qt::DisplayRole:
    case NameRole: return r.name;
    case StateRole: return r.state;
    case CategoryRole: return r.category;
    case SiteRole: return r.site;
    case PathRole: return r.path;
    case ErrorRole: return r.error;
    case TagsRole: return r.tags;
    case SizeRole: return r.size;
    case DoneRole: return r.done;
    case AddedRole: return r.added;
    case CompletedRole: return r.completed;
    case ProgressRole: return r.progress;
    case RatioRole: return r.ratio;
    case DownRole: return r.down;
    case UpRole: return r.up;
    case EtaRole: return r.eta;
    case SeedsRole: return r.seeds;
    case PeersRole: return r.peers;
    case SeedsTotalRole: return r.seedsTotal;
    case PeersTotalRole: return r.peersTotal;
    case ConnsRole: return r.conns;
    case QueueRole: return r.queue;
    case PrivateRole: return r.isPrivate;
    case SequentialRole: return r.sequential;
    case SegsRole: return r.segs;
    case SourceRole: return r.source;
    case AvailabilityRole: return r.availability;
    case ResumableRole: return r.resumable;
    default: return {};
    }
}

QHash<int, QByteArray> DownloadModel::roleNames() const {
    return {{IdRole, "rid"}, {KindRole, "kind"}, {NameRole, "name"}, {StateRole, "state"}, {CategoryRole, "category"},
            {SiteRole, "site"}, {PathRole, "path"}, {ErrorRole, "error"}, {TagsRole, "tags"}, {SizeRole, "size"},
            {DoneRole, "done"}, {AddedRole, "added"}, {CompletedRole, "completed"}, {ProgressRole, "progress"},
            {RatioRole, "ratio"}, {DownRole, "down"}, {UpRole, "up"}, {EtaRole, "eta"}, {SeedsRole, "seeds"},
            {PeersRole, "peers"}, {SeedsTotalRole, "seedsTotal"}, {PeersTotalRole, "peersTotal"}, {ConnsRole, "conns"},
            {QueueRole, "queuePos"}, {PrivateRole, "isPrivate"}, {SequentialRole, "sequential"}, {SegsRole, "segs"},
            {SourceRole, "source"}, {AvailabilityRole, "availability"}, {ResumableRole, "resumable"}};
}

const Row *DownloadModel::find(const QString &id) const {
    for (const Row &r : m_rows)
        if (r.id == id) return &r;
    return nullptr;
}

void DownloadModel::update(QVector<Row> next) {
    QHash<QString, int> nextIdx;
    for (int i = 0; i < next.size(); ++i) nextIdx.insert(next[i].id, i);
    // 1. Remove rows that disappeared (from the bottom up).
    for (int i = static_cast<int>(m_rows.size()) - 1; i >= 0; --i) {
        if (!nextIdx.contains(m_rows[i].id)) {
            beginRemoveRows({}, i, i);
            m_rows.removeAt(i);
            endRemoveRows();
        }
    }
    // 2. Update rows in place; collect new ones.
    QSet<QString> have;
    for (int i = 0; i < m_rows.size(); ++i) {
        Row &cur = m_rows[i];
        have.insert(cur.id);
        const Row &n = next[nextIdx.value(cur.id)];
        if (!(cur == n)) {
            cur = n;
            emit dataChanged(index(i), index(i));
        }
    }
    // 3. Append new rows.
    QVector<Row> added;
    for (const Row &n : next)
        if (!have.contains(n.id)) added.push_back(n);
    if (!added.isEmpty()) {
        const int first = static_cast<int>(m_rows.size());
        beginInsertRows({}, first, first + static_cast<int>(added.size()) - 1);
        m_rows += added;
        endInsertRows();
    }
}

// ---------------------------------------------------------------- filter

DownloadFilter::DownloadFilter(QObject *parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
    sort(0, Qt::DescendingOrder);
    connect(this, &QAbstractItemModel::rowsInserted, this, &DownloadFilter::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &DownloadFilter::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &DownloadFilter::countChanged);
    connect(this, &QAbstractItemModel::layoutChanged, this, &DownloadFilter::countChanged);
}

static bool isActive(const Row &r) { return r.down > 0 || r.up > 0; }

bool DownloadFilter::matches(const Row &r, const QString &f) {
    if (f.isEmpty() || f == QLatin1String("all")) return true;
    const QString &s = r.state;
    if (f == QLatin1String("downloading"))
        return s == QLatin1String("downloading") || s == QLatin1String("stalled") || s == QLatin1String("metadata") ||
               s == QLatin1String("probing");
    if (f == QLatin1String("seeding")) return s == QLatin1String("seeding");
    if (f == QLatin1String("completed")) return r.progress >= 1.0 && r.size > 0;
    if (f == QLatin1String("paused")) return s == QLatin1String("paused") || s == QLatin1String("queued");
    if (f == QLatin1String("active")) return isActive(r);
    if (f == QLatin1String("inactive")) return !isActive(r);
    if (f == QLatin1String("stalled")) return s == QLatin1String("stalled");
    if (f == QLatin1String("checking")) return s == QLatin1String("checking") || s == QLatin1String("moving");
    if (f == QLatin1String("error")) return s == QLatin1String("error");
    if (f.startsWith(QLatin1String("k:"))) return r.kind == f.mid(2);
    if (f.startsWith(QLatin1String("cat:"))) return r.category == f.mid(4);
    if (f.startsWith(QLatin1String("tag:"))) return r.tags.contains(f.mid(4));
    if (f.startsWith(QLatin1String("site:"))) return r.site == f.mid(5);
    return s == f;
}

void DownloadFilter::refresh() {
    invalidate();
    emit changed();
    emit countChanged();
}

void DownloadFilter::setFilterKey(const QString &f) { if (m_filter != f) { m_filter = f; refresh(); } }
void DownloadFilter::setQuery(const QString &q) {
    if (m_query == q) return;
    m_query = q;
    m_rx = QRegularExpression(q, QRegularExpression::CaseInsensitiveOption);
    refresh();
}
void DownloadFilter::setRegex(bool r) { if (m_regex != r) { m_regex = r; refresh(); } }
void DownloadFilter::setSortKey(const QString &k) { if (m_sortKey != k) { m_sortKey = k; refresh(); } }
void DownloadFilter::setAscending(bool a) {
    if (m_asc == a) return;
    m_asc = a;
    sort(0, a ? Qt::AscendingOrder : Qt::DescendingOrder);
    refresh();
}

bool DownloadFilter::filterAcceptsRow(int sourceRow, const QModelIndex &) const {
    auto *m = static_cast<DownloadModel *>(sourceModel());
    if (!m || sourceRow >= m->rows().size()) return false;
    const Row &r = m->rows().at(sourceRow);
    if (!matches(r, m_filter)) return false;
    if (m_query.isEmpty()) return true;
    if (m_regex) return m_rx.isValid() && m_rx.match(r.name).hasMatch();
    return r.name.contains(m_query, Qt::CaseInsensitive) || r.site.contains(m_query, Qt::CaseInsensitive);
}

bool DownloadFilter::lessThan(const QModelIndex &a, const QModelIndex &b) const {
    auto *m = static_cast<DownloadModel *>(sourceModel());
    const Row &x = m->rows().at(a.row());
    const Row &y = m->rows().at(b.row());
    const QString &k = m_sortKey;
    auto tie = [&] { return x.added < y.added || (x.added == y.added && x.id < y.id); };
    if (k == QLatin1String("name")) {
        const int c = QString::localeAwareCompare(x.name, y.name);
        return c != 0 ? c < 0 : tie();
    }
    auto num = [&](double p, double q) { return p != q ? p < q : tie(); };
    if (k == QLatin1String("size")) return num(static_cast<double>(x.size), static_cast<double>(y.size));
    if (k == QLatin1String("progress")) return num(x.progress, y.progress);
    if (k == QLatin1String("state")) { const int c = x.state.compare(y.state); return c != 0 ? c < 0 : tie(); }
    if (k == QLatin1String("down")) return num(x.down, y.down);
    if (k == QLatin1String("up")) return num(x.up, y.up);
    if (k == QLatin1String("eta")) {
        const double ex = x.eta < 0 ? 1e18 : x.eta, ey = y.eta < 0 ? 1e18 : y.eta;
        return num(ex, ey);
    }
    if (k == QLatin1String("peers")) return num(x.seeds + x.peers + x.conns, y.seeds + y.peers + y.conns);
    if (k == QLatin1String("ratio")) return num(x.ratio, y.ratio);
    if (k == QLatin1String("queue")) return num(x.queue < 0 ? 1e9 : x.queue, y.queue < 0 ? 1e9 : y.queue);
    return tie(); // "added"
}

QString DownloadFilter::idAt(int row) const {
    const QModelIndex i = index(row, 0);
    return i.isValid() ? i.data(DownloadModel::IdRole).toString() : QString();
}

int DownloadFilter::rowOf(const QString &id) const {
    for (int i = 0; i < rowCount(); ++i)
        if (index(i, 0).data(DownloadModel::IdRole).toString() == id) return i;
    return -1;
}

QStringList DownloadFilter::idsBetween(int a, int b) const {
    QStringList out;
    if (a > b) std::swap(a, b);
    for (int i = std::max(0, a); i <= b && i < rowCount(); ++i) out << idAt(i);
    return out;
}

QStringList DownloadFilter::allIds() const { return idsBetween(0, rowCount() - 1); }
