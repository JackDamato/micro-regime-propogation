# MicroRegime Project Architecture

```mermaid
graph TD
    classDef component fill:#f9f,stroke:#333,stroke-width:2px;
    classDef interface fill:#bbf,stroke:#333,stroke-width:2px;
    classDef data fill:#9f9,stroke:#333,stroke-width:2px;
    
    subgraph Input
        DBN[Databento DBN Files] -->|read by| DbnMboReader
    end
    
    subgraph Core
        DbnMboReader -->|MarketEvent| OrderEngine
        OrderEngine -->|updates| OrderBookManager
        OrderBookManager -->|state| FeatureEngine
    end
    
    subgraph Output
        FeatureEngine -->|features| Strategy
        FeatureEngine -->|features| Model
        OrderBookManager -->|snapshots| DataStore
    end
    
    class DbnMboReader,OrderEngine,OrderBookManager,FeatureEngine component;
    class MarketEvent,features,snapshots data;
    class Strategy,Model,DataStore interface;
```

## Component Overview

## Overview
This document provides a comprehensive overview of the MicroRegime project's architecture, including its components, their relationships, and data flow.

## Core Components

## Class Diagrams

### 1. Core Classes

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
    
    class OrderBookManager {
        +add_order(MarketEvent) void
        +modify_order(MarketEvent) void
        +cancel_order(MarketEvent) void
        +get_bbo() BBO
        +get_l2_snapshot() L2Snapshot
        +get_l3_snapshot() L3Snapshot
        -process_add(MarketEvent) void
        -process_modify(MarketEvent) void
        -process_cancel(MarketEvent) void
    }
    
    class FeatureEngine {
        +on_market_event(MarketEvent) void
        +on_order_book_update(OrderBookUpdate) void
        +get_features() FeatureSnapshot
        +register_feature(std::string, FeatureType) void
        -update_features() void
    }
    
    class OrderEngine {
        +process_event(MarketEvent) void
        +get_order_book(std::string) OrderBookManager*
        +add_instrument(std::string) void
        -order_books_ std::unordered_map<std::string, OrderBookManager>
    }
    
    MarketEvent --> OrderEngine : processes
    OrderEngine --> OrderBookManager : updates
    OrderBookManager --> FeatureEngine : notifies
```

### 2. Data Structures

```mermaid
classDiagram
    class L2Snapshot {
        +int64_t timestamp_ns
        +std::string instrument
        +std::vector<PriceLevel> bids
        +std::vector<PriceLevel> asks
        +to_json() std::string
    }
    
    class L3Snapshot {
        +int64_t timestamp_ns
        +std::string instrument
        +std::map<Price, std::vector<Order>> bids
        +std::map<Price, std::vector<Order>> asks
        +to_json() std::string
    }
    
    class FeatureSnapshot {
        +int64_t timestamp_ns
        +std::string instrument
        +std::unordered_map<std::string, double> features
        +add_feature(std::string, double) void
        +get_feature(std::string) double
        +to_json() std::string
    }
    
    class OrderBookUpdate {
        +enum class Type { Add, Modify, Cancel, Trade }
        +Type type
        +MarketEvent event
        +L2Snapshot snapshot
        +std::vector<L2Delta> deltas
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
  - Maintains both L2 and L3 views of the order book
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

## Performance Considerations

- **Memory Efficiency**:
  - Uses fixed-size arrays for price levels
  - Implements custom memory management
  - Minimizes allocations in hot paths

- **Processing Speed**:
  - Optimized for low-latency processing
  - Batch processing capabilities
  - Efficient data structures for order lookup

## Extensibility

The architecture is designed to be extended with:
- Additional data sources
- New feature calculations
- Alternative order matching logic
- Different market data protocols

## Dependencies

- C++17 or later
- Standard Library
- Databento DBN decoder
- (Add other dependencies as needed)

## Build and Integration

(Add build system and integration details here)
