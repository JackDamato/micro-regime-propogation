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
    window.push_back(feature_set);
    update_sums(feature_set, feature_sums, feature_sums_2);

    if (window.size() > WINDOW_SIZE) {
        update_sums(window.front(), feature_sums, feature_sums_2, -1.0);
        window.pop_front();
    }
}


FeatureSet FeatureNormalizer::NormalizeFeatureSet(const FeatureSet& feature_set) {
    FeatureSet normalized_feature_set;

    normalized_feature_set.timestamp_ns = feature_set.timestamp_ns;
    normalized_feature_set.instrument = feature_set.instrument;

    auto safe_zscore = [](double x, double sum, double sum2, size_t n, const std::string& field) {
        double mean = sum / n;
        double variance = (sum2 / n) - (mean * mean);
        if (variance <= 0.0) {
            std::cout << "Warning: Variance is non-positive. Setting to 1.0. Field: " << field << std::endl;
        }
        double stddev = (variance > 0.0) ? std::sqrt(variance) : 1.0;
        return (x - mean) / stddev;
    };

    const size_t n = window.size();

    for (const auto& [name, member] : kFeatures) {
        double x = feature_set.*member;

        // Long window
        double sum = feature_sums[name];
        double sum2 = feature_sums_2[name];
        normalized_feature_set.*member = safe_zscore(x, sum, sum2, n, name);
    }

    return normalized_feature_set;
}

} // namespace microregime