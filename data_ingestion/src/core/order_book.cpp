#include "order_book.hpp"
#include "common_constants.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <numeric>
#include <iostream>

OrderBookManager::OrderBookManager() {
    Reset();
    
    // For OBI Weighting
    constexpr int DEPTH_LEVELS = 10;
    constexpr double LAMBDA = 0.8;

    for (int i = 0; i < DEPTH_LEVELS; ++i) {
        obi_weights[i] = exp(-LAMBDA * i);  // i=0 → weight=1.0 (best bid/ask)
    }
}

void OrderBookManager::ApplyAdd(uint64_t order_id, double price, int size, BookSide side) {
    if (order_lookup_.find(order_id) != order_lookup_.end()) {
        throw std::runtime_error("Order ID already exists");
    }
    
    if (side == BookSide::Bid) {
        auto& queue = bid_book_[price];
        queue.push_back({order_id, size});
        order_lookup_[order_id] = {price, side, --queue.end()};
    } else {
        auto& queue = ask_book_[price];
        queue.push_back({order_id, size});
        order_lookup_[order_id] = {price, side, --queue.end()};
    }
}

void OrderBookManager::ApplyModify(uint64_t order_id, double new_price, int new_size) {
    auto it = order_lookup_.find(order_id);
    if (it == order_lookup_.end()) {
        throw std::runtime_error("Order ID not found for modify");
    }
    
    auto& order_ref = it->second;
    
    // Remove from old price level
    if (order_ref.side == BookSide::Bid) {
        auto& old_queue = bid_book_[order_ref.price];
        old_queue.erase(order_ref.it);
        if (old_queue.empty()) {
            bid_book_.erase(order_ref.price);
        }
        
        // Add to new price level
        auto& new_queue = bid_book_[new_price];
        new_queue.push_back({order_id, new_size});
        order_ref.it = --new_queue.end();
    } else {
        auto& old_queue = ask_book_[order_ref.price];
        old_queue.erase(order_ref.it);
        if (old_queue.empty()) {
            ask_book_.erase(order_ref.price);
        }
        
        // Add to new price level
        auto& new_queue = ask_book_[new_price];
        new_queue.push_back({order_id, new_size});
        order_ref.it = --new_queue.end();
    }
    
    // Update order reference
    order_ref.price = new_price;
}

void OrderBookManager::ApplyCancel(uint64_t order_id, int canceled_size) {
    auto it = order_lookup_.find(order_id);
    if (it == order_lookup_.end()) {
        throw std::runtime_error("Order ID not found for cancel");
    }
    
    auto& order_ref = it->second;
    int canceled = 0;
    
    if (order_ref.side == BookSide::Bid) {
        auto price_it = bid_book_.find(order_ref.price);
        if (price_it != bid_book_.end()) {
            // Get the size before erasing
            canceled = order_ref.it->size;
            
            // Erase the order
            price_it->second.erase(order_ref.it);
            
            // If the price level is now empty, remove it
            if (price_it->second.empty()) {
                bid_book_.erase(price_it);
            }
        }
    } else {
        auto price_it = ask_book_.find(order_ref.price);
        if (price_it != ask_book_.end()) {
            // Get the size before erasing
            canceled = order_ref.it->size;
            
            // Erase the order
            price_it->second.erase(order_ref.it);
            
            // If the price level is now empty, remove it
            if (price_it->second.empty()) {
                ask_book_.erase(price_it);
            }
        }
    }
    
    // Remove from order lookup
    order_lookup_.erase(it);
    
    // No return value needed as per header declaration
}
 
void OrderBookManager::ApplyClear() {
    bid_book_.clear();
    ask_book_.clear();
    order_lookup_.clear();
    last_snapshot_ = L3Snapshot{};
}

void OrderBookManager::GetL3Snapshot(L3Snapshot& snapshot) const {
    std::fill(snapshot.bid.begin(), snapshot.bid.end(), PriceLevel{0.0, 0});
    std::fill(snapshot.ask.begin(), snapshot.ask.end(), PriceLevel{0.0, 0});
    
    // Build bid side (descending price)
    size_t bid_level = 0;
    for (auto it = bid_book_.begin(); it != bid_book_.end() && bid_level < DEPTH_LEVELS; ++it) {
        snapshot.bid[bid_level].price = it->first;
        int total_size = 0;
        for (const auto& order : it->second) {
            total_size += order.size;
        }
        snapshot.bid[bid_level].size = total_size;
        bid_level++;
    }
    
    // Build ask side (ascending price)
    size_t ask_level = 0;
    for (const auto& [price, queue] : ask_book_) {
        if (ask_level >= DEPTH_LEVELS) break;
        snapshot.ask[ask_level].price = price;
        int total_size = 0;
        for (const auto& order : queue) {
            total_size += order.size;
        }
        snapshot.ask[ask_level].size = total_size;
        ask_level++;
    }
}


void OrderBookManager::Reset() {
    std::cout << "Resetting order book " << std::endl;
    bid_book_.clear();
    ask_book_.clear();
    order_lookup_.clear();
    // print out bid and ask books to verify they are empty
    std::cout << "Bid book size: " << bid_book_.size() << std::endl;
    std::cout << "Ask book size: " << ask_book_.size() << std::endl;
    last_snapshot_ = L3Snapshot{};
}


