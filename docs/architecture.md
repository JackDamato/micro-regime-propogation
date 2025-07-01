# MicroRegime Project Architecture

```mermaid
graph TD
    classDef component fill:#f9f,stroke:#333,stroke-width:2px;
    classDef interface fill:#bbf,stroke:#333,stroke-width:2px;
    classDef data fill:#9f9,stroke:#333,stroke-width:2px;
    
    subgraph Input
        DBN[Databento DBN Files] -->|read by| EventParser
    end
    
    subgraph Core
        EventParser -->|MarketEvent| TimestampPipeline
        subgraph TimestampPipeline
            OrderEngine
            OrderBookManager
            FeatureEngine
            FeatureProcessor
            FeatureNormalizer
            DataReceiver
        end
        OrderEngine -->|updates| OrderBookManager
        OrderBookManager -->|updates| FeatureEngine
        FeatureEngine -->|FeatureInputSnapshot| FeatureProcessor
        FeatureProcessor -->|normalizes using| FeatureNormalizer
        FeatureProcessor -->|sends to| DataReceiver
    end
    
    class EventParser,OrderEngine,OrderBookManager,FeatureEngine,FeatureProcessor,FeatureNormalizer component;
    class MarketEvent,FeatureInputSnapshot data;
    class DataReceiver interface;
```

## Component Overview

## Overview
This document provides a comprehensive overview of the MicroRegime project's architecture, including its components, their relationships, and data flow.

## Core Components

## Class Diagrams

### 1. Core Classes
DBNReader, EventParser, OrderBookManager, OrderEngine, FeatureEngine, FeatureProcessor, FeatureNormalizer, TimestampPipeline, DataReciever

TimestampPipeline holds EventParser, OrderBookManager, OrderEngine, FeatureEngine, FeatureProcessor, FeatureNormalizer, DataReciever For both assets

Event Parser holds DBN Reader, parses and outputs Market Events
OrderEngine holds and OrderBookManager per instrument, is passed a FeatureEngine and MarketEvent to ProcessEvents from Event Parser. Handles calling the right functions in the OrderBookManager and updating the FeatureEngine in the right way
FeatureEngine does not hold anything other than state data about the market, can call generate_snapshot to recieve a FeatureInputSnapshot Data Transfer Object.

FeatureProcessor has a FeatureNormalizer and a Cache. It will take a FeatureInputSnapshot DTO and generate a raw FeatureSet, as well as Normalized FeatureSet's using long and short Welfords Online Windows (with the feature_normalizer_ member variable)

FeatureNormalizer has a long and short Welfords Online Window for each feature. It will take a FeatureSet and normalize it using the long and short Welfords Online Windows (with the feature_normalizer_ member variable)

Timestamp pipeline interface: pass in a date and two asset names and a data reciever. The data reciever will be called with the normalized feature sets for the two assets

