#include "feature_processor.hpp"
#include "feature_set.hpp"
#include "common_constants.hpp"
#include "feature_snapshot.hpp"
#include <cmath>
#include <numeric>
#include <utility>

namespace microregime {

FeatureSet FeatureProcessor::GetRawFeatureSet(const FeatureInputSnapshot& snapshot) {
    FeatureSet feature_set;
    feature_set.timestamp_ns = snapshot.timestamp_ns;
    feature_set.instrument = snapshot.instrument;
    ProcessPriceAndSpread(snapshot, feature_set);
    ProcessVolatility(snapshot, feature_set);
    ProcessOrderFlow(snapshot, feature_set);
    ProcessLiquidity(snapshot, feature_set);
    ProcessMicrostructureTransitions(snapshot, feature_set);
    ProcessEngineeredFeatures(snapshot, feature_set);
    
    feature_normalizer_.AddFeatureSet(feature_set);
    
    return feature_set;
}

FeatureSet FeatureProcessor::GetProcessedFeatureSet(const FeatureSet& raw_feature_set) {
    return feature_normalizer_.NormalizeFeatureSet(raw_feature_set);
}

// Log Spread, Price Impact, Log Return: USES CACHE OBJECTS
void FeatureProcessor::ProcessPriceAndSpread(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    feature_set.log_spread = std::log(snapshot.best_ask_price) - std::log(snapshot.best_bid_price);
    double midprice = (snapshot.best_ask_price + snapshot.best_bid_price) / 2;
    feature_set.midprice = midprice;

    double old_midprice = feature_normalizer_.getOldMidprice(10'000'000'000 / SNAPSHOT_INTERVAL_NS);
    if (old_midprice > 0.0) {
        feature_set.log_return = std::log(midprice) - std::log(old_midprice);
    } else {
        feature_set.log_return = 0.0;
    }
}



// EWM Volatility, Realized Variance, Directional Volatility, Spread Volatility: NO CACHE OBJECTS
void FeatureProcessor::ProcessVolatility(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    const auto& midprices = *snapshot.rolling_midprices;
    const auto& spreads = *snapshot.rolling_spreads;

    // Realized Variance
    double realized_variance = 0.0;
    int count = 0;
    for (size_t i = 1; i < midprices.size(); ++i) {
        if (midprices[i - 1] == 0.0 || midprices[i] == 0.0) continue;
        double ret = std::log(midprices[i]) - std::log(midprices[i - 1]);
        realized_variance += ret * ret;
        count ++;
    }
    feature_set.realized_variance = (count > 0) ? realized_variance / count : 0.0;

    // Exponential Weighted Volatility
    constexpr double alpha = 2.0 / (ROLLING_WINDOW + 1); // typical smoothing factor formulae
    double ewm_var = 0.0, ewm_mean = 0.0;
    int ewm_count = 0;
    for (size_t i = 1; i < midprices.size(); ++i) {
        if (midprices[i - 1] == 0.0 || midprices[i] == 0.0) continue;
        double ret = std::log(midprices[i]) - std::log(midprices[i - 1]);
        if (ewm_count == 0) {
            ewm_mean = ret;
            ewm_var = ret * ret;
        } else {
            ewm_var = (1 - alpha) * ewm_var + alpha * (ret * ret);
            ewm_mean = (1 - alpha) * ewm_mean + alpha * ret;
        }
        ewm_count ++;
    }
    feature_set.ewm_volatility = (ewm_count > 0) ? std::sqrt(ewm_var) : 0.0;

    // Directional Volatility
    double up_var = 0.0, down_var = 0.0;
    int up_count = 0, down_count = 0;
    for (size_t i = 1; i < midprices.size(); ++i) {
        if (midprices[i - 1] == 0.0 || midprices[i] == 0.0) continue;
        double ret = std::log(midprices[i]) - std::log(midprices[i - 1]);
        if (ret > 0) {
            up_var += ret * ret;
            up_count++;
        } else {
            down_var += ret * ret;
            down_count++;
        }
    }
    double avg_up_var = (up_count > 0) ? up_var / up_count : 0.0;
    double avg_down_var = (down_count > 0) ? down_var / down_count : 0.0;
    double diff = avg_up_var - avg_down_var;
    feature_set.directional_volatility = std::sqrt(std::abs(diff)) * ((diff >= 0) ? 1.0 : -1.0);

    // Spread Volatility
    double mean_spread = std::accumulate(spreads.begin(), spreads.end(), 0.0) / ROLLING_WINDOW;
    double spread_var = 0.0;
    for (double s : spreads) {
        spread_var += (s - mean_spread) * (s - mean_spread);
    }
    spread_var /= ROLLING_WINDOW;
    feature_set.spread_volatility = std::sqrt(spread_var);
}

// Volume Weighted OFI, Signed Volume Pressure, Order Arrival Rate: USES CACHE OBJECTS
void FeatureProcessor::ProcessOrderFlow(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // --- Improved Order Flow Imbalance (OFI) ---
    // Parameters
    constexpr double DEPTH_DECAY = 0.5;          // exponential decay per level
    constexpr double OFI_SMOOTH_ALPHA = 0.2;     // EMA smoothing
    constexpr double MIN_TOTAL_VOL = 1e-6;       // prevent div-by-zero

    double raw_ofi = 0.0;

    for (int i = 0; i < DEPTH_LEVELS; ++i) {
        double weight = std::exp(-i * DEPTH_DECAY);  // top level = 1.0, decays exponentially
        double bid_contrib = snapshot.bid_depth_change_direction[i] * snapshot.bid_sizes[i] * weight;
        double ask_contrib = snapshot.ask_depth_change_direction[i] * snapshot.ask_sizes[i] * weight;

        raw_ofi += bid_contrib - ask_contrib;
    }

    // --- Normalize by recent trade volume (rolling) ---
    double total_volume = snapshot.rolling_buy_volume + snapshot.rolling_sell_volume;
    double normalized_ofi = (total_volume > MIN_TOTAL_VOL) ? raw_ofi / total_volume : 0.0;

    // --- Optional Smoothing (EMA)
    feature_set.ofi = OFI_SMOOTH_ALPHA * normalized_ofi + (1.0 - OFI_SMOOTH_ALPHA) * cache_.prev_ofi;
    cache_.prev_ofi = feature_set.ofi;

    // --- Signed Volume Pressure ---
    double net_signed_volume = snapshot.rolling_buy_volume - snapshot.rolling_sell_volume;
    feature_set.signed_volume_pressure = (total_volume > 0.0) 
        ? net_signed_volume / total_volume 
        : 0.0;

    // --- Order Arrival Rate ---
    if (cache_.last_arrival_time_ns > 0) {
        uint64_t delta_ns = snapshot.timestamp_ns - cache_.last_arrival_time_ns;
        double seconds = static_cast<double>(delta_ns) * 1e-9;
        feature_set.order_arrival_rate = (seconds > 0.0) 
            ? static_cast<double>(snapshot.adds_since_last_snapshot) / seconds 
            : 0.0;
    } else {
        feature_set.order_arrival_rate = 0.0;
    }
    cache_.last_arrival_time_ns = snapshot.timestamp_ns;
}


// Market Depth, Depth Imbalance, LOB Slope, Price Gap: NO CACHE OBJECTS
void FeatureProcessor::ProcessLiquidity(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    double bid_depth = 0.0, ask_depth = 0.0;
    double bid_weighted = 0.0, ask_weighted = 0.0;

    double mid = (snapshot.best_ask_price + snapshot.best_bid_price) / 2;
    double log_mid = std::log(mid);

    for (size_t i = 0; i < DEPTH_LEVELS; ++i) {
        if (snapshot.bid_prices[i] > 0) {
            double dist = std::abs(log_mid - std::log(snapshot.bid_prices[i]));
            bid_weighted += dist * snapshot.bid_sizes[i];
            bid_depth += snapshot.bid_sizes[i];
        }
        if (snapshot.ask_prices[i] > 0) {
            double dist = std::abs(std::log(snapshot.ask_prices[i]) - log_mid);
            ask_weighted += dist * snapshot.ask_sizes[i];
            ask_depth += snapshot.ask_sizes[i];
        }
    }

    // --- Market Depth ---
    feature_set.market_depth = bid_depth + ask_depth;

    // --- Depth Imbalance ---
    feature_set.depth_imbalance = (bid_depth + ask_depth > 0.0)
        ? (bid_depth - ask_depth) / (bid_depth + ask_depth)
        : 0.0;

    // --- LOB Slope (log-price weighted) ---
    double bid_log_weighted = 0.0, ask_log_weighted = 0.0;
    double bid_sum = 0.0, ask_sum = 0.0;
    
    for (size_t i = 0; i < DEPTH_LEVELS; ++i) {
        if (snapshot.bid_prices[i] > 0) {
            double dist = std::abs(log_mid - std::log(snapshot.bid_prices[i]));
            bid_log_weighted += dist * snapshot.bid_sizes[i];
            bid_sum += snapshot.bid_sizes[i];
        }
        if (snapshot.ask_prices[i] > 0) {
            double dist = std::abs(std::log(snapshot.ask_prices[i]) - log_mid);
            ask_log_weighted += dist * snapshot.ask_sizes[i];
            ask_sum += snapshot.ask_sizes[i];
        }
    }
    
    double bid_slope = (bid_sum > 0.0) ? bid_log_weighted / bid_sum : 0.0;
    double ask_slope = (ask_sum > 0.0) ? ask_log_weighted / ask_sum : 0.0;
    feature_set.lob_slope = bid_slope + ask_slope;

    // --- Price Gap ---
    double vol_weighted_bid_gap = (snapshot.bid_prices[0] * snapshot.bid_sizes[0] - snapshot.bid_prices[1] * snapshot.bid_sizes[1]) 
                       / (snapshot.bid_sizes[0] + snapshot.bid_sizes[1]);
    double vol_weighted_ask_gap = (snapshot.ask_prices[0] * snapshot.ask_sizes[0] - snapshot.ask_prices[1] * snapshot.ask_sizes[1]) 
                       / (snapshot.ask_sizes[0] + snapshot.ask_sizes[1]);
    feature_set.price_gap = vol_weighted_bid_gap + vol_weighted_ask_gap;
}

// Tick Direction Entropy, Reversal Rate, Aggressor Bias: USES CACHE OBJECTS
void FeatureProcessor::ProcessMicrostructureTransitions(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    int up = 0, down = 0, zero = 0;
    for (auto dir : *snapshot.rolling_tick_directions) {
        if (dir > 0) ++up;
        else if (dir < 0) ++down;
        else ++zero;
    }
    double total = up + down + zero;
    double p_up = up / total, p_down = down / total, p_zero = zero / total;

    double tick_entropy = 0.0;
    if (p_up > 0) tick_entropy -= p_up * std::log2(p_up);
    if (p_down > 0) tick_entropy -= p_down * std::log2(p_down);
    if (p_zero > 0) tick_entropy -= p_zero * std::log2(p_zero);
    feature_set.tick_direction_entropy = tick_entropy;

    // --- Reversal Rate ---
    const auto& dirs = *snapshot.rolling_trade_directions;
    int reversals = 0;
    for (size_t i = 1; i < dirs.size(); ++i) {
        if (dirs[i] != 0 && dirs[i] == -dirs[i - 1]) {
            ++reversals;
        }
    }
    feature_set.reversal_rate = (dirs.size() > 1) ? static_cast<double>(reversals) / dirs.size() : 0.0;

    // --- Aggressor Bias ---
    double sum = std::accumulate(
        snapshot.rolling_trade_directions->begin(),
        snapshot.rolling_trade_directions->end(), 0
    );
    double aggressor_bias = static_cast<double>(sum) / snapshot.rolling_trade_directions->size();
    feature_set.aggressor_bias = aggressor_bias;
}


// Shannon Entropy, and Liquidity Stress
void FeatureProcessor::ProcessEngineeredFeatures(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set) {
    // --- Shannon Entropy of Order Flow ---
    int pos = 0, neg = 0;
    for (auto dir : *snapshot.rolling_trade_directions) {
        if (dir > 0) ++pos;
        else if (dir < 0) ++neg;
        // Ignore 0s entirely
    }

    int total = pos + neg;
    double p_pos = total > 0 ? static_cast<double>(pos) / total : 0.0;
    double p_neg = total > 0 ? static_cast<double>(neg) / total : 0.0;

    double entropy = 0.0;
    if (p_pos > 0) entropy -= p_pos * std::log2(p_pos);
    if (p_neg > 0) entropy -= p_neg * std::log2(p_neg);

    feature_set.shannon_entropy = entropy;

    // --- Liquidity Stress Index with Smoothing ---

    // === Parameters ===
    constexpr int LEVELS_TO_USE = 5;
    constexpr double MIN_QUOTE_SIZE = 5.0;
    constexpr double DISTANCE_DECAY = 10.0;
    constexpr double STRESS_SMOOTH_ALPHA = 0.1;  // smoothing factor (0.05–0.2 is reasonable)

    // === Compute Weighted Depth Liquidity ===
    double bid_weighted = 0.0, ask_weighted = 0.0;

    for (int i = 0; i < LEVELS_TO_USE; ++i) {
        double bid_price = snapshot.bid_prices[i];
        double ask_price = snapshot.ask_prices[i];
        int bid_size = snapshot.bid_sizes[i];
        int ask_size = snapshot.ask_sizes[i];

        if (bid_price > 0.0 && bid_size >= MIN_QUOTE_SIZE) {
            double dist = snapshot.best_bid_price - bid_price;
            double weight = std::exp(-dist * DISTANCE_DECAY);
            bid_weighted += weight * bid_size;
        }

        if (ask_price > 0.0 && ask_size >= MIN_QUOTE_SIZE) {
            double dist = ask_price - snapshot.best_ask_price;
            double weight = std::exp(-dist * DISTANCE_DECAY);
            ask_weighted += weight * ask_size;
        }
    }

    double total_weighted_liquidity = bid_weighted + ask_weighted;

    // === Liquidity Stress (Raw & Smoothed) ===
    double raw_liquidity_stress = 0.0;
    if (cache_.prev_liquidity > 0.0) {
        double liquidity_change = (total_weighted_liquidity - cache_.prev_liquidity) / cache_.prev_liquidity;
        raw_liquidity_stress = -liquidity_change;  // drop = stress
    }

    // Smooth stress using EMA (optional: clamp huge swings if needed)
    feature_set.liquidity_stress =
        STRESS_SMOOTH_ALPHA * raw_liquidity_stress +
        (1.0 - STRESS_SMOOTH_ALPHA) * cache_.prev_liquidity_stress;

    // === Update Caches ===
    cache_.prev_liquidity = total_weighted_liquidity;
    cache_.prev_liquidity_stress = feature_set.liquidity_stress;

}

double FeatureProcessor::infer_pre_trade_midprice(const FeatureInputSnapshot& snap) {
    const auto& mids = *snap.rolling_midprices;
    const auto& dirs = *snap.rolling_trade_directions;

    for (int i = static_cast<int>(dirs.size()) - 1; i > 0; --i) {
        if (dirs[i] != 0 && i - 1 < static_cast<int>(mids.size())) {
            double inferred_mid = mids[i - 1];
            return (inferred_mid > 0.0) ? inferred_mid : 0.0;
        }
    }
    return 0.0;
}

} // namespace microregime
