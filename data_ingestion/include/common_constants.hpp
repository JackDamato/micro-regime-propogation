#pragma once

#include <cstddef>

namespace microregime {

// Common constants used across the codebase
constexpr size_t DEPTH_LEVELS = 10;     // Top N price levels
constexpr size_t WINDOW_SIZE = 3600; // For Feature Normalizer windows
constexpr size_t SNAPSHOT_INTERVAL_NS = 1'000'000'000; // 100ms (for Timestamp Pipeline)
constexpr uint64_t ROLLING_UPDATE_INTERVAL_NS = 50'000'000; // 100 ms <-- This should always be less than snapshot interval
constexpr size_t EVENT_WINDOW_NS = 5'000'000'000; // Past X'000'000'000 seconds of events
constexpr size_t ROLLING_WINDOW = 600;   // For rollings stats updated every ROLLING_UPDATE_INTERVAL_NS. 1'000'000'000 / ROLLING_UPDATE_INTERVAL_NS = Number of events for 1 second

// FOR FEATURE WINDOW SIZES LONG_LONG, LONG, MEDIUM, SHORT, SHORT_SHORT, ADJUST BASED ON ROLLING WINDOW
constexpr int LONG_LONG = 600;
constexpr int LONG = 300;
constexpr int MEDIUM = 150;
constexpr int SHORT = 50;
constexpr int SHORT_SHORT = 20;

} // namespace microregime
