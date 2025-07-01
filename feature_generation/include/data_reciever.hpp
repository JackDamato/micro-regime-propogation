#pragma once

#include "feature_set.hpp"

namespace microregime {

class DataReciever {
public:
    virtual void ingest_feature_set(
        const std::string& symbol,
        uint64_t timestamp_ns,
        const FeatureSet& raw_features,
        const FeatureSet& normalized_long,
        const FeatureSet& normalized_short
    ) = 0;
    virtual ~DataReciever() = default;
};

} // namespace microregime
