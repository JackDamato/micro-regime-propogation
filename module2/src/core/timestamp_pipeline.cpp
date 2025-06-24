#include "timestamp_pipeline.hpp"
#include "feature_engine.hpp"
#include "feature_processor.hpp"
#include "feature_snapshot.hpp"
#include "feature_set.hpp"
#include "feature_normalizer.hpp"
#include "event_parser.hpp"
#include "order_engine.hpp"
#include "order_book.hpp"
#include "data_reciever.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace microregime {

TimestampPipeline::TimestampPipeline(const std::string& timestamp,
                                     const std::string& base_asset,
                                     const std::string& future,
                                     const FeatureCallback& callback)
    : timestamp_{timestamp},
      base_asset_{base_asset},
      future_{future},
      callback_{callback},
      order_engine_{},
      parser_base_(construct_parser(base_asset_, timestamp_)),
      parser_future_(construct_parser(future_, timestamp_)),
      feature_engine_base_{const_cast<OrderBookManager&>(order_engine_.get_or_create_order_book(base_asset_)), base_asset_},
      feature_engine_future_{const_cast<OrderBookManager&>(order_engine_.get_or_create_order_book(future_)), future_},
      feature_processor_base_{},
      feature_processor_future_{} {}

TimestampPipeline::~TimestampPipeline() = default;

void TimestampPipeline::run(uint64_t snapshot_interval_ns,
    DataReciever& base_data_reciever,
    DataReciever& future_data_reciever) {
    
    auto base_opt = parser_base_.get_next_event();
    auto future_opt = parser_future_.get_next_event();

    if (!base_opt || !future_opt) return;

    MarketEvent base_event = *base_opt;
    MarketEvent future_event = *future_opt;

    uint64_t next_snapshot_time = std::max(base_event.timestamp_ns, future_event.timestamp_ns) + 100'000'000'000; // 100 seconds

    while (base_opt && future_opt) {
        if (base_event.timestamp_ns <= future_event.timestamp_ns) {
            order_engine_.process_event(base_event, &feature_engine_base_);
            base_opt = parser_base_.get_next_event();
            if (!base_opt) break;
            base_event = *base_opt;
        } else {
            order_engine_.process_event(future_event, &feature_engine_future_);
            future_opt = parser_future_.get_next_event();
            if (!future_opt) break;
            future_event = *future_opt;
        }

        if (base_event.timestamp_ns >= next_snapshot_time &&
            future_event.timestamp_ns >= next_snapshot_time) {
            // --- BASE asset snapshot ---
            FeatureInputSnapshot base_snapshot = feature_engine_base_.generate_snapshot();
            FeatureSet base_raw = feature_processor_base_.GetRawFeatureSet(base_snapshot);
            auto [base_norm_long, base_norm_short] = feature_processor_base_.GetProcessedFeatureSets(base_raw);
            base_data_reciever.ingest_feature_set(base_asset_, next_snapshot_time,
                         base_raw, base_norm_short, base_norm_long);

            // --- FUTURE asset snapshot ---
            FeatureInputSnapshot future_snapshot = feature_engine_future_.generate_snapshot();
            FeatureSet fut_raw = feature_processor_future_.GetRawFeatureSet(future_snapshot);
            auto [fut_norm_long, fut_norm_short] = feature_processor_future_.GetProcessedFeatureSets(fut_raw);
            future_data_reciever.ingest_feature_set(future_, next_snapshot_time,
                           fut_raw, fut_norm_short, fut_norm_long);

            next_snapshot_time += snapshot_interval_ns;
        }
    }
}


EventParser TimestampPipeline::construct_parser(const std::string& instrument, const std::string& timestamp) {
    std::string file = (instrument == base_asset_)
        ? "xnas-itch-" + timestamp + ".mbo.dbn.zst"
        : "glbx-mdp3-" + timestamp + ".mbo.dbn.zst";
    fs::path path = fs::path("..") / ".." / "data" / instrument / file;
    if (!fs::exists(path)) {
        path = fs::path("..") / "data" / instrument / file;
    }
    return EventParser(path.string(), instrument);
}

} // namespace microregime
