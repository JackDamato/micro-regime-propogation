#pragma once

#include <unordered_map>
#include <array>
#include <cstddef>

namespace microregime {

struct FeatureSet {
    uint64_t timestamp_ns;
    std::string symbol;

    // --- Price & Spread
    double log_spread;
    double midprice;
    double microprice;
    double price_impact;
    double log_return;

    // --- Volatility
    double ewm_volatility;
    double realized_variance;
    double directional_volatility;
    double spread_volatility;

    // --- Order Flow
    double ofi;
    double signed_volume_pressure;
    double order_arrival_rate;
    double cancel_rate;
    double queue_position_imbalance;

    // --- Liquidity
    double depth_imbalance;
    double market_depth;
    double lob_slope;
    double price_gap;

    // --- Microstructure Transitions
    int8_t tick_direction;
    double reversal_rate;
    double quote_revision_rate;
    int spread_crossing;
    int aggressor_flag;

    // --- Engineered
    double of_entropy;
    double liquidity_stress;
};

} // namespace microregime