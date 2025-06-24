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
    // uint32_t quote_update_count;         // Number of order book modifications this tick

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
    const std::deque<double>* rolling_midprices;
    const std::deque<double>* rolling_spreads;
    const std::deque<int8_t>* rolling_tick_directions; // +1, 0, -1
    const std::deque<int8_t>* rolling_trade_directions;

    // --- Depth Change Direction (LOB Dynamics) ---
    std::array<int8_t, DEPTH_LEVELS> bid_depth_change_direction; // +1 = added, -1 = removed
    std::array<int8_t, DEPTH_LEVELS> ask_depth_change_direction;

    // --- Trade Execution Data (if applicable) ---
    double last_trade_price;     // Execution price if trade occurred
    double last_trade_size;
    int8_t last_trade_direction; // +1 = buy aggressor, -1 = sell, 0 = none

    // --- Optional Padding / Alignment ---
    uint32_t reserved = 0;
};