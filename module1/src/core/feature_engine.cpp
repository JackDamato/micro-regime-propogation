#include "feature_engine.hpp"
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>

FeatureEngine::FeatureEngine(OrderBookManager& order_book)
    : order_book_(order_book) {
    reset();
}

FeatureInputSnapshot FeatureEngine::generate_snapshot_from_l3(
    const L3Snapshot& l3_snapshot,
    uint64_t timestamp_ns,
    EventType event_type) {
    
    FeatureInputSnapshot snapshot{};
    snapshot.timestamp_ns = timestamp_ns;
    snapshot.event_type = event_type;
    
    // Set top of book
    if (!l3_snapshot.bid.empty()) {
        snapshot.best_bid_price = l3_snapshot.bid[0].price;
        snapshot.best_bid_size = l3_snapshot.bid[0].size;
    }
    
    if (!l3_snapshot.ask.empty()) {
        snapshot.best_ask_price = l3_snapshot.ask[0].price;
        snapshot.best_ask_size = l3_snapshot.ask[0].size;
    }
    
    // Set depth levels
    for (size_t i = 0; i < DEPTH_LEVELS; ++i) {
        if (i < l3_snapshot.bid.size()) {
            snapshot.bid_prices[i] = l3_snapshot.bid[i].price;
            snapshot.bid_sizes[i] = l3_snapshot.bid[i].size;
        }
        if (i < l3_snapshot.ask.size()) {
            snapshot.ask_prices[i] = l3_snapshot.ask[i].price;
            snapshot.ask_sizes[i] = l3_snapshot.ask[i].size;
        }
    }
    
    // Initialize rolling statistics
    double mid_price = (snapshot.best_bid_price + snapshot.best_ask_price) / 2.0;
    double spread = snapshot.best_ask_price - snapshot.best_bid_price;
    
    // Update rolling window (circular buffer)
    for (size_t i = 0; i < ROLLING_WINDOW; ++i) {
        snapshot.rolling_midprices[i] = (i < rolling_state_.mid_prices.size()) 
            ? rolling_state_.mid_prices[i] : 0.0;
        snapshot.rolling_spreads[i] = (i < rolling_state_.spreads.size()) 
            ? rolling_state_.spreads[i] : 0.0;
        snapshot.rolling_tick_directions[i] = (i < rolling_state_.tick_directions.size())
            ? rolling_state_.tick_directions[i] : 0;
    }
    
    // Update rolling statistics (calculate means)
    double sum_mid = 0.0;
    double sum_spread = 0.0;
    int valid_values = 0;
    
    for (size_t i = 0; i < ROLLING_WINDOW; ++i) {
        if (snapshot.rolling_midprices[i] > 0) {  // Assuming 0 means uninitialized
            sum_mid += snapshot.rolling_midprices[i];
            sum_spread += snapshot.rolling_spreads[i];
            valid_values++;
        }
    }
    
    if (valid_values > 0) {
        // Store the means in the first element for simplicity
        snapshot.rolling_midprices[0] = sum_mid / valid_values;
        snapshot.rolling_spreads[0] = sum_spread / valid_values;
    }
    
    // Set basic metrics
    snapshot.rolling_buy_volume = !l3_snapshot.bid.empty() ? l3_snapshot.bid[0].size : 0;
    snapshot.rolling_sell_volume = !l3_snapshot.ask.empty() ? l3_snapshot.ask[0].size : 0;
    
    // Update quote update count (simplified)
    static uint32_t quote_update_count = 0;
    snapshot.quote_update_count = ++quote_update_count;
    
    return snapshot;
}

FeatureInputSnapshot FeatureEngine::generate_snapshot(EventType event_type) {
    // Get the current L3 snapshot from the order book
    L3Snapshot book_snapshot;
    order_book_.GetL3Snapshot(book_snapshot);
    
    // Generate the snapshot using the L3 data
    auto snapshot = generate_snapshot_from_l3(book_snapshot, get_current_timestamp(), event_type);
    
    // Update rolling state and copy to snapshot
    snapshot.rolling_buy_volume = rolling_state_.buy_volume;
    snapshot.rolling_sell_volume = rolling_state_.sell_volume;

    int add_count = 0, cancel_count = 0;
    for (const auto& evt : rolling_state_.recent_event_types) {
        if (evt == 'A') ++add_count;
        else if (evt == 'C') ++cancel_count;
    }
    snapshot.rolling_add_count = add_count;
    snapshot.rolling_cancel_count = cancel_count;
    
    // Update depth changes and trade info
    // update_depth_changes(snapshot);
    update_trade_info(snapshot);
    
    return snapshot;
}

