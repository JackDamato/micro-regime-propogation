#pragma once

#include "feature_set.hpp"
#include "feature_snapshot.hpp"

namespace microregime {

class FeatureProcessor {
public:
    FeatureProcessor() = default;
    ~FeatureProcessor() = default;

    static FeatureSet GetFeatureSet();
private:
    void ProcessSnapshot(const FeatureInputSnapshot& snapshot);
    void ProcessPriceAndSpread(const FeatureInputSnapshot& snapshot);
    void ProcessVolatility(const FeatureInputSnapshot& snapshot);
    void ProcessOrderFlow(const FeatureInputSnapshot& snapshot);
    void ProcessLiquidity(const FeatureInputSnapshot& snapshot);
    void ProcessMicrostructureTransitions(const FeatureInputSnapshot& snapshot);
    void ProcessEngineeredFeatures(const FeatureInputSnapshot& snapshot);
};

} // namespace microregime
