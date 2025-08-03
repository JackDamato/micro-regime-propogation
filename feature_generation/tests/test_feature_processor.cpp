#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <dbn_reader.hpp>
#include <event_parser.hpp>
#include <order_engine.hpp>
#include <order_book.hpp>
#include <feature_engine.hpp>
#include <feature_snapshot.hpp>
#include <feature_processor.hpp>
#include <feature_set.hpp>
#include <feature_map.hpp>

namespace fs = std::filesystem;
using namespace microregime;

// Helper function to print a FeatureSet
void print_feature_set(const FeatureSet& fs, const std::string& title) {
    std::cout << "\n\n\n=== " << title << " ===\n";
    std::cout << "Timestamp: " << fs.timestamp_ns << " ns\n";
    std::cout << "Symbol: " << fs.instrument << "\n";
    
    for (const auto& feature : kFeatures) {
        std::cout << feature.first << ": " << fs.*feature.second << "\n";
    }
}


class FeatureProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Path to the test data file
        test_file_ = fs::path("..") / ".." / "data" / "ES" / "glbx-mdp3-20250505.mbo.dbn.zst";
        
        // Fallback path if running from build directory
        if (!fs::exists(test_file_)) {
            test_file_ = fs::path("..") / "data" / "ES" / "glbx-mdp3-20250505.mbo.dbn.zst";
            if (!fs::exists(test_file_)) {
                GTEST_SKIP() << "Test file not found at either location:"
                           << "\n  " << fs::absolute("../../data/ES/glbx-mdp3-20250505.mbo.dbn.zst")
                           << "\n  " << fs::absolute("../data/ES/glbx-mdp3-20250505.mbo.dbn.zst");
            }
        }
        
        instrument_ = "ES";
        std::cout << "Testing with file: " << fs::absolute(test_file_) << std::endl;
    }
    
    fs::path test_file_;
    std::string instrument_;
};

TEST_F(FeatureProcessorTest, ProcessSnapshots) {
    // 1. Create the components
    OrderEngine order_engine;
    EventParser parser(test_file_.string(), instrument_);
    
    // 2. Initialize feature engine and processor
    FeatureEngine feature_engine(const_cast<OrderBookManager&>(order_engine.get_or_create_order_book(instrument_)), instrument_);
    FeatureProcessor feature_processor;
    
    // 3. Process events and collect snapshots
    const size_t max_events = 100000000;      // Process first N events
    const size_t snapshot_interval = 1000000000000; // Take snapshot every 1s
    const uint64_t ROLLING_UPDATE_INTERVAL_NS = 100000000; // 100ms update interval
    size_t event_count = 0;
    size_t snapshot_count = 0;
    
    std::cout << "Starting feature processor test...\n";
    int64_t prev_timestamp = 0;
    int64_t last_rolling_update_time = 0;
    
    while (parser.has_more_events() && event_count < max_events) {
        // Process next event
        bool processed = parser.process_next(order_engine, &feature_engine);
        if (!processed) break;
        
        event_count++;
        
        // Update rolling stats at regular intervals
        if (last_rolling_update_time > 0) {
            uint64_t current_time = feature_engine.most_recent_timestamp_ns;
            uint64_t time_since_last_update = current_time - last_rolling_update_time;
            uint64_t intervals_passed = time_since_last_update / ROLLING_UPDATE_INTERVAL_NS;
            
            if (intervals_passed > 0) {
                for (uint64_t i = 1; i <= intervals_passed; ++i) {
                    feature_engine.UpdateRollingStats();
                }
                last_rolling_update_time += intervals_passed * ROLLING_UPDATE_INTERVAL_NS;
            }
        } else {
            last_rolling_update_time = feature_engine.most_recent_timestamp_ns;
        }
        
        // Take periodic snapshots
        if (event_count == 0) {
            prev_timestamp = feature_engine.most_recent_timestamp_ns;
        }
        else if (feature_engine.most_recent_timestamp_ns - prev_timestamp >= snapshot_interval) {
            prev_timestamp = feature_engine.most_recent_timestamp_ns;
            
            // Generate snapshot using FeatureEngine
            auto snapshot = feature_engine.generate_snapshot(parser.current_timestamp());
            snapshot_count++;
            
            // Get raw features first
            auto raw_features = feature_processor.GetRawFeatureSet(snapshot);
            raw_features.instrument = instrument_;
            raw_features.timestamp_ns = snapshot.timestamp_ns;
            
            // Then get normalized features
            auto normalized_features = feature_processor.GetProcessedFeatureSet(raw_features);
            
            // Print the feature sets
            std::cout << "\n\n===========================================\n";
            std::cout << "Snapshot #" << snapshot_count << " (Event #" << event_count << ")\n";
            std::cout << "===========================================\n";
            
            print_feature_set(raw_features, "Raw Features");
            // print_feature_set(normalized_features, "Normalized Features");
            
            // Basic validation
            EXPECT_NE(raw_features.timestamp_ns, 0) << "Timestamp should be set";
            
            // Debug output for snapshot data
            std::cout << "\nDebug - Snapshot data:\n";
            std::cout << "- rolling_midprices: " << (snapshot.rolling_midprices ? "valid" : "null") 
                      << ", size: " << (snapshot.rolling_midprices ? std::to_string(snapshot.rolling_midprices->size()) : "0") 
                      << std::endl;
            
            if (snapshot.rolling_midprices && !snapshot.rolling_midprices->empty()) {
                std::cout << "- Last midprice: " << std::scientific << snapshot.rolling_midprices->back() << std::fixed << std::endl;
                if (snapshot.rolling_midprices->back() <= 0) {
                    std::cout << "WARNING: Invalid midprice detected!" << std::endl;
                }
                EXPECT_GT(snapshot.rolling_midprices->back(), 0) << "Midprice should be positive";
            } else {
                std::cout << "WARNING: No midprice data available!" << std::endl;
            }
        }
    }
    
    std::cout << "\nFeature processor test completed.\n";
    std::cout << "- Processed " << event_count << " events\n";
    std::cout << "- Generated " << snapshot_count << " snapshots\n";
    
    EXPECT_GT(event_count, 0) << "No events were processed";
    EXPECT_GT(snapshot_count, 0) << "No snapshots were generated";
}

// Main function for running the tests
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }