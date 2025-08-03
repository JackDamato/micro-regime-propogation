#pragma once

#include <unordered_map>
#include <array>
#include <cstddef>
#include <string>

namespace microregime {

struct FeatureSet {
    uint64_t timestamp_ns;
    std::string instrument;

    // --- Price & Spread
    double midprice;
    double mean_log_return;
    double midprice_ema_ratio;
    double midprice_ema_deviation;
    double spread_mean;

    // --- Volatility
    double realized_variance;
    double realized_variance_recent;
    double realized_variance_bias;
    double directional_volatility;
    double spread_volatility;
    double vol_of_variance;

    // --- Order Flow
    double ofi;
    double add_rate;
    double trade_rate;
    double trade_size_entropy;

    // --- Liquidity
    double obi;
    double lob_bid_slope_mean;
    double lob_ask_slope_mean;

    // --- Microstructure Transitions
    double tick_direction_entropy;
    double reversal_rate;
    double reversal_entropy;
    double aggressor_ratio;
    double aggressor_bias;
};

} // namespace microregime