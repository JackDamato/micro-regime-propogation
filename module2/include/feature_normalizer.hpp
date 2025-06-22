#pragma once

#include "feature_set.hpp"
#include <deque>
#include <utility>
#include <string>
#include <unordered_map>

namespace features {
class FeatureNormalizer {
public:
    FeatureNormalizer() = default;
    ~FeatureNormalizer() = default;

    void AddFeatureSet(const FeatureSet& feature_set);
    std::pair<FeatureSet, FeatureSet> NormalizeFeatureSet(const FeatureSet& feature_set);

private:
    std::deque<FeatureSet> window_long;
    std::deque<FeatureSet> window_short;

    // valid keys:
    // log_spread, price_impact, log_return, 
    // ewm_volatility, realized_variance, directional_volatility, spread_volatility, 
    // ofi, signed_volume_pressure, order_arrival_rate, cancel_rate, 
    // market_depth, lob_slope, price_gap, 
    // shannon_entropy, liquidity_stress

    // long-window
    std::unordered_map<std::string, double> feature_sums_long;
    std::unordered_map<std::string, double> feature_sums_2_long;

    // short-window
    std::unordered_map<std::string, double> feature_sums_short;
    std::unordered_map<std::string, double> feature_sums_2_short;
};
} // namespace microregime
