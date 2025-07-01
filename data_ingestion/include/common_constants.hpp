#pragma once

#include <cstddef>

namespace microregime {

// Common constants used across the codebase
constexpr size_t DEPTH_LEVELS = 10;     // Top N price levels
constexpr size_t ROLLING_WINDOW = 500;   // For Feature Input windows
constexpr size_t LONG_WINDOW_SIZE = 500; // For Feature Normalizer windows
constexpr size_t SHORT_WINDOW_SIZE = 50; // For Feature Normalizer windows
constexpr size_t SNAPSHOT_INTERVAL_NS = 1'000'000'000; // 1 second (for Timestamp Pipeline)

} // namespace microregime
