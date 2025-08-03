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
    ProcessPriceAndSpread(snapshot, feature_set);
    ProcessVolatility(snapshot, feature_set);
    ProcessOrderFlow(snapshot, feature_set);
    ProcessLiquidity(snapshot, feature_set);
    ProcessMicrostructureTransitions(snapshot, feature_set);
    
    feature_normalizer_.AddFeatureSet(feature_set);
    
    return feature_set;
}

FeatureSet FeatureProcessor::GetProcessedFeatureSet(const FeatureSet& raw_feature_set) {
    return feature_normalizer_.NormalizeFeatureSet(raw_feature_set);
}

// Mean Log Return, EMA of Midprice full window/half window, Midprice Deviation from EMA full: CACHE OBJECTS
void FeatureProcessor::ProcessPriceAndSpread(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // Initialize with default values
    feature_set.mean_log_return = 0.0;
    feature_set.midprice_ema_ratio = 0.0;
    feature_set.midprice_ema_deviation = 0.0;
    
    // Check if we have enough data
    if (!snapshot.rolling_midprices || snapshot.rolling_midprices->size() < 2) {
        return;
    }
    
    const auto& midprices = *snapshot.rolling_midprices;
    double mean_log_return = 0.0;
    double ema_full = (cache_.prev_ema_full == 0.0) ? midprices[0] : cache_.prev_ema_full;
    double ema_half = (cache_.prev_ema_half == 0.0) ? midprices[0] : cache_.prev_ema_half;
    
    // Ensure we don't divide by zero
    if (midprices.size() <= 3) return;
        
    double EMA_FULL_ALPHA = 1.0 / (midprices.size() - 1);
    double EMA_HALF_ALPHA = 1.0 / (midprices.size() / 2 - 1);
    for (size_t i = 1; i < midprices.size(); ++i) {
        ema_full = (1 - EMA_FULL_ALPHA) * ema_full + EMA_FULL_ALPHA * midprices[i];
        ema_half = (1 - EMA_HALF_ALPHA) * ema_half + EMA_HALF_ALPHA * midprices[i];

        if (midprices[i - 1] == 0.0 || midprices[i] == 0.0) continue;
        double ret = std::log(midprices[i]) - std::log(midprices[i - 1]);
        mean_log_return += ret;
    }
    feature_set.mean_log_return = (midprices.size() > 2) ? mean_log_return / midprices.size() : 0.0;
    feature_set.midprice_ema_ratio = (ema_full > 0.0) ? ema_half / ema_full : 0.0;
    feature_set.midprice_ema_deviation = (ema_full > 0.0) ? (midprices.back() - ema_full) / ema_full : 0.0;
    cache_.prev_ema_full = ema_full;
    cache_.prev_ema_half = ema_half;
}



