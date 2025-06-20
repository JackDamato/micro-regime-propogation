#pragma once

#include <cstddef>

namespace microregime {

// Common constants used across the codebase
constexpr size_t DEPTH_LEVELS = 10;     // Top N price levels
constexpr size_t ROLLING_WINDOW = 500;   // For volatility/return stats

} // namespace microregime
