#include "dbn_reader.hpp"
#include <databento/dbn_decoder.hpp>
#include <databento/log.hpp>
#include <stdexcept>

using namespace databento;

// PIMPL implementation
struct DbnMboReader::Impl {
    // prevents implicit conversions of string literals to Impl type
    explicit Impl(const std::string& filepath) 
        : file_stream_{filepath}, decoder_{&log_receiver_, std::move(file_stream_)} {
        // Decode metadata first
        metadata_ = decoder_.DecodeMetadata();
    }

    // Logger that does nothing, databento::DbnDecoder() requires a pointer to an ILogReceiver
    struct NullLogReceiver : public ILogReceiver {
        void Receive(LogLevel, const std::string&) noexcept override {}
    } log_receiver_;
    
    InFileStream file_stream_;
    DbnDecoder decoder_{&log_receiver_, std::move(file_stream_)};
    Metadata metadata_;
    const Record* next_record_{nullptr};
};

// Constructor creates pimpl and decodes the first record
DbnMboReader::DbnMboReader(const std::string& filepath, const std::string& instrument)
    : instrument_{instrument},
      pimpl_{std::make_unique<Impl>(filepath)} {
    // Prime the first record
    pimpl_->next_record_ = pimpl_->decoder_.DecodeRecord();
}

DbnMboReader::~DbnMboReader() = default;

bool DbnMboReader::has_next() const {
    return pimpl_->next_record_ != nullptr;
}

// Returns the next parsed MBO record as a MarketEvent
MarketEvent DbnMboReader::next_event() {
    if (!has_next()) {
        throw std::runtime_error("No more events to read");
    }

    // Store current record and move to next record.
    const auto* record = pimpl_->next_record_;
    pimpl_->next_record_ = pimpl_->decoder_.DecodeRecord();
    ++event_count_;

    MarketEvent event{};
    
    // Map the DBN record to our MarketEvent
    if (record->RType() == RType::Mbo) {
        const auto& mbo = record->Get<MboMsg>();
        // Convert UnixNanos (time_point) to uint64_t by getting time since epoch
        event.timestamp_ns = mbo.ts_recv.time_since_epoch().count();
        event.instrument = instrument_;
        event.action = static_cast<char>(mbo.action);
        event.side = mbo.side == Side::Bid ? 'B' : 
                     mbo.side == Side::Ask ? 'A' : 'N';
        event.price = static_cast<double>(mbo.price) / 1e9;
        event.size = static_cast<int>(mbo.size);
        event.order_id = mbo.order_id;
        event.flags = static_cast<uint8_t>(mbo.flags);
        // Get the instrument_id from the record's header
        event.instrument_id = record->Header().instrument_id;
        event.channel_id = mbo.channel_id;
        event.sequence = mbo.sequence;
    } else {
        // Skip non-MBO records
        return next_event();
    }

    return event;
}