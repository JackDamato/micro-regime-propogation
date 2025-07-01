#pragma once

#include <cstdint>
#include <string>

// Common market event structure used across the application
struct MarketEvent {
    uint64_t timestamp_ns;   // Nanoseconds since epoch
    std::string instrument;  // Instrument identifier (e.g., "ES")
    char action;            // 'A'dd, 'M'odify, 'C'ancel, 'T'rade, 'F'ill, 'R'eplace
    char side;              // 'B'id, 'A'sk, or 'N'one
    double price;           // Price level
    int size;               // Order size
    uint64_t order_id;      // Unique order identifier
    uint8_t flags;          // Bitfield: end-of-event, implied, self-trade, etc.
    uint32_t instrument_id; // Numeric instrument identifier
    uint8_t channel_id;     // Channel identifier for sharded feeds
    uint32_t sequence;      // Sequence number for gap detection
};
