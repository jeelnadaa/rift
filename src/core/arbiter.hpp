// Rift core: unified bandwidth arbiter (PLAN 3). Splits one global limit between the torrent
// engine and the HTTP engine by measured demand, so the combined speed respects the limit.
#pragma once
#include <algorithm>
#include <cstdint>
#include <utility>

namespace rift::core {

// Returns {torrentLimit, httpLimit} in bytes/s. limit <= 0 means unlimited → {0, 0}.
// Each side keeps at least `floorShare` of the limit so an idle engine can ramp up again.
inline std::pair<int64_t, int64_t> splitLimit(int64_t limit, double torrentSpeed, double httpSpeed,
                                              double floorShare = 0.15) {
    if (limit <= 0) return {0, 0};
    const double t = std::max(0.0, torrentSpeed), h = std::max(0.0, httpSpeed);
    double hs = (t + h) > 0 ? h / (t + h) : 0.5;
    hs = std::clamp(hs, floorShare, 1.0 - floorShare);
    const auto http = std::max<int64_t>(1024, static_cast<int64_t>(static_cast<double>(limit) * hs));
    const auto bt = std::max<int64_t>(1024, limit - http);
    return {bt, http};
}

} // namespace rift::core
