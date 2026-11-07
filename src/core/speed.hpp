// Rift core: measured speed (PLAN 4.17). Header-only.
// current(): bytes in the last 1 s. smoothed(): exponential moving average (~5 s).
// Both drop to exactly 0 when nothing arrived for 2 s.
#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace rift::core {

class SpeedMeter {
public:
    static constexpr int64_t kBucketMs = 250;
    static constexpr int kBuckets = 8; // 2 s of history

    void add(int64_t bytes, int64_t nowMs) {
        roll(nowMs);
        m_buckets[static_cast<size_t>(m_head)] += bytes;
        if (bytes > 0) m_lastByteMs = nowMs;
        m_total += bytes;
    }

    // Call regularly (e.g. every 500 ms) so the smoothed value moves even without data.
    void tick(int64_t nowMs) {
        roll(nowMs);
        const double cur = current(nowMs);
        if (m_lastTickMs < 0) {
            m_ema = cur;
        } else {
            const double dt = static_cast<double>(nowMs - m_lastTickMs);
            const double a = 1.0 - std::exp(-dt / 5000.0);
            m_ema += (cur - m_ema) * a;
        }
        m_lastTickMs = nowMs;
        if (idle(nowMs)) m_ema = 0;
    }

    double current(int64_t nowMs) {
        roll(nowMs);
        if (idle(nowMs)) return 0;
        int64_t sum = 0;
        for (int i = 0; i < 4; ++i) sum += m_buckets[static_cast<size_t>((m_head - i + kBuckets) % kBuckets)];
        return static_cast<double>(sum); // 4 buckets = 1 s
    }
    double smoothed(int64_t nowMs) { return idle(nowMs) ? 0 : m_ema; }
    int64_t total() const { return m_total; }
    void reset() { *this = SpeedMeter(); }

private:
    bool idle(int64_t nowMs) const { return m_lastByteMs < 0 || nowMs - m_lastByteMs >= 2000; }
    void roll(int64_t nowMs) {
        const int64_t slot = nowMs / kBucketMs;
        if (m_slot < 0) { m_slot = slot; return; }
        int64_t steps = slot - m_slot;
        if (steps <= 0) return;
        if (steps > kBuckets) steps = kBuckets;
        for (int64_t i = 0; i < steps; ++i) {
            m_head = (m_head + 1) % kBuckets;
            m_buckets[static_cast<size_t>(m_head)] = 0;
        }
        m_slot = slot;
    }

    std::array<int64_t, kBuckets> m_buckets{};
    int m_head = 0;
    int64_t m_slot = -1;
    int64_t m_lastByteMs = -1;
    int64_t m_lastTickMs = -1;
    int64_t m_total = 0;
    double m_ema = 0;
};

} // namespace rift::core
