// Rift core: byte-range bookkeeping and the work-stealing segment scheduler used by rift-get.
// Pure C++20, no Qt and no libcurl, so it can be unit-tested anywhere.
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rift::core {

// Half-open byte range [start, end).
struct Range {
    int64_t start = 0;
    int64_t end = 0;
    int64_t size() const { return end - start; }
    bool operator==(const Range &o) const { return start == o.start && end == o.end; }
};

// Sorted, merged set of ranges.
class RangeSet {
public:
    void add(Range r);
    void clear() { m_r.clear(); }
    int64_t total() const;
    bool contains(int64_t pos) const;
    const std::vector<Range> &ranges() const { return m_r; }
    // Ranges of [0, size) that are NOT in the set.
    std::vector<Range> complement(int64_t size) const;
    // "0-1048576,2097152-3145728" (end exclusive). Parse ignores bad parts.
    std::string serialize() const;
    static RangeSet parse(const std::string &s);

private:
    std::vector<Range> m_r;
};

// One connection's current job: it downloads [cursor, end). `end` may shrink when another
// connection steals the second half of the remaining work.
struct Work {
    int worker = -1;
    int64_t start = 0;
    int64_t cursor = 0;
    int64_t end = 0;
    int64_t remaining() const { return end - cursor; }
};

// Dynamic segmentation (PLAN 4.14):
//  - a new connection takes a free (unassigned, not done) range;
//  - when none is free, it splits the largest remaining job in half and takes the second half,
//    so every connection stays busy until the end and there is no slow tail.
class SegmentScheduler {
public:
    SegmentScheduler(int64_t fileSize, int64_t minSegment);

    void setDone(const RangeSet &done) { m_done = done; }
    const RangeSet &done() const { return m_done; }
    int64_t fileSize() const { return m_size; }
    int64_t doneBytes() const { return m_done.total(); }
    bool complete() const { return m_done.total() >= m_size; }

    // Gives `worker` a job, or nothing if all remaining work is too small to split.
    std::optional<Work> acquire(int worker);
    // Records `len` bytes received at `offset` by `worker`. Returns how many bytes may be
    // written (data past the job's current end belongs to someone else). When the job ends the
    // worker has no job anymore and should call acquire() again.
    int64_t accept(int worker, int64_t offset, int64_t len);
    // Current job of a worker (nullptr if none).
    const Work *job(int worker) const;
    // Worker failed or stopped: its unfinished range becomes free again.
    void release(int worker);
    const std::vector<Work> &active() const { return m_active; }
    // Ranges no one is working on that still need data.
    std::vector<Range> freeRanges() const;

private:
    int64_t m_size;
    int64_t m_min;
    RangeSet m_done;
    std::vector<Work> m_active;
};

} // namespace rift::core
