#pragma once

#include "dbn_reader.hpp"
#include "order_engine.hpp"
#include "feature_engine.hpp"
#include <memory>
#include <string>
#include <cstdint>

class EventParser {
public:
    // Constructor with DBN file path and instrument
    EventParser(const std::string& dbn_filepath, const std::string& instrument);
    
    ~EventParser() = default;
    
    // Disable copy and move
    EventParser(const EventParser&) = delete;
    EventParser& operator=(const EventParser&) = delete;
    EventParser(EventParser&&) = delete;
    EventParser& operator=(EventParser&&) = delete;

    // Process all events in the file
    void process_all(OrderEngine& engine);
    
    // Process events until the specified timestamp (nanoseconds since epoch)
    // Returns true if more events are available after the stop time
    bool process_until(OrderEngine& engine, uint64_t stop_timestamp_ns);
    
    // Process the next event
    // Returns true if an event was processed, false if no more events
    bool process_next(OrderEngine& engine, FeatureEngine* feature_engine = nullptr);
    
    // Get the current timestamp (from last processed event)
    uint64_t current_timestamp() const { return current_timestamp_; }
    
    // Check if there are more events to process
    bool has_more_events() const;

private:
    // The DBN reader
    std::unique_ptr<DbnMboReader> reader_;
    
    // Current timestamp (from last processed event)
    uint64_t current_timestamp_ = 0;
    
    // Buffer for the next event (used for lookahead)
    std::optional<MarketEvent> next_event_;
    
    // Get the next event from the reader (with buffering)
    std::optional<MarketEvent> get_next_event();
};