```mermaid
classDiagram
    class TimestampPipeline {
        +run(snapshot_interval_ns: uint64_t, base_data_receiver: DataReceiver&, future_data_receiver: DataReceiver&) void
        -getNYSEStartTime(date_str: string) uint64_t
        -getNYSEEndTime(date_str: string) uint64_t
        -order_engine: OrderEngine
        -parser_base_: EventParser
        -parser_future_: EventParser
        -feature_engine_base_: FeatureEngine
        -feature_engine_future_: FeatureEngine
        -feature_processor_: FeatureProcessor
    }

    class EventParser {
        +process_next(engine: OrderEngine&, feature_engine: FeatureEngine*) bool
        +current_timestamp() uint64_t
        +has_more_events() bool
        +get_next_event() optional~MarketEvent~
        -reader_: unique_ptr~DbnMboReader~
        -next_event_: optional~MarketEvent~
    }

    class OrderEngine {
        +process_event(event: MarketEvent&, feature_engine: FeatureEngine* = nullptr) void
        +get_order_book(instrument: string) const OrderBookManager&
        +get_order_books() const unordered_map~string, OrderBookManager~&
        +current_timestamp() uint64_t
        +reset() void
        +get_order_info(order_id: uint64_t) optional~OrderInfo~
        -order_books_: unordered_map~string, OrderBookManager~
        -order_info_: unordered_map~uint64_t, OrderInfo~
        -current_timestamp_: uint64_t
    }

    class OrderBookManager {
        +ApplyAdd(order_id: uint64_t, price: double, size: int, side: BookSide) void
        +ApplyModify(order_id: uint64_t, new_price: double, new_size: int) void
        +ApplyCancel(order_id: uint64_t, canceled_size: int) void
        +ApplyClear() void
        +get_bbo() BBO
        +get_l3_snapshot() L3Snapshot
    }

    class FeatureEngine {
        +generate_snapshot() FeatureInputSnapshot
        +generate_snapshot_from_l3(l3_snapshot: L3Snapshot&, timestamp_ns: uint64_t) FeatureInputSnapshot
        +update_trade(price: double, size: double, direction: int8_t) void
        +update_rolling_state() void
        +update_events(event_type: char) void
        +reset() void
        -order_book_: OrderBookManager&
        -instrument_: string
        -most_recent_timestamp_ns: uint64_t
    }

    class FeatureProcessor {
        +GetRawFeatureSet(snapshot: FeatureInputSnapshot&) FeatureSet
        +GetProcessedFeatureSets(raw_feature_set: FeatureSet&) pair<FeatureSet, FeatureSet>
        -ProcessPriceAndSpread(snapshot: FeatureInputSnapshot&, feature_set: FeatureSet&) void
        -ProcessVolatility(snapshot: FeatureInputSnapshot&, feature_set: FeatureSet&) void
        -ProcessOrderFlow(snapshot: FeatureInputSnapshot&, feature_set: FeatureSet&) void
        -ProcessLiquidity(snapshot: FeatureInputSnapshot&, feature_set: FeatureSet&) void
        -ProcessMicrostructureTransitions(snapshot: FeatureInputSnapshot&, feature_set: FeatureSet&) void
        -ProcessEngineeredFeatures(snapshot: FeatureInputSnapshot&, feature_set: FeatureSet&) void
        -infer_pre_trade_midprice(snap: FeatureInputSnapshot&) double
        -cache_: Cache
        -feature_normalizer_: FeatureNormalizer
    }

    class FeatureNormalizer {
        +AddFeatureSet(feature_set: FeatureSet&) void
        +NormalizeFeatureSet(feature_set: FeatureSet&) pair<FeatureSet, FeatureSet>
        -window_long: deque<FeatureSet>
        -window_short: deque<FeatureSet>
        -feature_sums_long: unordered_map<string, double>
        -feature_sums_2_long: unordered_map<string, double>
        -feature_sums_short: unordered_map<string, double>
        -feature_sums_2_short: unordered_map<string, double>
    }

    class DataReceiver {
        +ingest_feature_set() void
    }

    TimestampPipeline --> EventParser : uses
    TimestampPipeline --> OrderEngine : uses
    TimestampPipeline --> FeatureEngine : uses
    TimestampPipeline --> FeatureProcessor : uses
    TimestampPipeline --> DataReceiver : notifies
    OrderEngine --> OrderBookManager : updates
    OrderEngine --> FeatureEngine : notifies
    FeatureEngine --> OrderBookManager : reads from
    FeatureProcessor --> FeatureNormalizer : uses
```

### 2. Data Transfer Objects

```mermaid
classDiagram
    class MarketEvent {
        +int64_t timestamp_ns
        +std::string instrument
        +ActionType action
        +Side side
        +double price
        +uint32_t size
        +uint64_t order_id
        +uint32_t flags
        +uint16_t instrument_id
        +uint8_t channel_id
        +uint32_t sequence
        +to_string() std::string
    }
    
    class FeatureInputSnapshot {
        uint64_t timestamp_ns;
        std::string instrument;
        double best_bid_price;
        double best_ask_price;
        std::array<double, DEPTH_LEVELS> bid_prices;
        std::array<double, DEPTH_LEVELS> ask_prices;
        std::array<int, DEPTH_LEVELS> bid_sizes;
        std::array<int, DEPTH_LEVELS> ask_sizes;
        double rolling_buy_volume;
        double rolling_sell_volume;
        int adds_since_last_snapshot;
        const std::deque<double>* rolling_midprices;
        const std::deque<double>* rolling_spreads;
        const std::deque<int8_t>* rolling_tick_directions; // +1, 0, -1
        const std::deque<int8_t>* rolling_trade_directions;
        std::array<int8_t, DEPTH_LEVELS> bid_depth_change_direction; // +1, -1
        std::array<int8_t, DEPTH_LEVELS> ask_depth_change_direction;
        double last_trade_price;
        double last_trade_size;
        int8_t last_trade_direction;
        uint32_t reserved = 0;
    }
    class FeatureSet {
        +int64_t timestamp_ns
        +std::string instrument
        +double reversal_rate
        +double spread_crossing
        +double aggressor_bias
        +double log_spread
        +double price_impact
        +double log_return
        +double ewm_volatility
        +double realized_variance
        +double directional_volatility
        +double spread_volatility
        +double ofi
        +double signed_volume_pressure
        +double order_arrival_rate
        +double tick_direction_entropy
        +double depth_imbalance
        +double market_depth
        +double lob_slope
        +double price_gap
        +double shannon_entropy
        +double liquidity_stress
    }
```

## Sequence Diagrams

### 1. Market Data Processing Flow

