#pragma once

#include <cstddef>

namespace microregime {

// Common constants used across the codebase
constexpr size_t DEPTH_LEVELS = 10;     // Top N price levels
constexpr size_t ROLLING_WINDOW = 1500;   // For Feature Input windows *We avg 470 events per second, so 500 is about 1 second
constexpr size_t WINDOW_SIZE = 30000; // For Feature Normalizer windows
constexpr size_t SNAPSHOT_INTERVAL_NS = 500'000'000; // 100ms (for Timestamp Pipeline)

} // namespace microregime
