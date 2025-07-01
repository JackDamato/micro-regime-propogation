#include "feature_normalizer.hpp"
#include "common_constants.hpp"
#include "feature_set.hpp"
#include <numeric>
#include <iostream>
#include <ctime>

namespace microregime {

const std::vector<std::string> feature_names = {
    "log_spread", "price_impact", "log_return",
    "ewm_volatility", "realized_variance", "directional_volatility", "spread_volatility",
    "ofi", "signed_volume_pressure", "order_arrival_rate",
    "depth_imbalance", "market_depth", "lob_slope", "price_gap",
    "shannon_entropy", "liquidity_stress"
};

//  "reversal_rate", "spread_crossing", "aggressor_bias" are already normalized

void FeatureNormalizer::AddFeatureSet(const FeatureSet& feature_set) {
    window_long.push_back(feature_set);

    feature_sums_long["log_spread"] += feature_set.log_spread;
    feature_sums_long["price_impact"] += feature_set.price_impact;
    feature_sums_long["log_return"] += feature_set.log_return;
    feature_sums_long["ewm_volatility"] += feature_set.ewm_volatility;
    feature_sums_long["realized_variance"] += feature_set.realized_variance;
    feature_sums_long["directional_volatility"] += feature_set.directional_volatility;
    feature_sums_long["spread_volatility"] += feature_set.spread_volatility;
    feature_sums_long["ofi"] += feature_set.ofi;
    feature_sums_long["signed_volume_pressure"] += feature_set.signed_volume_pressure;
    feature_sums_long["order_arrival_rate"] += feature_set.order_arrival_rate;
    feature_sums_long["tick_direction_entropy"] += feature_set.tick_direction_entropy;
    feature_sums_long["depth_imbalance"] += feature_set.depth_imbalance;
    feature_sums_long["market_depth"] += feature_set.market_depth;
    feature_sums_long["lob_slope"] += feature_set.lob_slope;
    feature_sums_long["price_gap"] += feature_set.price_gap;
    feature_sums_long["shannon_entropy"] += feature_set.shannon_entropy;
    feature_sums_long["liquidity_stress"] += feature_set.liquidity_stress;
    
    feature_sums_2_long["log_spread"] += feature_set.log_spread * feature_set.log_spread;
    feature_sums_2_long["price_impact"] += feature_set.price_impact * feature_set.price_impact;
    feature_sums_2_long["log_return"] += feature_set.log_return * feature_set.log_return;
    feature_sums_2_long["ewm_volatility"] += feature_set.ewm_volatility * feature_set.ewm_volatility;
    feature_sums_2_long["realized_variance"] += feature_set.realized_variance * feature_set.realized_variance;
    feature_sums_2_long["directional_volatility"] += feature_set.directional_volatility * feature_set.directional_volatility;
    feature_sums_2_long["spread_volatility"] += feature_set.spread_volatility * feature_set.spread_volatility;
    feature_sums_2_long["ofi"] += feature_set.ofi * feature_set.ofi;
    feature_sums_2_long["signed_volume_pressure"] += feature_set.signed_volume_pressure * feature_set.signed_volume_pressure;
    feature_sums_2_long["order_arrival_rate"] += feature_set.order_arrival_rate * feature_set.order_arrival_rate;
    feature_sums_2_long["tick_direction_entropy"] += feature_set.tick_direction_entropy * feature_set.tick_direction_entropy;
    feature_sums_2_long["depth_imbalance"] += feature_set.depth_imbalance * feature_set.depth_imbalance;
    feature_sums_2_long["market_depth"] += feature_set.market_depth * feature_set.market_depth;
    feature_sums_2_long["lob_slope"] += feature_set.lob_slope * feature_set.lob_slope;
    feature_sums_2_long["price_gap"] += feature_set.price_gap * feature_set.price_gap;
    feature_sums_2_long["shannon_entropy"] += feature_set.shannon_entropy * feature_set.shannon_entropy;
    feature_sums_2_long["liquidity_stress"] += feature_set.liquidity_stress * feature_set.liquidity_stress;
    

    if (window_long.size() > LONG_WINDOW_SIZE) {
        FeatureSet prev_feature_set = window_long.front();
        feature_sums_long["log_spread"] -= prev_feature_set.log_spread;
        feature_sums_long["price_impact"] -= prev_feature_set.price_impact;
        feature_sums_long["log_return"] -= prev_feature_set.log_return;
        feature_sums_long["ewm_volatility"] -= prev_feature_set.ewm_volatility;
        feature_sums_long["realized_variance"] -= prev_feature_set.realized_variance;
        feature_sums_long["directional_volatility"] -= prev_feature_set.directional_volatility;
        feature_sums_long["spread_volatility"] -= prev_feature_set.spread_volatility;
        feature_sums_long["ofi"] -= prev_feature_set.ofi;
        feature_sums_long["signed_volume_pressure"] -= prev_feature_set.signed_volume_pressure;
        feature_sums_long["order_arrival_rate"] -= prev_feature_set.order_arrival_rate;
        feature_sums_long["tick_direction_entropy"] -= prev_feature_set.tick_direction_entropy;
        feature_sums_long["depth_imbalance"] -= prev_feature_set.depth_imbalance;
        feature_sums_long["market_depth"] -= prev_feature_set.market_depth;
        feature_sums_long["lob_slope"] -= prev_feature_set.lob_slope;
        feature_sums_long["price_gap"] -= prev_feature_set.price_gap;
        feature_sums_long["shannon_entropy"] -= prev_feature_set.shannon_entropy;
        feature_sums_long["liquidity_stress"] -= prev_feature_set.liquidity_stress;
        
        feature_sums_2_long["log_spread"] -= prev_feature_set.log_spread * prev_feature_set.log_spread;
        feature_sums_2_long["price_impact"] -= prev_feature_set.price_impact * prev_feature_set.price_impact;
        feature_sums_2_long["log_return"] -= prev_feature_set.log_return * prev_feature_set.log_return;
        feature_sums_2_long["ewm_volatility"] -= prev_feature_set.ewm_volatility * prev_feature_set.ewm_volatility;
        feature_sums_2_long["realized_variance"] -= prev_feature_set.realized_variance * prev_feature_set.realized_variance;
        feature_sums_2_long["directional_volatility"] -= prev_feature_set.directional_volatility * prev_feature_set.directional_volatility;
        feature_sums_2_long["spread_volatility"] -= prev_feature_set.spread_volatility * prev_feature_set.spread_volatility;
        feature_sums_2_long["ofi"] -= prev_feature_set.ofi * prev_feature_set.ofi;
        feature_sums_2_long["signed_volume_pressure"] -= prev_feature_set.signed_volume_pressure * prev_feature_set.signed_volume_pressure;
        feature_sums_2_long["order_arrival_rate"] -= prev_feature_set.order_arrival_rate * prev_feature_set.order_arrival_rate;
        feature_sums_2_long["tick_direction_entropy"] -= prev_feature_set.tick_direction_entropy * prev_feature_set.tick_direction_entropy;
        feature_sums_2_long["depth_imbalance"] -= prev_feature_set.depth_imbalance * prev_feature_set.depth_imbalance;
        feature_sums_2_long["market_depth"] -= prev_feature_set.market_depth * prev_feature_set.market_depth;
        feature_sums_2_long["lob_slope"] -= prev_feature_set.lob_slope * prev_feature_set.lob_slope;
        feature_sums_2_long["price_gap"] -= prev_feature_set.price_gap * prev_feature_set.price_gap;
        feature_sums_2_long["shannon_entropy"] -= prev_feature_set.shannon_entropy * prev_feature_set.shannon_entropy;
        feature_sums_2_long["liquidity_stress"] -= prev_feature_set.liquidity_stress * prev_feature_set.liquidity_stress;
        window_long.pop_front();
    }

    window_short.push_back(feature_set);

    feature_sums_short["log_spread"] += feature_set.log_spread;
    feature_sums_short["price_impact"] += feature_set.price_impact;
    feature_sums_short["log_return"] += feature_set.log_return;
    feature_sums_short["ewm_volatility"] += feature_set.ewm_volatility;
    feature_sums_short["realized_variance"] += feature_set.realized_variance;
    feature_sums_short["directional_volatility"] += feature_set.directional_volatility;
    feature_sums_short["spread_volatility"] += feature_set.spread_volatility;
    feature_sums_short["ofi"] += feature_set.ofi;
    feature_sums_short["signed_volume_pressure"] += feature_set.signed_volume_pressure;
    feature_sums_short["order_arrival_rate"] += feature_set.order_arrival_rate;
    feature_sums_short["tick_direction_entropy"] += feature_set.tick_direction_entropy;
    feature_sums_short["depth_imbalance"] += feature_set.depth_imbalance;
    feature_sums_short["market_depth"] += feature_set.market_depth;
    feature_sums_short["lob_slope"] += feature_set.lob_slope;
    feature_sums_short["price_gap"] += feature_set.price_gap;
    feature_sums_short["shannon_entropy"] += feature_set.shannon_entropy;
    feature_sums_short["liquidity_stress"] += feature_set.liquidity_stress;

    feature_sums_2_short["log_spread"] += feature_set.log_spread * feature_set.log_spread;
    feature_sums_2_short["price_impact"] += feature_set.price_impact * feature_set.price_impact;
    feature_sums_2_short["log_return"] += feature_set.log_return * feature_set.log_return;
    feature_sums_2_short["ewm_volatility"] += feature_set.ewm_volatility * feature_set.ewm_volatility;
    feature_sums_2_short["realized_variance"] += feature_set.realized_variance * feature_set.realized_variance;
    feature_sums_2_short["directional_volatility"] += feature_set.directional_volatility * feature_set.directional_volatility;
    feature_sums_2_short["spread_volatility"] += feature_set.spread_volatility * feature_set.spread_volatility;
    feature_sums_2_short["ofi"] += feature_set.ofi * feature_set.ofi;
    feature_sums_2_short["signed_volume_pressure"] += feature_set.signed_volume_pressure * feature_set.signed_volume_pressure;
    feature_sums_2_short["order_arrival_rate"] += feature_set.order_arrival_rate * feature_set.order_arrival_rate;
    feature_sums_2_short["tick_direction_entropy"] += feature_set.tick_direction_entropy * feature_set.tick_direction_entropy;
    feature_sums_2_short["depth_imbalance"] += feature_set.depth_imbalance * feature_set.depth_imbalance;
    feature_sums_2_short["market_depth"] += feature_set.market_depth * feature_set.market_depth;
    feature_sums_2_short["lob_slope"] += feature_set.lob_slope * feature_set.lob_slope;
    feature_sums_2_short["price_gap"] += feature_set.price_gap * feature_set.price_gap;
    feature_sums_2_short["shannon_entropy"] += feature_set.shannon_entropy * feature_set.shannon_entropy;
    feature_sums_2_short["liquidity_stress"] += feature_set.liquidity_stress * feature_set.liquidity_stress;

    if (window_short.size() > SHORT_WINDOW_SIZE) {
        FeatureSet prev_feature_set = window_short.front();
        feature_sums_short["log_spread"] -= prev_feature_set.log_spread;
        feature_sums_short["price_impact"] -= prev_feature_set.price_impact;
        feature_sums_short["log_return"] -= prev_feature_set.log_return;
        feature_sums_short["ewm_volatility"] -= prev_feature_set.ewm_volatility;
        feature_sums_short["realized_variance"] -= prev_feature_set.realized_variance;
        feature_sums_short["directional_volatility"] -= prev_feature_set.directional_volatility;
        feature_sums_short["spread_volatility"] -= prev_feature_set.spread_volatility;
        feature_sums_short["ofi"] -= prev_feature_set.ofi;
        feature_sums_short["signed_volume_pressure"] -= prev_feature_set.signed_volume_pressure;
        feature_sums_short["order_arrival_rate"] -= prev_feature_set.order_arrival_rate;
        feature_sums_short["tick_direction_entropy"] -= prev_feature_set.tick_direction_entropy;
        feature_sums_short["depth_imbalance"] -= prev_feature_set.depth_imbalance;
        feature_sums_short["market_depth"] -= prev_feature_set.market_depth;
        feature_sums_short["lob_slope"] -= prev_feature_set.lob_slope;
        feature_sums_short["price_gap"] -= prev_feature_set.price_gap;
        feature_sums_short["shannon_entropy"] -= prev_feature_set.shannon_entropy;
        feature_sums_short["liquidity_stress"] -= prev_feature_set.liquidity_stress;

        feature_sums_2_short["log_spread"] -= prev_feature_set.log_spread * prev_feature_set.log_spread;
        feature_sums_2_short["price_impact"] -= prev_feature_set.price_impact * prev_feature_set.price_impact;
        feature_sums_2_short["log_return"] -= prev_feature_set.log_return * prev_feature_set.log_return;
        feature_sums_2_short["ewm_volatility"] -= prev_feature_set.ewm_volatility * prev_feature_set.ewm_volatility;
        feature_sums_2_short["realized_variance"] -= prev_feature_set.realized_variance * prev_feature_set.realized_variance;
        feature_sums_2_short["directional_volatility"] -= prev_feature_set.directional_volatility * prev_feature_set.directional_volatility;
        feature_sums_2_short["spread_volatility"] -= prev_feature_set.spread_volatility * prev_feature_set.spread_volatility;
        feature_sums_2_short["ofi"] -= prev_feature_set.ofi * prev_feature_set.ofi;
        feature_sums_2_short["signed_volume_pressure"] -= prev_feature_set.signed_volume_pressure * prev_feature_set.signed_volume_pressure;
        feature_sums_2_short["order_arrival_rate"] -= prev_feature_set.order_arrival_rate * prev_feature_set.order_arrival_rate;
        feature_sums_2_short["tick_direction_entropy"] -= prev_feature_set.tick_direction_entropy * prev_feature_set.tick_direction_entropy;
        feature_sums_2_short["depth_imbalance"] -= prev_feature_set.depth_imbalance * prev_feature_set.depth_imbalance;
        feature_sums_2_short["market_depth"] -= prev_feature_set.market_depth * prev_feature_set.market_depth;
        feature_sums_2_short["lob_slope"] -= prev_feature_set.lob_slope * prev_feature_set.lob_slope;
        feature_sums_2_short["price_gap"] -= prev_feature_set.price_gap * prev_feature_set.price_gap;
        feature_sums_2_short["shannon_entropy"] -= prev_feature_set.shannon_entropy * prev_feature_set.shannon_entropy;
        feature_sums_2_short["liquidity_stress"] -= prev_feature_set.liquidity_stress * prev_feature_set.liquidity_stress;

        window_short.pop_front();
    }
}

std::pair<FeatureSet, FeatureSet> FeatureNormalizer::NormalizeFeatureSet(const FeatureSet& feature_set) {
    FeatureSet normalized_feature_set_long;
    FeatureSet normalized_feature_set_short;
    
    // Make the long window normalized feature set
    normalized_feature_set_long.timestamp_ns = feature_set.timestamp_ns;
    normalized_feature_set_long.instrument = feature_set.instrument;
    normalized_feature_set_long.reversal_rate = feature_set.reversal_rate;
    normalized_feature_set_long.spread_crossing = feature_set.spread_crossing;
    normalized_feature_set_long.aggressor_bias = feature_set.aggressor_bias;

    auto safe_zscore = [](double x, double sum, double sum2, size_t n, const std::string& field) {
        double mean = sum / n;
        double variance = (sum2 / n) - (mean * mean);
        if (variance <= 0.0) {
            std::cout << "Warning: Variance is non-positive. Setting to 1.0. Field: " << field << std::endl;
        }
        double stddev = (variance > 0.0) ? std::sqrt(variance) : 1.0;
        return (x - mean) / stddev;
    };

    double n_long = static_cast<double>(window_long.size());
    double sum, sum2;

    #define CALC_ZSCORE(field) \
        sum = feature_sums_long[#field]; \
        sum2 = feature_sums_2_long[#field]; \
        normalized_feature_set_long.field = safe_zscore(feature_set.field, sum, sum2, n_long, #field);

    
    CALC_ZSCORE(log_spread);
    CALC_ZSCORE(price_impact);
    CALC_ZSCORE(log_return);
    CALC_ZSCORE(ewm_volatility);
    CALC_ZSCORE(realized_variance);
    CALC_ZSCORE(directional_volatility);
    CALC_ZSCORE(spread_volatility);
    CALC_ZSCORE(ofi);
    CALC_ZSCORE(signed_volume_pressure);
    CALC_ZSCORE(order_arrival_rate);
    CALC_ZSCORE(tick_direction_entropy);
    CALC_ZSCORE(depth_imbalance);
    CALC_ZSCORE(market_depth);
    CALC_ZSCORE(lob_slope);
    CALC_ZSCORE(price_gap);
    CALC_ZSCORE(shannon_entropy);
    CALC_ZSCORE(liquidity_stress);

    #undef CALC_ZSCORE

    // Make the short window normalized feature set
    normalized_feature_set_short.timestamp_ns = feature_set.timestamp_ns;
    normalized_feature_set_short.instrument = feature_set.instrument;
    normalized_feature_set_short.reversal_rate = feature_set.reversal_rate;
    normalized_feature_set_short.spread_crossing = feature_set.spread_crossing;
    normalized_feature_set_short.aggressor_bias = feature_set.aggressor_bias;

    double n_short = static_cast<double>(window_short.size());

    #define CALC_ZSCORE_SHORT(field) \
        sum = feature_sums_short[#field]; \
        sum2 = feature_sums_2_short[#field]; \
        normalized_feature_set_short.field = safe_zscore(feature_set.field, sum, sum2, n_short, #field);

    CALC_ZSCORE_SHORT(log_spread);
    CALC_ZSCORE_SHORT(price_impact);
    CALC_ZSCORE_SHORT(log_return);
    CALC_ZSCORE_SHORT(ewm_volatility);
    CALC_ZSCORE_SHORT(realized_variance);
    CALC_ZSCORE_SHORT(directional_volatility);
    CALC_ZSCORE_SHORT(spread_volatility);
    CALC_ZSCORE_SHORT(ofi);
    CALC_ZSCORE_SHORT(signed_volume_pressure);
    CALC_ZSCORE_SHORT(order_arrival_rate);
    CALC_ZSCORE_SHORT(tick_direction_entropy);
    CALC_ZSCORE_SHORT(depth_imbalance);
    CALC_ZSCORE_SHORT(market_depth);
    CALC_ZSCORE_SHORT(lob_slope);
    CALC_ZSCORE_SHORT(price_gap);
    CALC_ZSCORE_SHORT(shannon_entropy);
    CALC_ZSCORE_SHORT(liquidity_stress);

    #undef CALC_ZSCORE_SHORT

    return std::make_pair(normalized_feature_set_long, normalized_feature_set_short);
}

} // namespace microregime