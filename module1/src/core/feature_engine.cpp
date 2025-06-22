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
    uint64_t timestamp_ns) {
    
    FeatureInputSnapshot snapshot{};
    snapshot.timestamp_ns = timestamp_ns;
    
    // Set top of book
    if (!l3_snapshot.bid.empty()) {
        snapshot.best_bid_price = l3_snapshot.bid[0].price;
    }
    
    if (!l3_snapshot.ask.empty()) {
        snapshot.best_ask_price = l3_snapshot.ask[0].price;
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
    
    snapshot.rolling_midprices = &rolling_state_.mid_prices;
    snapshot.rolling_spreads = &rolling_state_.spreads;
    snapshot.rolling_tick_directions = &rolling_state_.tick_directions;
    snapshot.rolling_trade_directions = &rolling_state_.rolling_trade_directions;
    
    // Set basic metrics
    snapshot.rolling_buy_volume = !l3_snapshot.bid.empty() ? l3_snapshot.bid[0].size : 0;
    snapshot.rolling_sell_volume = !l3_snapshot.ask.empty() ? l3_snapshot.ask[0].size : 0;
    
    // // Update quote update count (simplified)
    // static uint32_t quote_update_count = 0;
    // snapshot.quote_update_count = ++quote_update_count;
    
    return snapshot;
}

FeatureInputSnapshot FeatureEngine::generate_snapshot() {
    // Get the current L3 snapshot from the order book
    L3Snapshot book_snapshot;
    order_book_.GetL3Snapshot(book_snapshot);
    
    // Generate the snapshot using the L3 data
    auto snapshot = generate_snapshot_from_l3(book_snapshot, most_recent_timestamp_ns);
    
    // Update rolling state and copy to snapshot
    snapshot.rolling_buy_volume = rolling_state_.buy_volume;
    snapshot.rolling_sell_volume = rolling_state_.sell_volume;

    snapshot.adds_since_last_snapshot = rolling_state_.adds_since_last_snapshot;
    rolling_state_.adds_since_last_snapshot = 0;
    
    // Update depth changes and trade info
    L3Delta delta;
    order_book_.GetDepthChange(delta);
    snapshot.bid_depth_change_direction = delta.bid_dir;
    snapshot.ask_depth_change_direction = delta.ask_dir;
    update_trade_info(snapshot);
    
    return snapshot;
}

void FeatureEngine::update_trade(double price, double size, int8_t direction) {
    last_trade_.price = price;
    last_trade_.size = size;
    last_trade_.direction = direction;
    
    if (direction != 0) {
        rolling_state_.rolling_trade_directions.push_back(direction);
        if (rolling_state_.rolling_trade_directions.size() > ROLLING_WINDOW) {
            rolling_state_.rolling_trade_directions.pop_front();
        }
    }

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
    if (event_type == 'A') {
        rolling_state_.adds_since_last_snapshot++;
    }
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