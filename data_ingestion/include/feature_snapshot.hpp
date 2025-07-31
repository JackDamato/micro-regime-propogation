#pragma once

#include <deque>
#include <array>
#include <string>
#include <cstdint>
#include "common_constants.hpp"

using microregime::DEPTH_LEVELS;
using microregime::ROLLING_WINDOW;

struct FeatureInputSnapshot {
    // --- Timestamp and Event Type ---
    uint64_t timestamp_ns;               // Nanosecond timestamp
    std::string instrument;

    // --- Top of Book State ---
    double best_bid_price;
    double best_ask_price;

    // --- Depth State (Top N) ---
    std::array<double, DEPTH_LEVELS> bid_prices;
    std::array<double, DEPTH_LEVELS> ask_prices;
    std::array<int, DEPTH_LEVELS> bid_sizes;
    std::array<int, DEPTH_LEVELS> ask_sizes;

    // --- Rolling Statistics / Aggregates ---
    double rolling_buy_volume;
    double rolling_sell_volume;
    int adds_since_last_snapshot;

    // Rolling windows.
    const std::deque<double>* rolling_midprices; // Take the last 5 minutes of midprices 0.05 seconds between each update.
    const std::deque<double>* rolling_spreads;   // Take the last 5 minutes of spreads 0.05 seconds between each update.
    const std::deque<int8_t>* rolling_tick_directions; // +1, 0, -1
    const std::deque<int8_t>* rolling_trade_directions;

    // --- Depth Change Direction (LOB Dynamics) ---
    std::array<int8_t, DEPTH_LEVELS> bid_depth_change_direction; // +1 = added, -1 = removed
    std::array<int8_t, DEPTH_LEVELS> ask_depth_change_direction;

    // --- Optional Padding / Alignment ---
    uint32_t reserved = 0;
};