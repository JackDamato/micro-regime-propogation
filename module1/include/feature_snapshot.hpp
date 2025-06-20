#pragma once

#include <array>
#include <cstdint>
#include "common_constants.hpp"

using microregime::DEPTH_LEVELS;
using microregime::ROLLING_WINDOW;

enum class EventType : uint8_t {
    UNKNOWN = 0,
    QUOTE_UPDATE = 1,
    MARKET_ORDER = 2,
    CANCEL = 3
};

struct FeatureInputSnapshot {
    // --- Timestamp and Event Type ---
    uint64_t timestamp_ns;               // Nanosecond timestamp
    EventType event_type;                // Type of event (quote, trade, cancel)
    uint32_t quote_update_count;         // Number of order book modifications this tick

    // --- Top of Book State ---
    double best_bid_price;
    double best_ask_price;
    int best_bid_size;
    int best_ask_size;

    // --- Depth State (Top N) ---
    std::array<double, DEPTH_LEVELS> bid_prices;
    std::array<double, DEPTH_LEVELS> ask_prices;
    std::array<int, DEPTH_LEVELS> bid_sizes;
    std::array<int, DEPTH_LEVELS> ask_sizes;

    // --- Rolling Statistics / Aggregates ---
    double rolling_buy_volume;
    double rolling_sell_volume;
    int rolling_add_count;
    int rolling_cancel_count;
    std::array<double, ROLLING_WINDOW> rolling_midprices;
    std::array<double, ROLLING_WINDOW> rolling_spreads;
    std::array<int8_t, ROLLING_WINDOW> rolling_tick_directions; // +1, 0, -1

    // --- Depth Change Direction (LOB Dynamics) ---
    // std::array<int8_t, DEPTH_LEVELS> bid_depth_change_direction; // +1 = added, -1 = removed
    // std::array<int8_t, DEPTH_LEVELS> ask_depth_change_direction;

    // --- Trade Execution Data (if applicable) ---
    double last_trade_price;     // Execution price if trade occurred
    double last_trade_size;
    int8_t last_trade_direction; // +1 = buy aggressor, -1 = sell, 0 = none

    // --- Optional Padding / Alignment ---
    uint32_t reserved = 0;
};