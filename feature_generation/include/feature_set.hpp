#pragma once

#include <unordered_map>
#include <array>
#include <cstddef>
#include <string>

namespace microregime {

struct FeatureSet {
    uint64_t timestamp_ns;
    std::string instrument;

    // ============================================================
    // =================== PRICING/MOVING AVG =====================
    // ============================================================
    double midprice;
    double ema_midprice_long_long;   // Forgetting factor 2/(1 + LONG_LONG)
    double ema_midprice_long;   // Forgetting factor 2/(1 + LONG)
    double ema_midprice_medium; // forgetting factor 2/(1 + MEDIUM)
    double ema_midprice_short; // Forgetting factor 2/(1 + SHORT)
    double ema_midprice_short_short; // Forgetting factor 2/(1 + SHORT_SHORT)

    double ema_log_return_long_long;   // Forgetting factor 2/(1 + LONG_LONG)
    double ema_log_return_long;   // Forgetting factor 2/(1 + LONG)
    double ema_log_return_medium; // forgetting factor 2/(1 + MEDIUM)
    double ema_log_return_short; // Forgetting factor 2/(1 + SHORT)
    double ema_log_return_short_short; // Forgetting factor 2/(1 + SHORT_SHORT)

    // ============================================================
    // =================== VARIANCE/VOLATILIY =====================
    // ============================================================
    double realized_variance_long_long; // Use long_long window of data
    double realized_variance_long; // Use recent long window of data
    double realized_variance_medium; // Use recent medium window of data
    double realized_variance_short;  // Use recent short window of data
    double realized_variance_short_short; // Use recent short_short window of data

    double vol_of_variance_short_one;
    double vol_of_variance_short_two;
    double vol_of_variance_short_five;
    double vol_of_variance_long_one;
    double vol_of_variance_long_two;
    double vol_of_variance_long_five;

    double up_volatility_long_long; // Use long_long window of data (only up steps)
    double up_volatility_long; // Use recent long window of data
    double up_volatility_medium; // Use recent medium window of data
    double up_volatility_short;  // Use recent short window of data
    double up_volatility_short_short; // Use recent short_short window of data

    double down_volatility_long_long; // Use long_long window of data (only down steps)
    double down_volatility_long; // Use recent long window of data
    double down_volatility_medium; // Use recent medium window of data
    double down_volatility_short;  // Use recent short window of data
    double down_volatility_short_short; // Use recent short_short window of data

    // ============================================================
    // ================== ENTROPY/DISTRIBUTION ====================
    // ============================================================
    double tick_direction_entropy_long_long;
    double tick_direction_entropy_medium;
    double tick_direction_entropy_short_short;

    double trade_direction_entropy_long_long;
    double trade_direction_entropy_medium;
    double trade_direction_entropy_short_short;

    double autocor_log_return_1;
    double autocor_log_return_3;
    double autocor_log_return_5;

    double autocor_trade_dir_1;
    double autocor_trade_dir_3;
    double autocor_trade_dir_5;  

    double skewness_log_returns_long_long;
    double skewness_log_returns_long;
    double skewness_log_returns_medium;

    double kurtosis_log_returns_long_long;
    double kurtosis_log_returns_long;
    double kurtosis_log_returns_medium;

    // ============================================================
    // ======================= ORDER FLOW =========================
    // ============================================================
    double ofi_long;
    double ofi_medium;
    double ofi_short;

    double add_rate_long;
    double add_rate_medium;
    double add_rate_short;

    double trade_rate_long;
    double trade_rate_medium;
    double trade_rate_short;

    double aggressor_ratio_long;
    double aggressor_ratio_medium;
    double aggressor_ratio_short;

    // ============================================================
    // =================== RATIO/DIFFERENCES ======================
    // ============================================================
    // Sharpe = ema_log_return / sqrt(realized_variance)
    double sharpe_long_long; 
    double sharpe_long;
    double sharpe_medium;
    double sharpe_short;
    double sharpe_short_short;

    // Directional_volatility = 1 - down_volatility/up_volatility (should be centered at zero)
    double directional_volatility_long_long;
    double directional_volatility_long;
    double directional_volatility_medium;
    double directional_volatility_short;
    double directional_volatility_short_short;

    // Midprice_deviation = 1 - midprice / ema_midprice
    double midprice_deviation_long_long;
    double midprice_deviation_long;
    double midprice_deviation_medium;
    double midprice_deviation_short;
    double midprice_deviation_short_short;

    double midprice_reversal_rate; // = ema_log_return_short / midprice_deviation_long_long

    // Volatility ratios to measure directionality
    double vol_ratio_long_long_long;
    double vol_ratio_medium_long_long;
    double vol_ratio_short_long_long;
    double vol_ratio_short_short_long_long;
    double vol_ratio_short_medium;
    double vol_ratio_short_short_medium;

    // Log return Ratios to measure second order price movement
    double log_return_ratio_long_long_long;
    double log_return_ratio_medium_long_long;
    double log_return_ratio_short_long_long;
    double log_return_ratio_short_short_long_long;
    double log_return_ratio_short_medium;
    double log_return_ratio_short_short_medium;

    // Order Flow and entropy ratios/differences
    double ofi_ratio_medium_long;
    double ofi_ratio_short_long;
    double ofi_ratio_short_medium;

    double trade_rate_diff; // long - short
    double add_rate_diff; // long - short
    double aggressor_ratio_diff; // long - short
    double tick_direction_entropy_diff; // long - short
    double trade_direction_entropy_diff; // long - short
}; // 98 features here

} // namespace microregime