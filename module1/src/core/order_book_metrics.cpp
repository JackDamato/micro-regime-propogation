#include "order_book.hpp"
#include <algorithm>
#include <limits>

// Market microstructure metrics implementations

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
