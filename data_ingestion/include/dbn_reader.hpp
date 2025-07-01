#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "databento/dbn_decoder.hpp"
#include "market_event.hpp"

class DbnMboReader {
public:
    explicit DbnMboReader(const std::string& filepath, const std::string& instrument);
    ~DbnMboReader();

    // Whether there are more MBO records to read
    bool has_next() const;

    // Return the next parsed MBO record as a MarketEvent
    MarketEvent next_event();

    // Return the instrument name (e.g., "ES" or "SPY")
    const std::string& instrument_id() const { return instrument_; }

    // Total number of events parsed
    size_t event_count() const { return event_count_; }

private:
    // PIMPL pattern to hide implementation details
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Instrument name
    std::string instrument_;
    
    // Event counter
    size_t event_count_{0};
};