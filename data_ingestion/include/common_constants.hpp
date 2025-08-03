#pragma once

#include <cstddef>

namespace microregime {

// Common constants used across the codebase
constexpr size_t DEPTH_LEVELS = 10;     // Top N price levels
constexpr size_t WINDOW_SIZE = 30000; // For Feature Normalizer windows
constexpr size_t SNAPSHOT_INTERVAL_NS = 100'000'000; // 100ms (for Timestamp Pipeline)
constexpr uint64_t ROLLING_UPDATE_INTERVAL_NS = 10'000'000; // 50 ms <-- This should always be less than snapshot interval
constexpr size_t EVENT_WINDOW_NS = 1'000'000'000; // 1 seconds
constexpr size_t ROLLING_WINDOW = 500;   // For rollings stats updated every ROLLING_UPDATE_INTERVAL_NS. 1'000'000'000 / ROLLING_UPDATE_INTERVAL_NS = Number of events for 1 second


// 1800 events at 50 ms is 90 seconds, 3600 events at 50 ms is 180 seconds

} // namespace microregime
