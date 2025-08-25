#include "feature_processor.hpp"
#include "feature_set.hpp"
#include "common_constants.hpp"
#include "feature_snapshot.hpp"
#include <cmath>
#include <numeric>
#include <utility>
#include <algorithm>
#include <iostream>

namespace microregime {

FeatureSet FeatureProcessor::GetRawFeatureSet(const FeatureInputSnapshot& snapshot) {
    FeatureSet feature_set;
    feature_set.timestamp_ns = snapshot.timestamp_ns;
    feature_set.instrument = snapshot.instrument;
    feature_set.midprice = snapshot.midprice;
    ProcessPriceFeatures(snapshot, feature_set);
    ProcessVolatilityFeatures(snapshot, feature_set);
    ProcessOrderFlowFeatures(snapshot, feature_set);
    ProcessRandomnessFeatures(snapshot, feature_set);
    ProcessEngineeredFeatures(snapshot, feature_set);

    feature_normalizer_.AddFeatureSet(feature_set);
    
    return feature_set;
}

FeatureSet FeatureProcessor::GetProcessedFeatureSet(const FeatureSet& raw_feature_set) {
    return feature_normalizer_.NormalizeFeatureSet(raw_feature_set);
}

// Midprice, ema_midprices, ema_log_returns (5 windows each)
void FeatureProcessor::ProcessPriceFeatures(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // Initialize with default values
    feature_set.ema_midprice_long_long = 0.0;
    feature_set.ema_midprice_long = 0.0;
    feature_set.ema_midprice_medium = 0.0;
    feature_set.ema_midprice_short = 0.0;
    feature_set.ema_midprice_short_short = 0.0;
    feature_set.ema_log_return_long_long = 0.0;
    feature_set.ema_log_return_long = 0.0;
    feature_set.ema_log_return_medium = 0.0;
    feature_set.ema_log_return_short = 0.0;
    feature_set.ema_log_return_short_short = 0.0;
    
    // Check if we have enough data
    if (!snapshot.rolling_midprices || snapshot.rolling_midprices->size() < 2 || 
        !snapshot.rolling_log_returns || snapshot.rolling_log_returns->size() < 2) {
        return;
    }
    
    const auto& midprices = *snapshot.rolling_midprices;
    const auto& log_returns = *snapshot.rolling_log_returns;
    
    // Ensure we don't divide by zero
    if (midprices.size() <= 3) return;
        
    double EMA_LONG_LONG_ALPHA = 2.0 / (LONG_LONG + 1);
    double EMA_LONG_ALPHA = 2.0 / (LONG + 1);
    double EMA_MEDIUM_ALPHA = 2.0 / (MEDIUM + 1);
    double EMA_SHORT_ALPHA = 2.0 / (SHORT + 1);
    double EMA_SHORT_SHORT_ALPHA = 2.0 / (SHORT_SHORT + 1);
    for (size_t i = 1; i < midprices.size(); ++i) {
        feature_set.ema_midprice_long_long = (1 - EMA_LONG_LONG_ALPHA) * feature_set.ema_midprice_long_long + EMA_LONG_LONG_ALPHA * midprices[i];
        feature_set.ema_midprice_long = (1 - EMA_LONG_ALPHA) * feature_set.ema_midprice_long + EMA_LONG_ALPHA * midprices[i];
        feature_set.ema_midprice_medium = (1 - EMA_MEDIUM_ALPHA) * feature_set.ema_midprice_medium + EMA_MEDIUM_ALPHA * midprices[i];
        feature_set.ema_midprice_short = (1 - EMA_SHORT_ALPHA) * feature_set.ema_midprice_short + EMA_SHORT_ALPHA * midprices[i];
        feature_set.ema_midprice_short_short = (1 - EMA_SHORT_SHORT_ALPHA) * feature_set.ema_midprice_short_short + EMA_SHORT_SHORT_ALPHA * midprices[i];
    }

    for (size_t i = 1; i < log_returns.size(); ++i) {
        feature_set.ema_log_return_long_long = (1 - EMA_LONG_LONG_ALPHA) * feature_set.ema_log_return_long_long + EMA_LONG_LONG_ALPHA * log_returns[i];
        feature_set.ema_log_return_long = (1 - EMA_LONG_ALPHA) * feature_set.ema_log_return_long + EMA_LONG_ALPHA * log_returns[i];
        feature_set.ema_log_return_medium = (1 - EMA_MEDIUM_ALPHA) * feature_set.ema_log_return_medium + EMA_MEDIUM_ALPHA * log_returns[i];
        feature_set.ema_log_return_short = (1 - EMA_SHORT_ALPHA) * feature_set.ema_log_return_short + EMA_SHORT_ALPHA * log_returns[i];
        feature_set.ema_log_return_short_short = (1 - EMA_SHORT_SHORT_ALPHA) * feature_set.ema_log_return_short_short + EMA_SHORT_SHORT_ALPHA * log_returns[i];
    }
}


// Realized Variance, Volatility of Variance, Up Volatility, Down Volatility
void FeatureProcessor::ProcessVolatilityFeatures(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // Initialize with default values
    feature_set.realized_variance_long_long = 0.0;
    feature_set.realized_variance_long = 0.0;
    feature_set.realized_variance_medium = 0.0;
    feature_set.realized_variance_short = 0.0;
    feature_set.realized_variance_short_short = 0.0;
    feature_set.vol_of_variance_short_one = 0.0;
    feature_set.vol_of_variance_short_two = 0.0;
    feature_set.vol_of_variance_short_five = 0.0;
    feature_set.vol_of_variance_long_one = 0.0;
    feature_set.vol_of_variance_long_two = 0.0;
    feature_set.vol_of_variance_long_five = 0.0;
    feature_set.up_volatility_long_long = 0.0;
    feature_set.up_volatility_long = 0.0;
    feature_set.up_volatility_medium = 0.0;
    feature_set.up_volatility_short = 0.0;
    feature_set.up_volatility_short_short = 0.0;
    feature_set.down_volatility_long_long = 0.0;
    feature_set.down_volatility_long = 0.0;
    feature_set.down_volatility_medium = 0.0;
    feature_set.down_volatility_short = 0.0;
    feature_set.down_volatility_short_short = 0.0;
    
    // Check if we have enough data
    if (!snapshot.rolling_midprices || snapshot.rolling_midprices->empty() || 
        !snapshot.rolling_log_returns || snapshot.rolling_log_returns->empty()) {
        return;
    }
    
    const auto& midprices = *snapshot.rolling_midprices;
    const auto& log_returns = *snapshot.rolling_log_returns;

    // Ensure midprices and spreads have the same size and at least 2 elements
    if (midprices.size() < 2 || midprices.size() != log_returns.size()) {
        return;
    }

    // Realized Variances, and Directional Volatility
    double up_var = 0.0, down_var = 0.0;
    int up_count = 0, down_count = 0;
    double realized_variance = 0.0;
    int count = 0;

    double realized_variance_half = 0.0;
    int half_window = 0;
    for (size_t i = 1; i < midprices.size(); ++i) {
        if (midprices[i - 1] == 0.0 || midprices[i] == 0.0) continue;
        double ret = std::log(midprices[i]) - std::log(midprices[i - 1]);
        realized_variance += ret * ret;
        count ++;
        // For short window variance
        if (i > midprices.size() / 2) {
            realized_variance_half += ret * ret;
            half_window ++;
        }
        // For directional volatility
        if (ret > 0) {
            up_var += ret * ret;
            up_count++;
        } else {
            down_var += ret * ret;
            down_count++;
        }
    }
    feature_set.realized_variance = (count > 0) ? realized_variance / count : 0.0;

    
    double avg_up_var = (up_count > 0) ? up_var / up_count : 0.0;
    double avg_down_var = (down_count > 0) ? down_var / down_count : 0.0;
    double ratio = (avg_up_var > 0.0 && avg_down_var > 0.0) ? avg_up_var / avg_down_var : 0.0;
    feature_set.directional_volatility = std::sqrt(ratio);
    
    if (cache_.prev_variance > 0.0) {
        double log_diff = std::log(realized_variance) - std::log(cache_.prev_variance);
        cache_.rolling_log_diff_variance.push_back(log_diff);
        cache_.logvar_sum += log_diff;
        cache_.logvar_sum_squared += log_diff * log_diff;
        cache_.logvar_count ++;

        if (cache_.rolling_log_diff_variance.size() > 100) {
            cache_.logvar_sum -= cache_.rolling_log_diff_variance.front();
            cache_.logvar_sum_squared -= cache_.rolling_log_diff_variance.front() * cache_.rolling_log_diff_variance.front();
            cache_.logvar_count --;
            cache_.rolling_log_diff_variance.pop_front();
        }
        if (cache_.logvar_count > 0) {
            feature_set.vol_of_variance = std::sqrt(std::max(0.0, (cache_.logvar_sum_squared / cache_.logvar_count) - 
                                                (cache_.logvar_sum / cache_.logvar_count) * (cache_.logvar_sum / cache_.logvar_count)));
        } else {
            feature_set.vol_of_variance = 0.0;
        }
    } else {
        feature_set.vol_of_variance = 0.0;
    }
    cache_.prev_variance = realized_variance;

    // Spread Volatility
    double mean_spread = 0.0;
    double spread_sum = 0.0;
    for (size_t i = 0; i < spreads.size(); ++i) {
        double spread = (midprices[i] > 0.0) ? spreads[i] / midprices[i] : 0.0;
        spread_sum += spread;
    }
    mean_spread = spread_sum / spreads.size();
    feature_set.spread_mean = mean_spread;
    
    double spread_var = 0.0;
    for (size_t i = 0; i < spreads.size(); ++i) {
        double spread = (midprices[i] > 0.0) ? spreads[i] / midprices[i] : 0.0;
        spread_var += (spread - mean_spread) * (spread - mean_spread);
    }
    spread_var /= spreads.size();
    feature_set.spread_volatility = std::sqrt(spread_var);
}

// Order Flow Imbalance, Add Rate, Trade Rate, Aggressor Ratio (long, medium, short.)
void FeatureProcessor::ProcessOrderFlowFeatures(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // --- Order Flow Imbalance (OFI) ---
    feature_set.ofi = (snapshot.bid_volume > 0.0 && snapshot.ask_volume > 0.0) ? 
            (snapshot.bid_volume - snapshot.ask_volume) / (snapshot.bid_volume + snapshot.ask_volume) : 0.0;

    // --- Add Rate ---
    feature_set.add_rate = (snapshot.order_time_ns > 0.0) ? 
            1'000'000'000 * (snapshot.bid_volume + snapshot.ask_volume) / snapshot.order_time_ns : 0.0;
    
    // --- Trade Rate ---
    feature_set.trade_rate = (snapshot.trade_time_ns > 0.0) ? 
            1'000'000'000 * (snapshot.buy_volume + snapshot.sell_volume) / snapshot.trade_time_ns : 0.0;

    // --- Aggressor Ratio ---
    double aggressor_ratio = (snapshot.buy_volume + snapshot.sell_volume > 0.0) ? 
            snapshot.buy_volume / (snapshot.buy_volume + snapshot.sell_volume) : 0.0;
    feature_set.aggressor_ratio = aggressor_ratio;


    // --- Aggressor Bias ---
    double aggressor_bias = (snapshot.buy_volume + snapshot.sell_volume > 0.0) ? 
            (snapshot.buy_volume - snapshot.sell_volume) / (snapshot.buy_volume + snapshot.sell_volume) : 0.0;
    feature_set.aggressor_bias = aggressor_bias;
}


// Tick Direction Entropy, Trade Direction Entropy, Autocorrelation of Log Returns, Autocorrelation of Trade Directions, Skewness of Log Returns, Kurtosis of Log Returns
void FeatureProcessor::ProcessRandomnessFeatures(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // Initialize with default values
    feature_set.tick_direction_entropy = 0.0;
    feature_set.trade_direction_entropy = 0.0;
    feature_set.autocor_log_return_1 = 0.0;
    feature_set.autocor_log_return_3 = 0.0;
    feature_set.autocor_log_return_5 = 0.0;
    feature_set.autocor_trade_dir_1 = 0.0;
    feature_set.autocor_trade_dir_3 = 0.0;
    feature_set.autocor_trade_dir_5 = 0.0;
    feature_set.skewness_log_returns = 0.0;
    feature_set.kurtosis_log_returns = 0.0;

    // Check if required data is available
    if (!snapshot.rolling_midprices || snapshot.rolling_midprices->size() < 2) {
        return;  // Not enough data to calculate transitions
    }

    const auto& midprices = *snapshot.rolling_midprices;
    int up = 0, down = 0, zero = 0;
    
    // Calculate price direction changes
    for (size_t i = 1; i < midprices.size(); ++i) {
        if (midprices[i] > midprices[i - 1]) ++up;
        else if (midprices[i] < midprices[i - 1]) ++down;
        else ++zero;
    }
    double total = up + down + zero;
    double p_up = 0.0, p_down = 0.0, p_zero = 0.0;
    if (total > 0) {
        p_up = up / total;
        p_down = down / total;
        p_zero = zero / total;
    }
    
    double tick_entropy = 0.0;
    if (p_up > 0) tick_entropy -= p_up * std::log2(p_up);
    if (p_down > 0) tick_entropy -= p_down * std::log2(p_down);
    if (p_zero > 0) tick_entropy -= p_zero * std::log2(p_zero);
    feature_set.tick_direction_entropy = tick_entropy;


    // --- Reversal Rate and Reversal Entropy ---
    if (!snapshot.rolling_trade_directions || snapshot.rolling_trade_directions->size() < 2) {
        return;  // Not enough trade direction data
    }
    
    const auto& dirs = *snapshot.rolling_trade_directions;
    int reversals = 0;
    int up_down = 0;
    int down_up = 0;
    int up_up = 0;
    int down_down = 0;
    for (size_t i = 1; i < dirs.size(); ++i) {
        if (dirs[i] != 0 && dirs[i] == -dirs[i - 1]) {
            ++reversals;
        }
        if (dirs[i] > 0 && dirs[i - 1] < 0) {
            ++down_up;
        }
        if (dirs[i] < 0 && dirs[i - 1] > 0) {
            ++up_down;
        }
        if (dirs[i] > 0 && dirs[i - 1] > 0) {
            ++up_up;
        }
        if (dirs[i] < 0 && dirs[i - 1] < 0) {
            ++down_down;
        }
    }
    feature_set.reversal_rate = (dirs.size() > 1) ? static_cast<double>(reversals) / dirs.size() : 0.0;

    double p_up_down = static_cast<double>(up_down) / (up_down + down_up + up_up + down_down);
    double p_down_up = static_cast<double>(down_up) / (up_down + down_up + up_up + down_down);
    double p_up_up = static_cast<double>(up_up) / (up_up + down_down + up_down + down_up);
    double p_down_down = static_cast<double>(down_down) / (up_up + down_down + up_down + down_up);
    if (p_up_down > 0) feature_set.reversal_entropy -= p_up_down * std::log2(p_up_down);
    if (p_down_up > 0) feature_set.reversal_entropy -= p_down_up * std::log2(p_down_up);
    if (p_up_up > 0) feature_set.reversal_entropy -= p_up_up * std::log2(p_up_up);
    if (p_down_down > 0) feature_set.reversal_entropy -= p_down_down * std::log2(p_down_down);
}

void FeatureProcessor::ProcessEngineeredFeatures(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    feature_set.sharpe_long_long = feature_set.ema_log_return_long_long / std::sqrt(feature_set.realized_variance_long_long);
    feature_set.sharpe_long = feature_set.ema_log_return_long / std::sqrt(feature_set.realized_variance_long);
    feature_set.sharpe_medium = feature_set.ema_log_return_medium / std::sqrt(feature_set.realized_variance_medium);
    feature_set.sharpe_short = feature_set.ema_log_return_short / std::sqrt(feature_set.realized_variance_short);
    feature_set.sharpe_short_short = feature_set.ema_log_return_short_short / std::sqrt(feature_set.realized_variance_short_short);
    
    feature_set.directional_volatility_long_long = 1.0 - feature_set.down_volatility_long_long / feature_set.up_volatility_long_long;
    feature_set.directional_volatility_long = 1.0 - feature_set.down_volatility_long / feature_set.up_volatility_long;
    feature_set.directional_volatility_medium = 1.0 - feature_set.down_volatility_medium / feature_set.up_volatility_medium;
    feature_set.directional_volatility_short = 1.0 - feature_set.down_volatility_short / feature_set.up_volatility_short;
    feature_set.directional_volatility_short_short = 1.0 - feature_set.down_volatility_short_short / feature_set.up_volatility_short_short;

    feature_set.midprice_deviation_long_long = 1.0 - feature_set.midprice / feature_set.ema_midprice_long_long;
    feature_set.midprice_deviation_long = 1.0 - feature_set.midprice / feature_set.ema_midprice_long;
    feature_set.midprice_deviation_medium = 1.0 - feature_set.midprice / feature_set.ema_midprice_medium;
    feature_set.midprice_deviation_short = 1.0 - feature_set.midprice / feature_set.ema_midprice_short;
    feature_set.midprice_deviation_short_short = 1.0 - feature_set.midprice / feature_set.ema_midprice_short_short;

    feature_set.midprice_reversal_rate = feature_set.ema_log_return_short / feature_set.midprice_deviation_long_long;

    feature_set.ofi_ratio_medium_long = feature_set.ofi_medium / feature_set.ofi_long;
    feature_set.ofi_ratio_short_long = feature_set.ofi_short / feature_set.ofi_long;
    feature_set.ofi_ratio_short_medium = feature_set.ofi_short / feature_set.ofi_medium;

    feature_set.trade_rate_diff = feature_set.trade_rate_long - feature_set.trade_rate_short;
    feature_set.add_rate_diff = feature_set.add_rate_long - feature_set.add_rate_short;
    feature_set.aggressor_ratio_diff = feature_set.aggressor_ratio_long - feature_set.aggressor_ratio_short;
    feature_set.tick_direction_entropy_diff = feature_set.tick_direction_entropy_long - feature_set.tick_direction_entropy_short;
    feature_set.trade_direction_entropy_diff = feature_set.trade_direction_entropy_long - feature_set.trade_direction_entropy_short;
}

} // namespace microregime
