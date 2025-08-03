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
    // mean_log_return, midprice_ema_ratio, midprice_ema_deviation,
    // realized_variance, realized_variance_recent, realized_variance_bias,
    // directional_volatility, spread_volatility, vol_of_variance,
    // ofi, add_rate, trade_rate, trade_size_entropy,
    // obi, lob_bid_slope_mean, lob_ask_slope_mean,
    // tick_direction_entropy, reversal_rate, reversal_entropy,
    // aggressor_ratio, aggressor_bias

    std::unordered_map<std::string, double> feature_sums;
    std::unordered_map<std::string, double> feature_sums_2;
};
} // namespace microregime
