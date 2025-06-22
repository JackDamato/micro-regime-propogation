#pragma once

#include "feature_set.hpp"
#include "feature_snapshot.hpp"
#include "feature_normalizer.hpp"

namespace features {

class FeatureProcessor {
public:
    FeatureProcessor() = default;
    ~FeatureProcessor() = default;

    FeatureSet GetRawFeatureSet(const FeatureInputSnapshot& snapshot);
    std::pair<FeatureSet, FeatureSet> GetProcessedFeatureSets(const FeatureSet& raw_feature_set);
private:
    void ProcessPriceAndSpread(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    void ProcessVolatility(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    void ProcessOrderFlow(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    void ProcessLiquidity(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    void ProcessMicrostructureTransitions(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    void ProcessEngineeredFeatures(const FeatureInputSnapshot& snapshot, FeatureSet& feature_set);
    double infer_pre_trade_midprice(const FeatureInputSnapshot& snap);

    struct Cache {
        // 1-step history
        double prev_midprice = 0.0;
        double prev_liquidity = 0.0;
        uint64_t last_arrival_time_ns = 0;
        // rolling OFI stats (Welford)
        // int      ofi_n   = 0;
        // double   ofi_mean = 0.0;
        // double   ofi_M2   = 0.0;   // sum of squared diffs
        // int    prev_total_depth = 0;
        // double prev_best_bid = 0.0, prev_best_ask = 0.0;

        // 500-window timers
        uint64_t window_start_ts = 0;          // for arrival / cancel rates
    };
    
    Cache cache_;

    FeatureNormalizer feature_normalizer_;
};

} // namespace features
