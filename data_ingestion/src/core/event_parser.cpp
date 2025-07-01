#include "event_parser.hpp"
#include <stdexcept>

EventParser::EventParser(const std::string& dbn_filepath, const std::string& instrument)
    : reader_(std::make_unique<DbnMboReader>(dbn_filepath, instrument)) {
    // Pre-fetch the first event
    next_event_ = get_next_event();
    if (next_event_) {
        current_timestamp_ = next_event_->timestamp_ns;
    }
}

void EventParser::process_all(OrderEngine& engine) {
    while (has_more_events()) {
        process_next(engine);
    }
}

bool EventParser::process_until(OrderEngine& engine, uint64_t stop_timestamp_ns) {
    bool processed_any = false;
    
    while (has_more_events()) {
        // Check if the next event is after the stop time
        if (next_event_ && next_event_->timestamp_ns > stop_timestamp_ns) {
            current_timestamp_ = stop_timestamp_ns;
            return true; // More events available after stop time
        }
        
        // Process the next event
        if (process_next(engine)) {
            processed_any = true;
        } else {
            break;
        }
    }
    
    return has_more_events();
}

bool EventParser::process_next(OrderEngine& engine, FeatureEngine* feature_engine) {
    if (!next_event_) {
        return false;
    }
    // Process the current event
    engine.process_event(*next_event_, feature_engine);

    // Update our timestamp
    current_timestamp_ = next_event_->timestamp_ns;
    
    // Get the next event
    next_event_ = get_next_event();
    
    return true;
}

bool EventParser::has_more_events() const {
    return next_event_.has_value();
}

std::optional<MarketEvent> EventParser::get_next_event() {
    if (!reader_ || !reader_->has_next()) {
        return std::nullopt;
    }
    
    try {
        return reader_->next_event();
    } catch (const std::exception& e) {
        // Log the error and continue with the next event if possible
        // In a production system, you might want to handle this differently
        std::cerr << "Error reading event: " << e.what() << std::endl;
        return get_next_event(); // Try to get the next event
    }
}
