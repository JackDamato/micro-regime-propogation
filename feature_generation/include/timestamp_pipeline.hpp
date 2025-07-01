#pragma once

#include "feature_processor.hpp"
#include "feature_engine.hpp"
#include "feature_set.hpp"
#include "feature_snapshot.hpp"
#include "feature_normalizer.hpp"
#include "event_parser.hpp"
#include "order_engine.hpp"
#include "order_book.hpp"
#include "data_reciever.hpp"
#include "common_constants.hpp"

#include <string>
#include <functional>

namespace microregime {

class TimestampPipeline {
public:
    using FeatureCallback = std::function<void(
        const std::string& symbol,
        uint64_t timestamp_ns,
        const FeatureSet& raw_features,
        const FeatureSet& normalized_features_long,
        const FeatureSet& normalized_features_short
    )>;

    TimestampPipeline(const std::string& timestamp = "YYYYMMDD",
                      const std::string& base_asset = "SPY",
                      const std::string& future = "ES",
                      const FeatureCallback& callback = nullptr);

    ~TimestampPipeline();

    void run(uint64_t snapshot_interval_ns = SNAPSHOT_INTERVAL_NS,
             DataReciever& base_data_reciever = *(DataReciever*)nullptr,
             DataReciever& future_data_reciever = *(DataReciever*)nullptr);

private:
    std::string timestamp_;
    std::string base_asset_;
    std::string future_;
    uint64_t getNYSEStartTime(const std::string& date_str);
    uint64_t getNYSEEndTime(const std::string& date_str);
    FeatureCallback callback_;

    OrderEngine order_engine_;

    EventParser parser_base_;
    EventParser parser_future_;

    FeatureEngine feature_engine_base_;
    FeatureEngine feature_engine_future_;

    FeatureProcessor feature_processor_base_;
    FeatureProcessor feature_processor_future_;

    EventParser construct_parser(const std::string& instrument, const std::string& timestamp);
};

} // namespace microregime
