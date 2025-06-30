#include "order_engine.hpp"
#include "../include/market_event.hpp"
#include <stdexcept>
#include <iostream>
#include <algorithm>

OrderBookManager& OrderEngine::get_or_create_order_book(const std::string& instrument) {
    auto [it, inserted] = order_books_.try_emplace(instrument);
    return it->second;
}

void OrderEngine::track_order(uint64_t order_id, const std::string& instrument, BookSide side, double price) {
    order_info_[order_id] = {side, price, instrument};
}

void OrderEngine::untrack_order(uint64_t order_id) {
    order_info_.erase(order_id);
}

void OrderEngine::reset() {
    order_books_.clear();
    order_info_.clear();
    current_timestamp_ = 0;
}

const OrderBookManager& OrderEngine::get_order_book(const std::string& instrument) const {
    auto it = order_books_.find(instrument);
    if (it == order_books_.end()) {
        throw std::runtime_error("No order book found for instrument: " + instrument);
    }
    return it->second;
}

void OrderEngine::process_event(const MarketEvent& event, FeatureEngine* feature_engine) {
    // Update current timestamp
    if (event.timestamp_ns < current_timestamp_) {
        throw std::runtime_error("Out-of-order event detected");
    }
    
    if (event.instrument == "ES" && event.instrument_id != 4916) {
        return;
    }

    current_timestamp_ = event.timestamp_ns;
    
    auto& order_book = get_or_create_order_book(event.instrument);
    feature_engine->most_recent_timestamp_ns = event.timestamp_ns;
    try {
        switch (event.action) {
            case 'A': {  // Add
                BookSide side = (event.side == 'B') ? BookSide::Bid : BookSide::Ask;
                order_book.ApplyAdd(event.order_id, event.price, event.size, side);
                track_order(event.order_id, event.instrument, side, event.price);
                feature_engine->update_events('A');
                feature_engine->update_rolling_state();
                break;
            }
            case 'M': {  // Modify
                // Look up the order to get its original side and price
                auto order_info = get_order_info(event.order_id);
                if (!order_info) {
                    std::cerr << "Warning: Modify for unknown order ID: " << event.order_id << std::endl;
                    break;
                }
                
                // Cancel the old order (0 means cancel all remaining size)
                order_book.ApplyCancel(event.order_id, 0);
                
                // Add the modified order with new price/size
                order_book.ApplyAdd(event.order_id, event.price, event.size, order_info->side);
                
                // Update the order info with new price
                track_order(event.order_id, event.instrument, order_info->side, event.price);
                feature_engine->update_rolling_state();
                feature_engine->update_events('M');
                break;
            }
            case 'C': {  // Cancel
                // Look up the order to get its side and price
                auto order_info = get_order_info(event.order_id);
                if (!order_info) {
                    // std::cerr << "Warning: Cancel for unknown order ID: " << event.order_id << std::endl;
                    break;
                }
                
                // Apply the cancel (0 means cancel all remaining size)
                order_book.ApplyCancel(event.order_id, 0);
                
                // Remove the order from tracking
                untrack_order(event.order_id);
                feature_engine->update_events('C');
                feature_engine->update_rolling_state();
                break;
            }
            case 'R': { // Clear
                std::cout << "Flags: " << event.flags << std::endl;
                // order_book.Reset();
                // feature_engine->reset();
                // reset();
                std::cout << "Clear event received at " << event.timestamp_ns << std::endl;
                break;
            }
            case 'T': {  // Trade
                // Trades typically don't modify the order book directly
                // They're recorded but don't change the visible order book
                if (feature_engine) {
                    double price = event.price;
                    int size = event.size;
                    int8_t direction = event.side == 'B' ? 1 : -1;
                    feature_engine->update_trade(price, size, direction);
                }
                break;
            }
            case 'F': {  // Fill
                // Trades typically don't modify the order book directly
                break;
            }
            default:
                std::cerr << "Warning: Unknown event action: " << event.action << std::endl;
                break;
        }
    } catch (const std::exception& e) {
        // std::cerr << "Error processing event: " << e.what() << std::endl;
    } catch (...) {
        // std::cerr << "Unknown error processing event" << std::endl;
    }
}
