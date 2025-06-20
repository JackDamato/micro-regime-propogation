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
    explicit FeatureEngine(OrderBookManager& order_book);
    
    // Disable copy and move constructors/assignment
    FeatureEngine(const FeatureEngine&) = delete;
    FeatureEngine& operator=(const FeatureEngine&) = delete;
    FeatureEngine(FeatureEngine&&) = delete;
    FeatureEngine& operator=(FeatureEngine&&) = delete;
    
    // Automatically updates rolling state, trade data, and depth changes before returning a full snapshot.
    FeatureInputSnapshot generate_snapshot(EventType event_type = EventType::UNKNOWN);
    
    // Static helper to generate a snapshot from an L3 snapshot
    FeatureInputSnapshot generate_snapshot_from_l3(
        const L3Snapshot& l3_snapshot, 
        uint64_t timestamp_ns, 
        EventType event_type = EventType::UNKNOWN);
    
    // Update the engine with a new trade
    void update_trade(double price, double size, int8_t direction);
    void update_rolling_state();
    void update_events(char event_type);
    // Reset the engine's internal state
    void reset();
    
private:
    // Reference to the order book
    OrderBookManager& order_book_;
    std::pair<double, double> get_rolling_means() const {
        double mid_sum = std::accumulate(rolling_state_.mid_prices.begin(), rolling_state_.mid_prices.end(), 0.0);
        double spread_sum = std::accumulate(rolling_state_.spreads.begin(), rolling_state_.spreads.end(), 0.0);
        size_t count = rolling_state_.mid_prices.size();
        return {mid_sum / count, spread_sum / count};
    }
    // Internal state for rolling calculations
    struct RollingState {
        std::deque<double> mid_prices;
        std::deque<double> spreads;
        std::deque<int8_t> tick_directions;
        std::deque<char> recent_event_types;
        std::deque<std::pair<int8_t, double>> trade_volumes;
        double buy_volume = 0.0;
        double sell_volume = 0.0;

        int add_count = 0;
        int cancel_count = 0;
    } rolling_state_;
    
    // Trade data
    struct {
        double price = 0.0;
        double size = 0.0;
        int8_t direction = 0;  // +1 for buy, -1 for sell, 0 for none/unknown
    } last_trade_;
    
    // Previous snapshot for delta calculations
    struct {
        std::array<double, DEPTH_LEVELS> bid_prices = {};
        std::array<double, DEPTH_LEVELS> ask_prices = {};
        std::array<int, DEPTH_LEVELS> bid_sizes = {};
        std::array<int, DEPTH_LEVELS> ask_sizes = {};
    } prev_snapshot_;
    
    // Helper methods
    void update_depth_changes(FeatureInputSnapshot& snapshot);
    void update_trade_info(FeatureInputSnapshot& snapshot);
    
    // Timestamp generation (could be overridden for testing)
    virtual uint64_t get_current_timestamp() const;
};
