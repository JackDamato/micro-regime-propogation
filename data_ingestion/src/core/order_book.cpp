#include "order_book.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <numeric>
#include <iostream>

OrderBookManager::OrderBookManager() {
    Reset();
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
    last_delta_ = L3Delta{};
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

void OrderBookManager::GetDepthChange(L3Delta& delta) const {
    L3Snapshot new_snapshot;
    GetL3Snapshot(new_snapshot);
    
    compute_delta(last_snapshot_.bid, new_snapshot.bid, delta.bid_dir);
    compute_delta(last_snapshot_.ask, new_snapshot.ask, delta.ask_dir);
    
    last_snapshot_ = new_snapshot;
    last_delta_ = delta;
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
    last_delta_ = L3Delta{};
}


int OrderBookManager::sum_level_size(const OrderQueue& queue) const {
    int total_size = 0;
    for (const auto& order : queue) {
        total_size += order.size;
    }
    return total_size;
}

void OrderBookManager::build_snapshot(BookSide side, const auto& book, 
                                     std::array<PriceLevel, DEPTH_LEVELS>& levels) const {
    size_t i = 0;
    if (side == BookSide::Bid) {
        // For bids, we want highest prices first (descending)
        for (auto it = bid_book_.begin(); it != bid_book_.end() && i < DEPTH_LEVELS; ++it) {
            levels[i].price = it->first;
            levels[i].size = sum_level_size(it->second);
            ++i;
        }   
    } else {
        // For asks, we want lowest prices first (ascending)
        for (const auto& [price, queue] : ask_book_) {
            if (i >= DEPTH_LEVELS) break;
            levels[i].price = price;
            levels[i].size = sum_level_size(queue);
            i++;
        }
    }
    
    // Fill remaining levels with zeros
    for (; i < DEPTH_LEVELS; ++i) {
        levels[i].price = 0.0;
        levels[i].size = 0;
    }
}

void OrderBookManager::compute_delta(
    const std::array<PriceLevel, DEPTH_LEVELS>& old_levels,
    const std::array<PriceLevel, DEPTH_LEVELS>& new_levels,
    std::array<int8_t, DEPTH_LEVELS>& delta
) const {
    for (size_t i = 0; i < DEPTH_LEVELS; ++i) {
        double old_price = old_levels[i].price;
        double new_price = new_levels[i].price;
        int old_size = old_levels[i].size;
        int new_size = new_levels[i].size;

        if (std::abs(new_price - old_price) < 1e-10) {
            // Same price level: compare sizes
            if (new_size > old_size) {
                delta[i] = 1;   // Added liquidity
            } else if (new_size < old_size) {
                delta[i] = -1;  // Removed liquidity
            } else {
                delta[i] = 0;    // No change
            }
        } else {
            // Price level changed
            if (new_size == old_size) {
                delta[i] = 0;   // Assume shift without net liquidity change
            } else if (new_size > old_size) {
                delta[i] = 1;   // Net add at new level
            } else {
                delta[i] = -1;  // Net remove
            }
        }
    }
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
