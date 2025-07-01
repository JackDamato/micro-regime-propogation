#pragma once

#include <map>
#include <list>
#include <unordered_map>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <limits>
#include "common_constants.hpp"

using microregime::DEPTH_LEVELS;

enum class BookSide { Bid, Ask };

struct PriceLevel {
    double price;
    int size;
};

struct L3Snapshot {
    std::array<PriceLevel, DEPTH_LEVELS> bid{};
    std::array<PriceLevel, DEPTH_LEVELS> ask{};
};

struct L3Delta {
    std::array<int8_t, DEPTH_LEVELS> bid_dir{}; // +1 = added, -1 = removed
    std::array<int8_t, DEPTH_LEVELS> ask_dir{};
};

struct Order {
    uint64_t order_id;
    int size;
};

struct OrderRef {
    double price;
    BookSide side;
    std::list<Order>::iterator it;
};

class OrderBookManager {
public:
    OrderBookManager();

    // Core event application
    void ApplyAdd(uint64_t order_id, double price, int size, BookSide side);
    void ApplyModify(uint64_t order_id, double new_price, int new_size);
    void ApplyCancel(uint64_t order_id, int canceled_size);
    void ApplyClear();

    // Query top-of-book and top N levels
    void GetL3Snapshot(L3Snapshot& snapshot) const;
    void GetDepthChange(L3Delta& delta) const;

    // Resets internal state
    void Reset();

    // Market microstructure metrics
    double GetMidPrice() const;
    double GetSpread() const;

private:
    // Internal helper methods
    using OrderQueue = std::list<Order>;
    using BidBook = std::map<double, OrderQueue, std::greater<>>; // Sorted by price descending
    using AskBook = std::map<double, OrderQueue>; // Sorted by price ascending

    BidBook bid_book_;
    AskBook ask_book_;
    std::unordered_map<uint64_t, OrderRef> order_lookup_;

    mutable L3Snapshot last_snapshot_;
    mutable L3Delta last_delta_;

    int sum_level_size(const OrderQueue& queue) const;
    void build_snapshot(BookSide side, const auto& book, std::array<PriceLevel, DEPTH_LEVELS>& levels) const;
    void compute_delta(const std::array<PriceLevel, DEPTH_LEVELS>& old_levels,
                       const std::array<PriceLevel, DEPTH_LEVELS>& new_levels,
                       std::array<int8_t, DEPTH_LEVELS>& delta) const;
};
