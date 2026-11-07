#include "segments.hpp"

#include <algorithm>
#include <sstream>

namespace rift::core {

void RangeSet::add(Range r) {
    if (r.end <= r.start) return;
    std::vector<Range> out;
    out.reserve(m_r.size() + 1);
    bool placed = false;
    for (const Range &x : m_r) {
        if (x.end < r.start) {
            out.push_back(x);
        } else if (r.end < x.start) {
            if (!placed) { out.push_back(r); placed = true; }
            out.push_back(x);
        } else {
            r.start = std::min(r.start, x.start);
            r.end = std::max(r.end, x.end);
        }
    }
    if (!placed) out.push_back(r);
    std::sort(out.begin(), out.end(), [](const Range &a, const Range &b) { return a.start < b.start; });
    m_r.swap(out);
}

int64_t RangeSet::total() const {
    int64_t t = 0;
    for (const Range &r : m_r) t += r.size();
    return t;
}

bool RangeSet::contains(int64_t pos) const {
    for (const Range &r : m_r)
        if (pos >= r.start && pos < r.end) return true;
    return false;
}

std::vector<Range> RangeSet::complement(int64_t size) const {
    std::vector<Range> out;
    int64_t pos = 0;
    for (const Range &r : m_r) {
        if (r.start > pos) out.push_back({pos, std::min(r.start, size)});
        pos = std::max(pos, r.end);
        if (pos >= size) break;
    }
    if (pos < size) out.push_back({pos, size});
    return out;
}

std::string RangeSet::serialize() const {
    std::ostringstream s;
    for (size_t i = 0; i < m_r.size(); ++i) {
        if (i) s << ',';
        s << m_r[i].start << '-' << m_r[i].end;
    }
    return s.str();
}

RangeSet RangeSet::parse(const std::string &s) {
    RangeSet set;
    size_t i = 0;
    while (i < s.size()) {
        size_t comma = s.find(',', i);
        if (comma == std::string::npos) comma = s.size();
        const std::string part = s.substr(i, comma - i);
        const size_t dash = part.find('-');
        if (dash != std::string::npos && dash > 0) {
            try {
                const long long a = std::stoll(part.substr(0, dash));
                const long long b = std::stoll(part.substr(dash + 1));
                if (a >= 0 && b > a) set.add({a, b});
            } catch (...) {
            }
        }
        i = comma + 1;
    }
    return set;
}

SegmentScheduler::SegmentScheduler(int64_t fileSize, int64_t minSegment)
    : m_size(fileSize), m_min(std::max<int64_t>(minSegment, 1)) {}

std::vector<Range> SegmentScheduler::freeRanges() const {
    RangeSet taken = m_done;
    for (const Work &w : m_active) taken.add({w.cursor, w.end});
    return taken.complement(m_size);
}

const Work *SegmentScheduler::job(int worker) const {
    for (const Work &w : m_active)
        if (w.worker == worker) return &w;
    return nullptr;
}

std::optional<Work> SegmentScheduler::acquire(int worker) {
    if (job(worker)) release(worker);
    const auto free = freeRanges();
    if (!free.empty()) {
        Work w{worker, free.front().start, free.front().start, free.front().end};
        m_active.push_back(w);
        return w;
    }
    // Steal: split the largest remaining job in half.
    Work *big = nullptr;
    for (Work &w : m_active)
        if (!big || w.remaining() > big->remaining()) big = &w;
    if (!big || big->remaining() < 2 * m_min) return std::nullopt;
    const int64_t mid = big->cursor + big->remaining() / 2;
    Work w{worker, mid, mid, big->end};
    big->end = mid;
    m_active.push_back(w);
    return w;
}

int64_t SegmentScheduler::accept(int worker, int64_t offset, int64_t len) {
    for (size_t i = 0; i < m_active.size(); ++i) {
        Work &w = m_active[i];
        if (w.worker != worker) continue;
        if (offset != w.cursor || len <= 0) return 0;
        const int64_t ok = std::min(len, w.end - w.cursor);
        if (ok > 0) {
            m_done.add({w.cursor, w.cursor + ok});
            w.cursor += ok;
        }
        if (w.cursor >= w.end) m_active.erase(m_active.begin() + static_cast<long>(i));
        return ok;
    }
    return 0;
}

void SegmentScheduler::release(int worker) {
    m_active.erase(std::remove_if(m_active.begin(), m_active.end(), [worker](const Work &w) { return w.worker == worker; }),
                   m_active.end());
}

} // namespace rift::core
