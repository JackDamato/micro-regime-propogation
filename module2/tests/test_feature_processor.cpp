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

namespace fs = std::filesystem;

// Helper function to print a FeatureSet
void print_feature_set(const features::FeatureSet& fs, const std::string& title) {
    std::cout << "\n\n\n=== " << title << " ===\n";
    std::cout << "Timestamp: " << fs.timestamp_ns << " ns\n";
    std::cout << "Symbol: " << fs.symbol << "\n";
    
    // Price & Spread
    std::cout << "\n--- Price & Spread ---\n";
    std::cout << "Log Spread: " << fs.log_spread << "\n";
    std::cout << "Price Impact: " << fs.price_impact << "\n";
    std::cout << "Log Return: " << fs.log_return << "\n";
    
    // Volatility
    std::cout << "\n--- Volatility ---\n";
    std::cout << "EWM Volatility: " << fs.ewm_volatility << "\n";
    std::cout << "Realized Variance: " << fs.realized_variance << "\n";
    std::cout << "Directional Volatility: " << fs.directional_volatility << "\n";
    std::cout << "Spread Volatility: " << fs.spread_volatility << "\n";
    
    // Order Flow
    std::cout << "\n--- Order Flow ---\n";
    std::cout << "OFI: " << fs.ofi << "\n";
    std::cout << "Signed Volume Pressure: " << fs.signed_volume_pressure << "\n";
    std::cout << "Order Arrival Rate: " << fs.order_arrival_rate << "\n";
    
    // Liquidity
    std::cout << "\n--- Liquidity ---\n";
    std::cout << "Depth Imbalance: " << fs.depth_imbalance << "\n";
    std::cout << "Market Depth: " << fs.market_depth << "\n";
    std::cout << "LOB Slope: " << fs.lob_slope << "\n";
    std::cout << "Price Gap: " << fs.price_gap << "\n";
    
    // Microstructure Transitions
    std::cout << "\n--- Microstructure Transitions ---\n";
    std::cout << "Tick Direction Entropy: " << fs.tick_direction_entropy << "\n";
    std::cout << "Reversal Rate: " << fs.reversal_rate << "\n";
    std::cout << "Spread Crossing: " << fs.spread_crossing << "\n";
    std::cout << "Aggressor Bias: " << fs.aggressor_bias << "\n";
    
    // Engineered Features
    std::cout << "\n--- Engineered Features ---\n";
    std::cout << "Shannon Entropy: " << fs.shannon_entropy << "\n";
    std::cout << "Liquidity Stress: " << fs.liquidity_stress << "\n";
}

class FeatureProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Path to the test data file
        test_file_ = fs::path("..") / ".." / "data" / "SPY" / "xnas-itch-20250505.mbo.dbn.zst";
        
        // Fallback path if running from build directory
        if (!fs::exists(test_file_)) {
            test_file_ = fs::path("..") / "data" / "SPY" / "xnas-itch-20250505.mbo.dbn.zst";
            if (!fs::exists(test_file_)) {
                GTEST_SKIP() << "Test file not found at either location:"
                           << "\n  " << fs::absolute("../../data/SPY/xnas-itch-20250505.mbo.dbn.zst")
                           << "\n  " << fs::absolute("../data/SPY/xnas-itch-20250505.mbo.dbn.zst");
            }
        }
        
        instrument_ = "SPY";
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
    FeatureEngine feature_engine(const_cast<OrderBookManager&>(order_engine.get_or_create_order_book(instrument_)));
    features::FeatureProcessor feature_processor;
    
    // 3. Process events and collect snapshots
    const size_t max_events = 100000000;      // Process first N events
    const size_t snapshot_interval = 1000000000; // Take snapshot every 1s
    size_t event_count = 0;
    size_t snapshot_count = 0;
    
    std::cout << "Starting feature processor test...\n";
    int64_t prev_timestamp = 0;
    while (parser.has_more_events() && event_count < max_events) {
        
        // Process next event
        bool processed = parser.process_next(order_engine, &feature_engine);
        if (!processed) break;
        
        event_count++;
        
        // Take periodic snapshots
        if (event_count == 0) {
            prev_timestamp = feature_engine.most_recent_timestamp_ns;
        }
        else if (feature_engine.most_recent_timestamp_ns - prev_timestamp >= snapshot_interval) {
            prev_timestamp = feature_engine.most_recent_timestamp_ns;
            // Generate snapshot using FeatureEngine
            auto snapshot = feature_engine.generate_snapshot();
            snapshot_count++;
            
            // Get raw features first
            auto raw_features = feature_processor.GetRawFeatureSet(snapshot);
            raw_features.symbol = instrument_;
            raw_features.timestamp_ns = snapshot.timestamp_ns;
            
            // Then get normalized features
            auto [normalized_features_long, normalized_features_short] = feature_processor.GetProcessedFeatureSets(raw_features);

            
            // Print the feature sets
            std::cout << "\n\n===========================================\n";
            std::cout << "Snapshot #" << snapshot_count << " (Event #" << event_count << ")\n";
            std::cout << "===========================================\n";
            
            print_feature_set(raw_features, "Raw Features");
            // print_feature_set(normalized_features_long, "Normalized Features (20000 Window)");
            // print_feature_set(normalized_features_short, "Normalized Features (1500 Window)");
            
            // Basic validation
            EXPECT_NE(raw_features.timestamp_ns, 0) << "Timestamp should be set";
            EXPECT_GT(raw_features.market_depth, 0) << "Market depth should be positive";
        }
    }
    
    std::cout << "\nFeature processor test completed.\n";
    std::cout << "- Processed " << event_count << " events\n";
    std::cout << "- Generated " << snapshot_count << " snapshots\n";
    
    EXPECT_GT(event_count, 0) << "No events were processed";
    EXPECT_GT(snapshot_count, 0) << "No snapshots were generated";
}

// Main function for running the tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}