int OrderBookManager::sum_level_size(const OrderQueue& queue) const {
    int total_size = 0;
    for (const auto& order : queue) {
        total_size += order.size;
    }
    return total_size;
}


double OrderBookManager::GetMidPrice() const {
    if (bid_book_.empty() || ask_book_.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (bid_book_.begin()->first + ask_book_.begin()->first) / 2.0;
}

double OrderBookManager::GetSpread() const {
    if (bid_book_.empty() || ask_book_.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return ask_book_.begin()->first - bid_book_.begin()->first;
}

double OrderBookManager::GetOrderBookImbalance() const {
    if (bid_book_.empty() || ask_book_.empty()) {
        return 0.0;
    }
    
    double bid_sum = 0.0, ask_sum = 0.0;
    
    // Sum bid volumes for top DEPTH_LEVELS price levels (highest to lowest)
    int bid_level = 0;
    for (auto it = bid_book_.begin(); it != bid_book_.end() && bid_level < DEPTH_LEVELS; ++it, ++bid_level) {
        bid_sum += sum_level_size(it->second) * obi_weights[bid_level];
    }
    
    // Sum ask volumes for top DEPTH_LEVELS price levels (lowest to highest)
    int ask_level = 0;
    for (auto it = ask_book_.begin(); it != ask_book_.end() && ask_level < DEPTH_LEVELS; ++it, ++ask_level) {
        ask_sum += sum_level_size(it->second) * obi_weights[ask_level];
    }
    
    const double total = bid_sum + ask_sum;
    return (total > 0) ? (bid_sum - ask_sum) / total : 0.0;
}


std::pair<double, double> OrderBookManager::GetLOBSlopes() const {
    if (bid_book_.empty() || ask_book_.empty()) {
        return {0.0, 0.0};
    }
    if (bid_book_.size() < DEPTH_LEVELS || ask_book_.size() < DEPTH_LEVELS) {
        return {0.0, 0.0};
    }
    // Get current snapshot of the order book
    L3Snapshot snapshot;
    GetL3Snapshot(snapshot);
    
    // Get best bid and ask prices
    const double best_bid = !bid_book_.empty() ? bid_book_.begin()->first : 0.0;
    const double best_ask = !ask_book_.empty() ? ask_book_.begin()->first : 0.0;
    
    if (best_bid <= 0.0 || best_ask <= 0.0) {
        return {0.0, 0.0};
    }
    
    std::array<double, DEPTH_LEVELS> bid_x, ask_x;  // Price distances
    std::array<double, DEPTH_LEVELS> bid_y, ask_y;  // Normalized sizes
    
    // Calculate total volume for normalization
    double bid_total = 0.0, ask_total = 0.0;
    for (int i = 0; i < DEPTH_LEVELS; ++i) {
        bid_total += snapshot.bid[i].size;
        ask_total += snapshot.ask[i].size;
    }
    
    if (bid_total <= 0.0 || ask_total <= 0.0) {
        return {0.0, 0.0};
    }
    
    // Calculate cumulative depth and price distances
    double bid_cumul = 0.0, ask_cumul = 0.0;
    for (int i = 0; i < DEPTH_LEVELS; ++i) {
        // Calculate price distance as (price - best_price) / best_price
        bid_x[i] = (snapshot.bid[i].price - best_bid) / best_bid;
        ask_x[i] = (snapshot.ask[i].price - best_ask) / best_ask;
            
        // Calculate fractional cumulative depth
        bid_cumul += snapshot.bid[i].size;
        ask_cumul += snapshot.ask[i].size;
        bid_y[i] = bid_cumul / bid_total;
        ask_y[i] = ask_cumul / ask_total;
    }
    
    // Linear regression to get slopes
    auto linreg = [](const std::array<double, DEPTH_LEVELS>& x, 
        const std::array<double, DEPTH_LEVELS>& y) {
        // 1. Compute sums
        double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    
        // Compiler will likely unroll this loop automatically for small DEPTH_LEVELS
        for (int i = 0; i < DEPTH_LEVELS; ++i) {
            sum_x += x[i];
            sum_y += y[i];
            sum_xy += x[i] * y[i];
            sum_xx += x[i] * x[i];
        }
    
        // 2. Calculate slope (β) and intercept (α)
        double denominator = DEPTH_LEVELS * sum_xx - sum_x * sum_x;
        if (std::fabs(denominator) < 1e-10) return std::make_pair(0.0, 0.0);  // Avoid division by zero
    
        double slope = (denominator > 0) ? (DEPTH_LEVELS * sum_xy - sum_x * sum_y) / denominator : 0.0;
        double intercept = (sum_y - slope * sum_x) / DEPTH_LEVELS;
    
        return std::make_pair(slope, intercept);
    };
    
    auto bid_slope = linreg(bid_x, bid_y).first;
    auto ask_slope = linreg(ask_x, ask_y).first;
    
    return std::make_pair(bid_slope, ask_slope);
}