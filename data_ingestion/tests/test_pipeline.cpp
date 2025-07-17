#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <dbn_reader.hpp>
#include <event_parser.hpp>
#include <order_engine.hpp>
#include <order_book.hpp>
#include <feature_engine.hpp>
#include <feature_snapshot.hpp>

namespace fs = std::filesystem;

class MarketDataPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Path to the test data file
        test_file_ = fs::path("..") / ".." / "data" / "ES" / "glbx-mdp3-20250506.mbo.dbn.zst";
        
        // Fallback path if running from build directory
        if (!fs::exists(test_file_)) {
            test_file_ = fs::path("..") / "data" / "ES" / "glbx-mdp3-20250506.mbo.dbn.zst";
            if (!fs::exists(test_file_)) {
                GTEST_SKIP() << "Test file not found at either location:"
                           << "\n  " << fs::absolute("../../data/ES/glbx-mdp3-20250506.mbo.dbn.zst")
                           << "\n  " << fs::absolute("../data/ES/glbx-mdp3-20250506.mbo.dbn.zst");
            }
        }
        
        instrument_ = "ES";
        std::cout << "Testing with file: " << fs::absolute(test_file_) << std::endl;
    }
    
    // Helper function to print a FeatureInputSnapshot
    void print_snapshot(const FeatureInputSnapshot& snapshot, size_t snapshot_num) {
        std::cout << "\n=== Snapshot #" << snapshot_num << " ===\n";
        std::cout << "Timestamp: " << snapshot.timestamp_ns << " ns\n";
        // std::cout << "Quote Update Count: " << snapshot.quote_update_count << "\n";
    
        // Top of Book
        std::cout << "\nTop of Book:\n";
        std::cout << "  Bid: " << snapshot.best_bid_price << "\n";
        std::cout << "  Ask: " << snapshot.best_ask_price << "\n";
    
        // Depth
        std::cout << "\nDepth Levels (Top " << DEPTH_LEVELS << "):\n";
        std::cout << "  Level |    Bid    | Size |    Ask    | Size\n";
        std::cout << "  ------+-----------+------+-----------+------\n";
        for (size_t i = 0; i < DEPTH_LEVELS; ++i) {
            std::cout << "  " << std::setw(5) << i + 1 << " | "
                      << std::setw(9) << snapshot.bid_prices[i] << " "
                      << std::setw(4) << snapshot.bid_sizes[i] << " | "
                      << std::setw(9) << snapshot.ask_prices[i] << " "
                      << std::setw(4) << snapshot.ask_sizes[i] << "\n";
        }
    
        // Rolling stats
        std::cout << "\nRolling Stats (last " << ROLLING_WINDOW << " events):\n";
        std::cout << "  Buy Volume: " << snapshot.rolling_buy_volume << "\n";
        std::cout << "  Sell Volume: " << snapshot.rolling_sell_volume << "\n";
        std::cout << "  Adds Since Last Snapshot: " << snapshot.adds_since_last_snapshot << "\n";


        // std::cout << "  Rolling Midprices: [";
        // std::copy(snapshot.rolling_midprices.begin(), snapshot.rolling_midprices.end() - 1, std::ostream_iterator<double>(std::cout, ", "));
        // std::cout << snapshot.rolling_midprices.back() << "]\n";
    
        // std::cout << "  Rolling Spreads: [";
        // std::copy(snapshot.rolling_spreads.begin(), snapshot.rolling_spreads.end() - 1, std::ostream_iterator<double>(std::cout, ", "));
        // std::cout << snapshot.rolling_spreads.back() << "]\n";
    
        // std::cout << "  Tick Directions: [";
        // std::copy(snapshot.rolling_tick_directions.begin(), snapshot.rolling_tick_directions.end() - 1, std::ostream_iterator<int>(std::cout, ", "));
        // std::cout << static_cast<int>(snapshot.rolling_tick_directions.back()) << "]\n";

        // std::cout << "  Trade Directions: [";
        // std::copy(snapshot.rolling_trade_directions.begin(), snapshot.rolling_trade_directions.end() - 1, std::ostream_iterator<int>(std::cout, ", "));
        // std::cout << static_cast<int>(snapshot.rolling_trade_directions.back()) << "]\n";

        std::cout << "  Bid Depth Change Directions: [";
        std::copy(snapshot.bid_depth_change_direction.begin(), snapshot.bid_depth_change_direction.end() - 1, std::ostream_iterator<int>(std::cout, ", "));
        std::cout << static_cast<int>(snapshot.bid_depth_change_direction.back()) << "]\n";

        std::cout << "  Ask Depth Change Directions: [";
        std::copy(snapshot.ask_depth_change_direction.begin(), snapshot.ask_depth_change_direction.end() - 1, std::ostream_iterator<int>(std::cout, ", "));
        std::cout << static_cast<int>(snapshot.ask_depth_change_direction.back()) << "]\n";
    
        // Reserved field
        std::cout << "Reserved: " << snapshot.reserved << "\n";
    }    
    
    fs::path test_file_;
    std::string instrument_;
};

TEST_F(MarketDataPipelineTest, FullPipelineTest) {
    // 1. Create the components
    OrderEngine order_engine;
    EventParser parser(test_file_.string(), instrument_);
    // 2. Process events and collect snapshots
    const size_t max_events = 10000000000;  // Process first N events
    const size_t snapshot_interval = 100000;  // Take snapshot every N events
    size_t event_count = 0;
    size_t snapshot_count = 0;
    
    FeatureEngine feature_engine(const_cast<OrderBookManager&>(order_engine.get_or_create_order_book(instrument_)), instrument_);
    std::cout << "Starting pipeline test...\n";
    while (parser.has_more_events() && event_count < max_events) {            
        // Process next event
        bool processed = parser.process_next(order_engine, &feature_engine);

        if (!processed) break;
        
        event_count++;
        
        // Take periodic snapshots
        if (event_count % snapshot_interval == 0) {
            const auto& order_book = order_engine.get_order_book(instrument_);
            FeatureInputSnapshot snapshot;
            
            // Generate snapshot using FeatureEngine
            snapshot = feature_engine.generate_snapshot();
            snapshot_count++;
            print_snapshot(snapshot, snapshot_count);
        }
    }
    
    std::cout << "\nPipeline test completed.\n";
    std::cout << "- Processed " << event_count << " events\n";
    std::cout << "- Generated " << snapshot_count << " snapshots\n";

    
    EXPECT_GT(event_count, 0) << "No events were processed";
    EXPECT_GT(snapshot_count, 0) << "No snapshots were generated";
}

// main() is defined in test_dbn_reader.cpp
