#pragma once

#include <cstddef>

namespace microregime {

// Common constants used across the codebase
constexpr size_t DEPTH_LEVELS = 10;     // Top N price levels
constexpr size_t ROLLING_WINDOW = 2000;   // For Feature Input windows *We avg 470 events per second, so 500 is about 1 second
constexpr size_t LONG_WINDOW_SIZE = 7200; // For Feature Normalizer windows   Theory says regimes last about 2 minutes depending on the regime
constexpr size_t SHORT_WINDOW_SIZE = 1200; // For Feature Normalizer windows
constexpr size_t SNAPSHOT_INTERVAL_NS = 500'000'000; // 100ms (for Timestamp Pipeline)

// IT IS VERY IMPORTANT THAT 1'000'000'000 / SNAPSHOT_INTERVAL_NS IS LESS THAN LONG_WINDOW_SIZE, OTHERWISE YOU WILL GET INDEX OUT OF BOUNDS ERRORS

} // namespace microregime
