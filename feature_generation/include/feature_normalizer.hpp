#pragma once

#include "feature_set.hpp"
#include <deque>
#include <utility>
#include <string>
#include <unordered_map>
#include "common_constants.hpp"

namespace microregime {
class FeatureNormalizer {
public:
    FeatureNormalizer() = default;
    ~FeatureNormalizer() = default;

    void AddFeatureSet(const FeatureSet& feature_set);
    FeatureSet NormalizeFeatureSet(const FeatureSet& feature_set);

    double getOldMidprice(int index) {
        if (index >= window.size()) {
            return 0.0;
        }
        return window[window.size() - index - 1].midprice;
    }

private:
    std::deque<FeatureSet> window;

    // valid keys:
    // log_spread, price_impact, log_return, 
    // ewm_volatility, realized_variance, directional_volatility, spread_volatility, 
    // ofi, signed_volume_pressure, order_arrival_rate, cancel_rate, 
    // market_depth, lob_slope, price_gap, 
    // shannon_entropy, liquidity_stress

    std::unordered_map<std::string, double> feature_sums;
    std::unordered_map<std::string, double> feature_sums_2;
};
} // namespace microregime