```mermaid
sequenceDiagram
    participant D as DbnMboReader
    participant E as OrderEngine
    participant O as OrderBookManager
    participant F as FeatureEngine
    participant S as Strategy
    
    D->>E: MarketEvent(Add)
    E->>O: add_order(event)
    O-->>F: OrderBookUpdate(Add)
    F->>F: update_features()
    F->>S: on_features_updated(features)
    
    D->>E: MarketEvent(Trade)
    E->>O: process_trade(event)
    O-->>F: OrderBookUpdate(Trade)
    F->>F: update_features()
    F->>S: on_features_updated(features)
```

### 2. Order Book Update Flow

```mermaid
sequenceDiagram
    participant E as OrderEngine
    participant O as OrderBookManager
    participant F as FeatureEngine
    
    E->>O: add_order(event)
    O->>O: validate_event(event)
    O->>O: update_order_book(Add)
    O->>O: generate_update_message()
    O-->>F: OrderBookUpdate(Add)
    F->>F: update_features()
    F-->>E: features_updated(callback)
```

## Data Flow Diagrams

### 1. System-Level Data Flow

```mermaid
flowchart TD
    A[Databento DBN Files] -->|read| B[DbnMboReader]
    B -->|MarketEvent| C[OrderEngine]
    C -->|route| D[OrderBookManager]
    D -->|update| E[OrderBook State]
    E -->|notify| F[FeatureEngine]
    F -->|features| G[Strategy/Model]
    G -->|orders| H[Exchange]
    H -->|market data| A
    
    style A fill:#f9f,stroke:#333
    style B fill:#bbf,stroke:#333
    style C fill:#bbf,stroke:#333
    style D fill:#bbf,stroke:#333
    style E fill:#9f9,stroke:#333
    style F fill:#bbf,stroke:#333
    style G fill:#fbb,stroke:#333
    style H fill:#fbb,stroke:#333
```

### 2. Feature Computation Flow

```mermaid
flowchart LR
    A[OrderBook Updates] --> B[FeatureEngine]
    B --> C[Feature 1: Mid Price]
    B --> D[Feature 2: Spread]
    B --> E[Feature 3: Order Imbalance]
    B --> F[Feature N: ...]
    C --> G[Feature Vector]
    D --> G
    E --> G
    F --> G
    G --> H[Strategy/Model]
    
    style G fill:#9f9,stroke:#333
    style H fill:#fbb,stroke:#333
```

## Component Details

### 1. Market Data Layer

#### DbnMboReader (`dbn_reader.hpp/cpp`)
- **Purpose**: Reads and parses market-by-order (MBO) data from Databento files
- **Key Features**:
  - Reads MBO records and converts them to `MarketEvent` objects
  - Supports sequential reading of market data
  - Tracks instrument-specific data

#### MarketEvent (`market_event.hpp`)
- **Data Structure**: Represents a single market event
- **Fields**:
  - `timestamp_ns`: Nanosecond timestamp
  - `instrument`: Instrument identifier (e.g., "ES")
  - `action`: Type of event (Add/Modify/Cancel/Trade/Fill/Replace)
  - `side`: Bid/Ask/None
  - `price`: Price level
  - `size`: Order size
  - `order_id`: Unique order identifier
  - `flags`: Bitfield for additional metadata
  - `instrument_id`: Numeric instrument identifier
  - `channel_id`: Feed channel identifier
  - `sequence`: Sequence number for gap detection

### 2. Order Book Management

#### OrderBookManager (`order_book.hpp/cpp`)
- **Purpose**: Maintains the limit order book state
- **Key Features**:
  - Tracks price levels and order queues
  - Handles order add/modify/cancel operations
  - Maintains L3 views of the order book
  - Provides snapshots and deltas of the order book state

#### Data Structures:
- `PriceLevel`: Price and size at a specific level
- `L3Snapshot`: Snapshot of top N price levels for both sides
- `L3Delta`: Changes to the order book since last update
- `Order`: Represents an individual order in the book
- `OrderRef`: Reference to an order's location in the book

### 3. Feature Engineering

#### FeatureEngine (`feature_engine.hpp/cpp`)
- **Purpose**: Computes features from order book state
- **Key Features**:
  - Generates feature snapshots from order book state
  - Maintains rolling windows for time-series features
  - Supports both real-time and batch processing
  - Handles trade and quote updates

#### Feature Types:
- Price-based features (mid-price, spread, etc.)
- Volume-based features
- Order flow metrics
- Statistical measures (volatility, skew, etc.)

### 4. Order Processing

#### OrderEngine (`order_engine.hpp/cpp`)
- **Purpose**: Processes market events and maintains order state
- **Key Features**:
  - Routes events to the appropriate order book
  - Maintains order metadata
  - Handles instrument-specific order books
  - Integrates with FeatureEngine for feature generation

