#include "dual_feature_pipeline.hpp"
#include "feature_engine.hpp"
#include "feature_processor.hpp"
#include "feature_snapshot.hpp"
#include "feature_set.hpp"
#include "feature_normalizer.hpp"
#include "event_parser.hpp"
#include "order_engine.hpp"
#include "order_book.hpp"
#include "data_reciever.hpp"
#include "common_constants.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <ctime>
#include <sstream>
#include <cstdlib>
#include <iostream>

namespace fs = std::filesystem;

namespace microregime {

DualFeaturePipeline::DualFeaturePipeline(const std::string& timestamp,
                                     const std::string& base_asset,
                                     const std::string& future)
    : timestamp_{timestamp},
      base_asset_{base_asset},
      future_{future},
      order_engine_{},
      parser_base_(construct_parser(base_asset_, timestamp_)),
      parser_future_(construct_parser(future_, timestamp_)),
      feature_engine_base_{const_cast<OrderBookManager&>(order_engine_.get_or_create_order_book(base_asset_)), base_asset_},
      feature_engine_future_{const_cast<OrderBookManager&>(order_engine_.get_or_create_order_book(future_)), future_},
      feature_processor_base_{},
      feature_processor_future_{} {}

DualFeaturePipeline::~DualFeaturePipeline() = default;

void DualFeaturePipeline::run(uint64_t snapshot_interval_ns,
                            DataReciever& base_data_reciever,
                            DataReciever& future_data_reciever) {
    
    if (snapshot_interval_ns != SNAPSHOT_INTERVAL_NS) {
        std::cerr << "Snapshot interval " << snapshot_interval_ns << " is not equal to default " << SNAPSHOT_INTERVAL_NS << std::endl;
    }

    const uint64_t midprice_update_interval_ns = 50'000'000; // 50 ms
    uint64_t last_midprice_update_time = 0;
    
    auto base_opt = parser_base_.get_next_event();
    auto future_opt = parser_future_.get_next_event();

    if (!base_opt || !future_opt) return;

    MarketEvent base_event = *base_opt;
    MarketEvent future_event = *future_opt;
    
    uint64_t nyseStart = getNYSEStartTime(timestamp_);
    uint64_t nyseEnd = getNYSEEndTime(timestamp_);
    uint64_t next_snapshot_time = nyseStart + 100'000'000'000; // 100 seconds after market open begin
    
    std::cout << "NYSE Start Time: " << getNYSEStartTime(timestamp_) << std::endl;
    std::cout << "NYSE End Time: " << getNYSEEndTime(timestamp_) << std::endl;
    last_midprice_update_time = std::min(base_event.timestamp_ns, future_event.timestamp_ns);

    while (base_opt && future_opt) {
        uint64_t current_event_time = std::min(base_event.timestamp_ns, future_event.timestamp_ns);
        // Calculate how many 50ms intervals have passed since last update
        if (last_midprice_update_time > 0) {
            uint64_t time_since_last_update = current_event_time - last_midprice_update_time;
            uint64_t intervals_passed = time_since_last_update / midprice_update_interval_ns;
            
            if (intervals_passed > 0) {
                for (uint64_t i = 1; i <= intervals_passed; ++i) {
                    uint64_t update_time = last_midprice_update_time + (i * midprice_update_interval_ns);
                    if (update_time > nyseEnd) break;
                    
                    // Update midprice and spread at each 50ms interval
                    feature_engine_base_.UpdateMidpriceAndSpread(order_engine_.get_order_book(base_asset_).GetMidPrice(), 
                                                                order_engine_.get_order_book(base_asset_).GetSpread());
                    feature_engine_future_.UpdateMidpriceAndSpread(order_engine_.get_order_book(future_).GetMidPrice(), 
                                                                order_engine_.get_order_book(future_).GetSpread());
                }
                last_midprice_update_time += intervals_passed * midprice_update_interval_ns;
            }
        }

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
        if (next_snapshot_time > nyseEnd) {
            break;
        } 
        if (base_event.timestamp_ns >= next_snapshot_time &&
            future_event.timestamp_ns >= next_snapshot_time &&
            next_snapshot_time > nyseStart) {
            // --- BASE asset snapshot ---
            FeatureInputSnapshot base_snapshot = feature_engine_base_.generate_snapshot();
            FeatureSet base_raw = feature_processor_base_.GetRawFeatureSet(base_snapshot);
            auto base_norm = feature_processor_base_.GetProcessedFeatureSet(base_raw);
            base_data_reciever.ingest_feature_set(base_asset_, next_snapshot_time, base_raw, base_norm);

            // --- FUTURE asset snapshot ---
            FeatureInputSnapshot future_snapshot = feature_engine_future_.generate_snapshot();
            FeatureSet fut_raw = feature_processor_future_.GetRawFeatureSet(future_snapshot);
            auto fut_norm = feature_processor_future_.GetProcessedFeatureSet(fut_raw);
            future_data_reciever.ingest_feature_set(future_, next_snapshot_time, fut_raw, fut_norm);

            next_snapshot_time += snapshot_interval_ns;
        }
    }
    if (base_opt) {
        std::cout << "Base events left: " << base_opt->timestamp_ns << std::endl;
    } 
    if (future_opt) {
        std::cout << "Future events left: " << future_opt->timestamp_ns << std::endl;
    }
}


EventParser DualFeaturePipeline::construct_parser(const std::string& instrument, const std::string& timestamp) {
    std::string file = (instrument == base_asset_)
        ? "xnas-itch-" + timestamp + ".mbo.dbn.zst"
        : "glbx-mdp3-" + timestamp + ".mbo.dbn.zst";
    fs::path path = fs::path("..") / ".." / "data" / instrument / file;
    if (!fs::exists(path)) {
        path = fs::path("..") / "data" / instrument / file;
    }
    return EventParser(path.string(), instrument);
}


#ifdef _WIN32
#define timegm _mkgmtime
#endif


uint64_t DualFeaturePipeline::getNYSEStartTime(const std::string& date_str) {
    std::tm timeinfo = {};
    timeinfo.tm_year = std::stoi(date_str.substr(0, 4)) - 1900;
    timeinfo.tm_mon  = std::stoi(date_str.substr(4, 2)) - 1;
    timeinfo.tm_mday = std::stoi(date_str.substr(6, 2));
    timeinfo.tm_hour = 13; // GMT is +4 from EST
    timeinfo.tm_min  = 30;
    timeinfo.tm_sec  = 0;
    timeinfo.tm_isdst = -1;  // Not using daylight savings

    time_t time_seconds = timegm(&timeinfo); // UTC-safe version of mktime
    return static_cast<uint64_t>(time_seconds) * 1'000'000'000;
}

uint64_t DualFeaturePipeline::getNYSEEndTime(const std::string& date_str) {
    std::tm timeinfo = {};
    timeinfo.tm_year = std::stoi(date_str.substr(0, 4)) - 1900;
    timeinfo.tm_mon  = std::stoi(date_str.substr(4, 2)) - 1;
    timeinfo.tm_mday = std::stoi(date_str.substr(6, 2));
    timeinfo.tm_hour = 20; // GMT is +4 from EST
    timeinfo.tm_min  = 0;
    timeinfo.tm_sec  = 0;
    timeinfo.tm_isdst = -1;  // Not using daylight savings

    time_t time_seconds = timegm(&timeinfo); // UTC-safe version of mktime
    return static_cast<uint64_t>(time_seconds) * 1'000'000'000;
}

} // namespace microregime