// Realized Variance (full and half window) and Bias, Volatility of Variance, Directional Volatility, Spread Volatility, Spread Mean: CACHE OBJECTS
void FeatureProcessor::ProcessVolatility(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // Initialize with default values
    feature_set.realized_variance = 0.0;
    feature_set.realized_variance_recent = 0.0;
    feature_set.realized_variance_bias = 0.0;
    feature_set.directional_volatility = 0.0;
    feature_set.vol_of_variance = 0.0;
    feature_set.spread_mean = 0.0;
    feature_set.spread_volatility = 0.0;
    
    // Check if we have enough data
    if (!snapshot.rolling_midprices || snapshot.rolling_midprices->empty() || 
        !snapshot.rolling_spreads || snapshot.rolling_spreads->empty()) {
        return;
    }
    
    const auto& midprices = *snapshot.rolling_midprices;
    const auto& spreads = *snapshot.rolling_spreads;
    
    // Ensure midprices and spreads have the same size and at least 2 elements
    if (midprices.size() < 2 || midprices.size() != spreads.size()) {
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
    feature_set.realized_variance_recent = (half_window > 0) ? realized_variance_half / half_window : 0.0;
    feature_set.realized_variance_bias = (realized_variance > 0.0 && realized_variance_half > 0.0) ? 
                                        (realized_variance - realized_variance_half) / (realized_variance + realized_variance_half) : 0.0;
    
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

// Order Flow Imbalance, Add Rate, Trade Size Entropy: USES CACHE OBJECTS
void FeatureProcessor::ProcessOrderFlow(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // --- Order Flow Imbalance (OFI) ---
    feature_set.ofi = (snapshot.bid_volume > 0.0 && snapshot.ask_volume > 0.0) ? 
            (snapshot.bid_volume - snapshot.ask_volume) / (snapshot.bid_volume + snapshot.ask_volume) : 0.0;

    // --- Add Rate ---
    feature_set.add_rate = (snapshot.order_time_ns > 0.0) ? 
            1'000'000'000 * (snapshot.bid_volume + snapshot.ask_volume) / snapshot.order_time_ns : 0.0;
    
    // --- Trade Rate ---
    feature_set.trade_rate = (snapshot.trade_time_ns > 0.0) ? 
            1'000'000'000 * (snapshot.buy_volume + snapshot.sell_volume) / snapshot.trade_time_ns : 0.0;
    
    // --- Trade Size Entropy ---
    constexpr int NUM_BINS = 10;
    double trade_size_entropy = 0.0;

    if (snapshot.rolling_trade_sizes && !snapshot.rolling_trade_sizes->empty()) {
        const auto& sizes = *snapshot.rolling_trade_sizes;

        // Handle edge case: All sizes equal
        double dynamic_min = *std::min_element(sizes.begin(), sizes.end());
        double dynamic_max = *std::max_element(sizes.begin(), sizes.end());
        if (dynamic_max - dynamic_min < 1e-10) {
            trade_size_entropy = 0.0;  // Perfect predictability
            return;
        }

        // Clamp and ensure non-zero range
        dynamic_min = std::max(dynamic_min, 1e-3);
        if (dynamic_max <= dynamic_min) dynamic_max = dynamic_min + 1.0;

        // Create logarithmic bins
        std::vector<double> bin_edges(NUM_BINS + 1);
        double log_min = std::log10(dynamic_min);
        double log_max = std::log10(dynamic_max);
        double log_step = (log_max - log_min) / NUM_BINS;
        for (int i = 0; i <= NUM_BINS; ++i) {
            bin_edges[i] = std::pow(10.0, log_min + i * log_step);
        }

        // Count bin frequencies (optimized with binary search)
        std::vector<int> counts(NUM_BINS, 0);
        for (double size : sizes) {
            auto it = std::upper_bound(bin_edges.begin(), bin_edges.end(), size);
            if (it != bin_edges.begin() && it != bin_edges.end()) {
                counts[std::distance(bin_edges.begin(), it) - 1]++;
            }
        }

        // Compute entropy
        int total = std::accumulate(counts.begin(), counts.end(), 0);
        if (total > 0) {
            for (int count : counts) {
                if (count > 0) {
                    double p = static_cast<double>(count) / total;
                    trade_size_entropy -= p * std::log2(p);
                }
            }
        }
    }

    feature_set.trade_size_entropy = trade_size_entropy;
}


// Order Book Imbalance, LOB Slopes for bid and ask: NO CACHE OBJECTS
void FeatureProcessor::ProcessLiquidity(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // Initialize with default values
    feature_set.obi = 0.0;
    feature_set.lob_bid_slope_mean = 0.0;
    feature_set.lob_ask_slope_mean = 0.0;
    
    // --- Order Book Imbalance ---
    if (snapshot.rolling_order_book_imbalances && !snapshot.rolling_order_book_imbalances->empty()) {
        const auto& obis = *snapshot.rolling_order_book_imbalances;
        feature_set.obi = std::accumulate(obis.begin(), obis.end(), 0.0) / static_cast<double>(obis.size());
    }
    
    // --- LOB Slope Means ---
    if (snapshot.rolling_lob_bid_slopes && !snapshot.rolling_lob_bid_slopes->empty() &&
        snapshot.rolling_lob_ask_slopes && !snapshot.rolling_lob_ask_slopes->empty()) {
            
        const auto& bid_slopes = *snapshot.rolling_lob_bid_slopes;
        const auto& ask_slopes = *snapshot.rolling_lob_ask_slopes;
        
        feature_set.lob_bid_slope_mean = std::accumulate(
            bid_slopes.begin(), bid_slopes.end(), 0.0) / static_cast<double>(bid_slopes.size());
            
        feature_set.lob_ask_slope_mean = std::accumulate(
            ask_slopes.begin(), ask_slopes.end(), 0.0) / static_cast<double>(ask_slopes.size());
    }
}

// Tick Direction Entropy, Reversal Rate and Reversal Entropy, Aggressor Bias and Aggressor Ratio
void FeatureProcessor::ProcessMicrostructureTransitions(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // Initialize with default values
    feature_set.tick_direction_entropy = 0.0;
    feature_set.reversal_rate = 0.0;
    feature_set.reversal_entropy = 0.0;
    feature_set.aggressor_bias = 0.0;
    feature_set.aggressor_ratio = 0.0;

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
    

    // --- Aggressor Ratio ---
    double aggressor_ratio = (snapshot.buy_volume + snapshot.sell_volume > 0.0) ? 
            snapshot.buy_volume / (snapshot.buy_volume + snapshot.sell_volume) : 0.0;
    feature_set.aggressor_ratio = aggressor_ratio;


    // --- Aggressor Bias ---
    double aggressor_bias = (snapshot.buy_volume + snapshot.sell_volume > 0.0) ? 
            (snapshot.buy_volume - snapshot.sell_volume) / (snapshot.buy_volume + snapshot.sell_volume) : 0.0;
    feature_set.aggressor_bias = aggressor_bias;
}

} // namespace microregime
