#pragma once

#include "feature_set.hpp"
#include <vector>
#include <utility>
#include <string>

namespace microregime {

const std::vector<std::pair<std::string, double FeatureSet::*>> kFeatures = {
    {"midprice", &FeatureSet::midprice},
    {"mean_log_return", &FeatureSet::mean_log_return},
    {"midprice_ema_ratio", &FeatureSet::midprice_ema_ratio},
    {"midprice_ema_deviation", &FeatureSet::midprice_ema_deviation},
    {"realized_variance", &FeatureSet::realized_variance},
    {"realized_variance_recent", &FeatureSet::realized_variance_recent},
    {"realized_variance_bias", &FeatureSet::realized_variance_bias},
    {"directional_volatility", &FeatureSet::directional_volatility},
    {"vol_of_variance", &FeatureSet::vol_of_variance},
    {"ofi", &FeatureSet::ofi},
    {"add_rate", &FeatureSet::add_rate},
    {"trade_rate", &FeatureSet::trade_rate},
    {"trade_size_entropy", &FeatureSet::trade_size_entropy},
    {"tick_direction_entropy", &FeatureSet::tick_direction_entropy},
    {"reversal_rate", &FeatureSet::reversal_rate},
    {"reversal_entropy", &FeatureSet::reversal_entropy},
    {"aggressor_ratio", &FeatureSet::aggressor_ratio},
    {"aggressor_bias", &FeatureSet::aggressor_bias}
};

} // namespace microregime