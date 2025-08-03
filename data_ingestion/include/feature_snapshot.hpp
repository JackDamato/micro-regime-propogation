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
    double best_bid_price;               // COMPUTED GOOD
    double best_ask_price;               // COMPUTED GOOD
    double midprice;

    // --- Depth State (Top N) ---
    std::array<double, DEPTH_LEVELS> bid_prices;    // COMPUTED GOOD
    std::array<double, DEPTH_LEVELS> ask_prices;    // COMPUTED GOOD
    std::array<int, DEPTH_LEVELS> bid_sizes;        // COMPUTED GOOD
    std::array<int, DEPTH_LEVELS> ask_sizes;        // COMPUTED GOOD

    // --- Rolling Statistics / Aggregates ---
    double buy_volume;                              // COMPUTED GOOD
    double sell_volume;                             // COMPUTED GOOD
    uint64_t trade_time_ns;

    double bid_volume;                             
    double ask_volume;                             
    uint64_t order_time_ns;

    // Rolling windows.
    const std::deque<double>* rolling_midprices; // Used for hella things                       COMPUTED GOOD
    const std::deque<double>* rolling_spreads;   // Used for spread volatility, spread mean     COMPUTED GOOD
    const std::deque<int8_t>* rolling_tick_directions; // +1, 0, -1 used for tick direction entropy, COMPUTED GOOD

    const std::deque<int8_t>* rolling_trade_directions; // used for reversal rate, reversal entropy   COMPUTED GOOD
    const std::deque<double>* rolling_trade_sizes; // Used for trade size entropy                COMPUTED GOOD

    const std::deque<double>* rolling_lob_bid_slopes; // Used for LOB slope means               COMPUTED GOOD
    const std::deque<double>* rolling_lob_ask_slopes; // Used for LOB slope means               COMPUTED GOOD
    const std::deque<double>* rolling_order_book_imbalances; // Used for OBI mean               COMPUTED GOOD

    // --- Optional Padding / Alignment ---
    uint32_t reserved = 0;
};