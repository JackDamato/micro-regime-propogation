#include "feature_engine.hpp"
#include "common_constants.hpp"
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>


FeatureEngine::FeatureEngine(OrderBookManager& order_book, std::string instrument)
    : order_book_(order_book) {
    instrument_ = instrument;
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

    return snapshot;
}

FeatureInputSnapshot FeatureEngine::generate_snapshot(uint64_t timestamp_ns) {
    // Get the current L3 snapshot from the order book
    L3Snapshot book_snapshot;
    order_book_.GetL3Snapshot(book_snapshot);
    
    // Generate the snapshot using the L3 data
    auto snapshot = generate_snapshot_from_l3(book_snapshot, timestamp_ns);
    
    //rolling_state_.trade_times.front() < timestamp_ns - microregime::EVENT_WINDOW_NS) {
    while (!rolling_state_.trade_times.empty() && rolling_state_.trade_times.size() > 75) {
        if (rolling_state_.rolling_trade_directions.front() == 1) {
            rolling_state_.buy_volume -= rolling_state_.trade_volumes.front();
        } else if (rolling_state_.rolling_trade_directions.front() == -1) {
            rolling_state_.sell_volume -= rolling_state_.trade_volumes.front();
        }
        rolling_state_.trade_times.pop_front();
        rolling_state_.trade_volumes.pop_front();
        rolling_state_.rolling_trade_directions.pop_front();
    }
    // rolling_state_.order_times.front() < timestamp_ns - microregime::EVENT_WINDOW_NS) {


    while (!rolling_state_.order_times.empty() && rolling_state_.order_times.size() > 75) {
        if (rolling_state_.rolling_order_directions.front() == 1) {
            rolling_state_.bid_volume -= rolling_state_.order_volumes.front();  
        } else if (rolling_state_.rolling_order_directions.front() == -1) {
            rolling_state_.ask_volume -= rolling_state_.order_volumes.front();
        }
        rolling_state_.order_times.pop_front();
        rolling_state_.order_volumes.pop_front();
        rolling_state_.rolling_order_directions.pop_front();
    }

    snapshot.trade_time_ns = (rolling_state_.trade_times.empty()) ? 0 : timestamp_ns - rolling_state_.trade_times.back();
    snapshot.order_time_ns = (rolling_state_.order_times.empty()) ? 0 : timestamp_ns - rolling_state_.order_times.back();

    // Initialize rolling statistics    
    snapshot.rolling_midprices = &rolling_state_.midprices;
    snapshot.rolling_spreads = &rolling_state_.spreads;
    snapshot.rolling_tick_directions = &rolling_state_.tick_directions;
    snapshot.rolling_order_book_imbalances = &rolling_state_.order_book_imbalances;
    snapshot.rolling_lob_bid_slopes = &rolling_state_.lob_bid_slopes;
    snapshot.rolling_lob_ask_slopes = &rolling_state_.lob_ask_slopes;

    // Rolling Trade State TODO
    snapshot.buy_volume = rolling_state_.buy_volume;
    snapshot.sell_volume = rolling_state_.sell_volume;
    snapshot.rolling_trade_sizes = &rolling_state_.trade_volumes;
    snapshot.rolling_trade_directions = &rolling_state_.rolling_trade_directions;

    // Rolling Order State TODO
    snapshot.bid_volume = rolling_state_.bid_volume;
    snapshot.ask_volume = rolling_state_.ask_volume;
    
    snapshot.midprice = order_book_.GetMidPrice();
    snapshot.instrument = instrument_;
    return snapshot;
}

// directions, sizes, times, volumes
void FeatureEngine::update_trade(double price, double size, int8_t direction, uint64_t timestamp_ns) {
    rolling_state_.rolling_trade_directions.push_back(direction);
    rolling_state_.trade_times.push_back(timestamp_ns);   
    // Update rolling statistics
    if (direction > 0) {
        rolling_state_.buy_volume += size;
    } else if (direction < 0) {
        rolling_state_.sell_volume += size;
    }

    rolling_state_.trade_volumes.push_back(size);
} 

void FeatureEngine::update_add(double size, int8_t direction, uint64_t timestamp_ns) {
    rolling_state_.rolling_order_directions.push_back(direction);
    rolling_state_.order_times.push_back(timestamp_ns);   
    // Update rolling statistics
    if (direction > 0) {
        rolling_state_.bid_volume += size;
    } else if (direction < 0) {
        rolling_state_.ask_volume += size;
    }

    rolling_state_.order_volumes.push_back(size);
}

void FeatureEngine::reset() {
    // Reset rolling state
    rolling_state_ = RollingState{};
}


// Called every ROLLING_UPDATE_INTERVAL_NS, so window length is ROLLING_UPDATE_INTERVAL_NS / 1'000'000'000 * ROLLING_WINDOW seconds
void FeatureEngine::UpdateRollingStats() {
    // Midprice and spread::
    double midprice = order_book_.GetMidPrice();
    double spread = order_book_.GetSpread();
    rolling_state_.midprices.push_back(midprice);
    rolling_state_.spreads.push_back(spread);
    if (rolling_state_.midprices.size() > 1) {
        auto it = rolling_state_.midprices.rbegin();
        double current = *it;
        double previous = *(++it);
        rolling_state_.tick_directions.push_back((current > previous) ? 1 : ((current < previous) ? -1 : 0));
    } else {
        rolling_state_.tick_directions.push_back(0);
    }

    if (rolling_state_.midprices.size() > ROLLING_WINDOW) {
        rolling_state_.midprices.pop_front();
        rolling_state_.spreads.pop_front();
        rolling_state_.tick_directions.pop_front();
    }


    // OBI and LOB Slopes
    double order_book_imbalance = order_book_.GetOrderBookImbalance();
    std::pair<double, double> lob_slopes = order_book_.GetLOBSlopes();
    rolling_state_.order_book_imbalances.push_back(order_book_imbalance);
    rolling_state_.lob_bid_slopes.push_back(lob_slopes.first);
    rolling_state_.lob_ask_slopes.push_back(lob_slopes.second);
    if (rolling_state_.order_book_imbalances.size() > 40) {
        rolling_state_.order_book_imbalances.pop_front();
        rolling_state_.lob_bid_slopes.pop_front();
        rolling_state_.lob_ask_slopes.pop_front();
    }
}
