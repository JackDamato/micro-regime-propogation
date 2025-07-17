#include "feature_normalizer.hpp"
#include "common_constants.hpp"
#include "feature_set.hpp"
#include <numeric>
#include <iostream>
#include <ctime>

namespace microregime {

const std::vector<std::pair<std::string, double FeatureSet::*>> kFeatures = {
    {"midprice", &FeatureSet::midprice},
    {"log_spread", &FeatureSet::log_spread},
    {"log_return", &FeatureSet::log_return},
    {"ewm_volatility", &FeatureSet::ewm_volatility},
    {"realized_variance", &FeatureSet::realized_variance},
    {"directional_volatility", &FeatureSet::directional_volatility},
    {"spread_volatility", &FeatureSet::spread_volatility},
    {"ofi", &FeatureSet::ofi},
    {"signed_volume_pressure", &FeatureSet::signed_volume_pressure},
    {"order_arrival_rate", &FeatureSet::order_arrival_rate},
    {"tick_direction_entropy", &FeatureSet::tick_direction_entropy},
    {"depth_imbalance", &FeatureSet::depth_imbalance},
    {"market_depth", &FeatureSet::market_depth},
    {"lob_slope", &FeatureSet::lob_slope},
    {"price_gap", &FeatureSet::price_gap},
    {"shannon_entropy", &FeatureSet::shannon_entropy},
    {"liquidity_stress", &FeatureSet::liquidity_stress},
    {"reversal_rate", &FeatureSet::reversal_rate},
    {"aggressor_bias", &FeatureSet::aggressor_bias}
};

void FeatureNormalizer::AddFeatureSet(const FeatureSet& feature_set) {
    auto update_sums = [&](const FeatureSet& fs, 
                           std::unordered_map<std::string, double>& sum,
                           std::unordered_map<std::string, double>& sum2,
                           double sign = 1.0) {
        for (const auto& [name, ptr] : kFeatures) {
            double val = fs.*ptr;
            sum[name] += sign * val;
            sum2[name] += sign * val * val;
        }
    };

    // --- Long window update ---
    window_long.push_back(feature_set);
    update_sums(feature_set, feature_sums_long, feature_sums_2_long);

    if (window_long.size() > LONG_WINDOW_SIZE) {
        update_sums(window_long.front(), feature_sums_long, feature_sums_2_long, -1.0);
        window_long.pop_front();
    }

    // --- Short window update ---
    window_short.push_back(feature_set);
    update_sums(feature_set, feature_sums_short, feature_sums_2_short);

    if (window_short.size() > SHORT_WINDOW_SIZE) {
        update_sums(window_short.front(), feature_sums_short, feature_sums_2_short, -1.0);
        window_short.pop_front();
    }
}


std::pair<FeatureSet, FeatureSet> FeatureNormalizer::NormalizeFeatureSet(const FeatureSet& feature_set) {
    FeatureSet normalized_feature_set_long;
    FeatureSet normalized_feature_set_short;

    normalized_feature_set_long.timestamp_ns = feature_set.timestamp_ns;
    normalized_feature_set_long.instrument = feature_set.instrument;

    normalized_feature_set_short = normalized_feature_set_long;

    auto safe_zscore = [](double x, double sum, double sum2, size_t n, const std::string& field) {
        double mean = sum / n;
        double variance = (sum2 / n) - (mean * mean);
        if (variance <= 0.0) {
            std::cout << "Warning: Variance is non-positive. Setting to 1.0. Field: " << field << std::endl;
        }
        double stddev = (variance > 0.0) ? std::sqrt(variance) : 1.0;
        return (x - mean) / stddev;
    };

    const size_t n_long = window_long.size();
    const size_t n_short = window_short.size();

    for (const auto& [name, member] : kFeatures) {
        double x = feature_set.*member;

        // Long window
        double sum_long = feature_sums_long[name];
        double sum2_long = feature_sums_2_long[name];
        normalized_feature_set_long.*member = safe_zscore(x, sum_long, sum2_long, n_long, name);

        // Short window
        double sum_short = feature_sums_short[name];
        double sum2_short = feature_sums_2_short[name];
        normalized_feature_set_short.*member = safe_zscore(x, sum_short, sum2_short, n_short, name);
    }

    return std::make_pair(normalized_feature_set_long, normalized_feature_set_short);
}

} // namespace microregime