#pragma once

#include "feature_set.hpp"
#include "feature_snapshot.hpp"
#include "feature_normalizer.hpp"

namespace microregime {

class FeatureProcessor {
public:
    FeatureProcessor() = default;
    ~FeatureProcessor() = default;

    FeatureSet GetRawFeatureSet(const FeatureInputSnapshot& snapshot);
    FeatureSet GetProcessedFeatureSet(const FeatureSet& raw_feature_set);
private:
    void ProcessPriceAndSpread(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    void ProcessVolatility(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    void ProcessOrderFlow(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    void ProcessLiquidity(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    void ProcessMicrostructureTransitions(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);

    struct Cache {
        // Rolling volatility of volatility
        std::deque<double> rolling_log_diff_variance;
        double logvar_sum = 0.0;
        double logvar_sum_squared = 0.0;
        double logvar_count = 0.0;
        double prev_variance = 0.0;

        // EMA Midprices
        double prev_ema_full = 0.0;
        double prev_ema_half = 0.0;
    };
    
    Cache cache_;

    FeatureNormalizer feature_normalizer_;
};

} // namespace microregime