void FeatureEngine::update_trade(double price, double size, int8_t direction) {
    last_trade_.price = price;
    last_trade_.size = size;
    last_trade_.direction = direction;
    
    // Update rolling statistics
    if (direction > 0) {
        rolling_state_.buy_volume += size;
    } else if (direction < 0) {
        rolling_state_.sell_volume += size;
    }

    rolling_state_.trade_volumes.push_back(std::make_pair(direction, size));
    if (rolling_state_.trade_volumes.size() > ROLLING_WINDOW) {
        if (rolling_state_.trade_volumes.front().first > 0) {
            rolling_state_.buy_volume -= rolling_state_.trade_volumes.front().second;
        } else {
            rolling_state_.sell_volume -= rolling_state_.trade_volumes.front().second;
        }
        rolling_state_.trade_volumes.pop_front();
    }
}

void FeatureEngine::reset() {
    // Reset rolling state
    rolling_state_ = RollingState{};
    
    // Reset trade info
    last_trade_ = {};
    
    // Reset previous snapshot
    prev_snapshot_ = {};
}


void FeatureEngine::update_rolling_state() {
    double mid_price = order_book_.GetMidPrice();
    double spread = order_book_.GetSpread();

    rolling_state_.mid_prices.push_back(mid_price);
    rolling_state_.spreads.push_back(spread);

    if (rolling_state_.mid_prices.size() > 1) {
        auto it = rolling_state_.mid_prices.rbegin();
        double current = *it;
        double previous = *(++it);
        rolling_state_.tick_directions.push_back((current > previous) ? 1 : ((current < previous) ? -1 : 0));
    } else {
        rolling_state_.tick_directions.push_back(0);
    }

    if (rolling_state_.mid_prices.size() > ROLLING_WINDOW) {
        rolling_state_.mid_prices.pop_front();
        rolling_state_.spreads.pop_front();
        rolling_state_.tick_directions.pop_front();
    }
}


void FeatureEngine::update_events(char event_type) {
    rolling_state_.recent_event_types.push_back(event_type);
    if (rolling_state_.recent_event_types.size() > ROLLING_WINDOW) {
        rolling_state_.recent_event_types.pop_front();
    }
}

void FeatureEngine::update_trade_info(FeatureInputSnapshot& snapshot) {
    snapshot.last_trade_price = last_trade_.price;
    snapshot.last_trade_size = last_trade_.size;
    snapshot.last_trade_direction = last_trade_.direction;
    
    // Reset trade info after it's been recorded
    last_trade_ = {};
}

uint64_t FeatureEngine::get_current_timestamp() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<nanoseconds>(duration).count();
}


// void FeatureEngine::update_depth_changes(FeatureInputSnapshot& snapshot) {
//     for (size_t i = 0; i < DEPTH_LEVELS; ++i) {
//         // Check for price level changes
//         bool bid_changed = (snapshot.bid_prices[i] != prev_snapshot_.bid_prices[i]);
//         bool ask_changed = (snapshot.ask_prices[i] != prev_snapshot_.ask_prices[i]);
        
//         // Update bid depth change direction
//         if (bid_changed) {
//             snapshot.bid_depth_change_direction[i] = (snapshot.bid_sizes[i] > 0) ? 1 : -1;
//         } else if (snapshot.bid_sizes[i] > prev_snapshot_.bid_sizes[i]) {
//             snapshot.bid_depth_change_direction[i] = 1;  // Added liquidity
//         } else if (snapshot.bid_sizes[i] < prev_snapshot_.bid_sizes[i]) {
//             snapshot.bid_depth_change_direction[i] = -1;  // Removed liquidity
//         } else {
//             snapshot.bid_depth_change_direction[i] = 0;   // No change
//         }
        
//         // Update ask depth change direction
//         if (ask_changed) {
//             snapshot.ask_depth_change_direction[i] = (snapshot.ask_sizes[i] > 0) ? 1 : -1;
//         } else if (snapshot.ask_sizes[i] > prev_snapshot_.ask_sizes[i]) {
//             snapshot.ask_depth_change_direction[i] = 1;  // Added liquidity
//         } else if (snapshot.ask_sizes[i] < prev_snapshot_.ask_sizes[i]) {
//             snapshot.ask_depth_change_direction[i] = -1;  // Removed liquidity
//         } else {
//             snapshot.ask_depth_change_direction[i] = 0;   // No change
//         }
//     }
// }