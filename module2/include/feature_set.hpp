#pragma once

#include <unordered_map>
#include <array>
#include <cstddef>
#include <string>

namespace features {

struct FeatureSet {
    uint64_t timestamp_ns;
    std::string symbol;

    // --- Price & Spread
    double log_spread;
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

    // --- Liquidity
    double depth_imbalance;
    double market_depth;
    double lob_slope;
    double price_gap;

    // --- Microstructure Transitions
    double tick_direction_entropy;
    double reversal_rate;
    double spread_crossing;
    double aggressor_bias;

    // --- Engineered
    double shannon_entropy;
    double liquidity_stress;
};

} // namespace features