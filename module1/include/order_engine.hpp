#pragma once

#include "order_book.hpp"
#include "market_event.hpp"
#include "feature_engine.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <optional>

// Structure to track order metadata
struct OrderInfo {
    BookSide side;
    double price;
    std::string instrument;
};

class OrderEngine {
public:
    explicit OrderEngine() = default;
    ~OrderEngine() = default;

    // Disable copy and move
    OrderEngine(const OrderEngine&) = delete;
    OrderEngine& operator=(const OrderEngine&) = delete;
    OrderEngine(OrderEngine&&) = delete;
    OrderEngine& operator=(OrderEngine&&) = delete;

    // Process a single market event
    void process_event(const MarketEvent& event, FeatureEngine* feature_engine = nullptr);
    
    // Get the order book for a specific instrument
    const OrderBookManager& get_order_book(const std::string& instrument) const;
    
    // Get all order books (for iteration)
    const auto& get_order_books() const { return order_books_; }
    
    // Get the current timestamp (latest event processed)
    uint64_t current_timestamp() const { return current_timestamp_; }
    
    // Reset the engine (clear all order books and order info)
    void reset();

    // Lookup order information by ID
    std::optional<OrderInfo> get_order_info(uint64_t order_id) const {
        auto it = order_info_.find(order_id);
        if (it != order_info_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    // Get or create an order book for an instrument
    OrderBookManager& get_or_create_order_book(const std::string& instrument);

private:
    // Order books by instrument
    std::unordered_map<std::string, OrderBookManager> order_books_;
    
    // Order metadata by order ID
    std::unordered_map<uint64_t, OrderInfo> order_info_;
    
    // Current timestamp (from last processed event)
    uint64_t current_timestamp_ = 0;
    
    // Update order info when an order is added
    void track_order(uint64_t order_id, const std::string& instrument, BookSide side, double price);
    
    // Remove order info when an order is fully canceled
    void untrack_order(uint64_t order_id);
};