## Data Flow

1. **Data Ingestion**:
   - `DbnMboReader` reads raw market data and produces `MarketEvent` objects

2. **Event Processing**:
   - `OrderEngine` receives `MarketEvent` objects
   - Events are routed to the appropriate `OrderBookManager`
   - Order book state is updated based on the event type

3. **Feature Generation**:
   - `FeatureEngine` observes order book state
   - Features are computed and made available for consumption
   - Features can be used for strategy signals or model training

4. **Output**:
   - Feature snapshots
   - Order book state
   - Processed market data


## Configuration

### Common Constants (`common_constants.hpp`)
- Defines system-wide constants
- Includes:
  - Default order book depth levels
  - Price and size precision
  - System limits

# MicroRegime Project Architecture

## System Overview

```mermaid
graph TD
    A[Market Data Feed] --> B[Event Parser]
    B --> C[Order Engine]
    C --> D[Order Book Manager]
    D --> E[Feature Engine]
    E --> F[Feature Processor]
    F --> G[Feature Normalizer]
    G --> H[Regime Detection]
    H --> I[Analysis & Visualization]
```

## Core Components

### 1. Event Parser
- **Purpose**: Parse incoming market data feed (DBN format)
- **Key Features**:
  - Handles multiple instruments
  - Buffers events for lookahead
  - Validates and normalizes market data

### 2. Order Engine
- **Purpose**: Process market events and maintain order book state
- **Key Features**:
  - Handles order adds, modifies, and cancels
  - Maintains order metadata
  - Tracks order lifecycle

### 3. Order Book Manager
- **Purpose**: Maintain limit order book state
- **Data Structures**:
  - Bid/Ask books using ordered maps
  - Order lookup table
  - Snapshot and delta tracking

### 4. Feature Engine
- **Purpose**: Calculate market microstructure features
- **Features Calculated**:
  - Price and spread metrics
  - Volume and order flow indicators
  - Liquidity measures
  - Volatility estimates

### 5. Feature Processor
- **Purpose**: Process and normalize features
- **Components**:
  - Raw feature calculation
  - Normalization (z-score, min-max)
  - Feature selection

### 6. Regime Detection
- **Purpose**: Identify market regimes using HMM
- **Implementation**:
  - Variational Gaussian HMM
  - Multiple observation models
  - Regime classification

## Data Flow

```mermaid
sequenceDiagram
    participant M as Market Data
    participant P as Event Parser
    participant O as Order Engine
    participant B as Order Book
    participant F as Feature Engine
    participant R as Regime Detection
    participant A as Analysis
    
    M->>P: Raw Market Events
    P->>O: Parsed Events
    O->>B: Update Order Book
    B->>F: Order Book State
    F->>R: Processed Features
    R->>A: Regime Classifications
    A->>A: Statistical Analysis
    A->>A: Visualization
```

## Python Analysis Components

### 1. Regime Analysis (`regime_data_analysis.py`)
- Loads and analyzes regime-classified data
- Generates descriptive statistics for each regime
- Handles missing data and normalization

### 2. HMM Implementation (`ghmm_tester.py`)
- Implements Variational Gaussian HMM
- Handles feature preprocessing (PCA, scaling)
- Trains and evaluates regime models

### 3. Visualization (`graphing.py`, `highlighted_graph.py`)
- Generates time series plots
- Visualizes regime transitions
- Compares feature distributions across regimes

## Performance Considerations

### Memory Efficiency
- Uses fixed-size arrays for price levels
- Implements custom memory management
- Minimizes allocations in hot paths

### Processing Speed
- Optimized for low-latency processing
- Batch processing capabilities
- Efficient data structures for order lookup

## Extensibility

The architecture is designed to be extended with:
- Additional data sources and protocols
- New feature calculations
- Alternative regime detection algorithms
- Real-time monitoring and alerts
- Backtesting framework

## Dependencies

### Core Dependencies
- C++17 or later
- Python 3.8+
- Standard Library
- Databento DBN decoder
- hmmlearn
- pandas
- numpy
- scikit-learn
- matplotlib

## Build and Integration

### C++ Components
```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Python Environment
```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
```

## Data Pipeline

1. **Data Ingestion**:
   - Raw market data in DBN format
   - Multiple instruments (e.g., SPY, ES)
   - High-frequency order book updates

2. **Feature Processing**:
   - Real-time feature calculation
   - Normalization and scaling
   - Dimensionality reduction (PCA)

3. **Regime Analysis**:
   - HMM-based regime detection
   - Statistical analysis of regimes
   - Visualization and reporting

## Future Work

- Implement online learning for regime detection
- Add more sophisticated feature engineering
- Integrate with trading strategies
- Develop real-time monitoring dashboard
