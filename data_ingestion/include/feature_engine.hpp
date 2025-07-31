#pragma once

#include "order_book.hpp"
#include "feature_snapshot.hpp"
#include <deque>
#include <array>
#include <cstdint>
#include <memory>
#include <numeric>
// Forward declaration
struct L3Snapshot;

class FeatureEngine {
public:
    // Constructor that takes a reference to an OrderBookManager
    FeatureEngine(OrderBookManager& order_book, std::string instrument);
    
    // Disable copy and move constructors/assignment (rule of 5 :))
    ~FeatureEngine() = default;
    FeatureEngine(const FeatureEngine&) = delete; // copy constructor Obj(const Obj&)
    FeatureEngine& operator=(const FeatureEngine&) = delete; // copy assignment Obj& operator=(const Obj&)
    FeatureEngine(FeatureEngine&&) = delete; // move constructor Obj(Obj&&)
    FeatureEngine& operator=(FeatureEngine&&) = delete; // move assignment Obj& operator=(Obj&&)
    
    // Automatically updates rolling state, trade data, and depth changes before returning a full snapshot.
    FeatureInputSnapshot generate_snapshot();
    
    // Static helper to generate a snapshot from an L3 snapshot
    FeatureInputSnapshot generate_snapshot_from_l3(
        const L3Snapshot& l3_snapshot, 
        uint64_t timestamp_ns);
    
    // Update the engine with a new trade
    void update_trade(double price, double size, int8_t direction);
    void update_events(char event_type);
    // Reset the engine's internal state
    void reset();
    
    void UpdateMidpriceAndSpread(double midprice, double spread);
    uint64_t most_recent_timestamp_ns = 0;
    
private:
    // Reference to the order book
    OrderBookManager& order_book_;
    std::string instrument_;
    // Internal state for rolling calculations
    struct RollingState {
        std::deque<double> midprices; // Take the last 5 minutes of midprices 0.05 seconds between each update.
        std::deque<double> spreads; // Take the last 5 minutes of spreads 0.05 seconds between each update.
        
        std::deque<int8_t> rolling_trade_directions;
        std::deque<int8_t> tick_directions;
        std::deque<char> recent_event_types;

        std::deque<std::pair<int8_t, double>> trade_volumes;
        double buy_volume = 0.0;
        double sell_volume = 0.0;

        int adds_since_last_snapshot = 0;
    } rolling_state_;
    
    // Helper methods
    void update_depth_changes(FeatureInputSnapshot& snapshot);
};
