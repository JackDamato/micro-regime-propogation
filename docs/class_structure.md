# MicroRegime Project - Class Structure and Data Flow

## Core Components

### 1. Event Parser (`EventParser`)
- **Purpose**: Parses market data events from DBN files
- **Key Responsibilities**:
  - Read and decode market data events from DBN files
  - Maintain event stream state (current timestamp, buffering)
  - Feed events to the OrderEngine
- **Key Methods**:
  - `process_all(OrderEngine&)`: Process all events
  - `process_until(OrderEngine&, uint64_t)`: Process until specified timestamp
  - `process_next(OrderEngine&)`: Process single event
  - `has_more_events()`: Check if more events are available

### 2. Order Engine (`OrderEngine`)
- **Purpose**: Maintains order book state and processes market events
- **Key Responsibilities**:
  - Maintain order books for multiple instruments
  - Process market events (add, modify, cancel orders)
  - Track order metadata
- **Key Methods**:
  - `process_event(const MarketEvent&)`: Process a single market event
  - `get_order_book(const std::string&)`: Get order book for instrument
  - `get_order_info(uint64_t)`: Lookup order information
  - `reset()`: Clear all state

### 3. Order Book (`OrderBookManager`)
- **Purpose**: Maintains limit order book state for a single instrument
- **Key Components**:
  - Bid and ask price levels
  - Order tracking
  - Depth management
- **Key Functionality**:
  - Add/modify/cancel orders
  - Calculate order book metrics
  - Maintain price-time priority

### 4. Feature Snapshot (`FeatureInputSnapshot`)
- **Purpose**: Snapshot of market state features for ML/Analysis
- **Key Components**:
  - Top of book state (prices, sizes)
  - Depth levels (configurable, default 5 levels)
  - Rolling statistics (volume, spread, etc.)
  - Trade execution data
  - Depth change tracking

## Data Flow

1. **Data Ingestion**:
   ```
   DBN File → DbnMboReader → EventParser
   ```

2. **Event Processing**:
   ```
   EventParser → OrderEngine → OrderBookManager
   ```

3. **Feature Generation**:
   ```
   OrderBookManager → FeatureInputSnapshot
   ```

## Key Data Structures

### MarketEvent
- Represents a single market data event
- Contains order details (add, modify, cancel)
- Includes timestamp and instrument information

### OrderInfo
- Tracks metadata for individual orders
- Contains side, price, and instrument information
- Used for order lookup and validation

## Configuration
- `DEPTH_LEVELS`: Number of price levels to track (default: 5)
- `ROLLING_WINDOW`: Window size for rolling calculations (default: 50)

## Dependencies
- `databento-cpp`: For DBN file reading
- Standard Library: Containers, memory management, I/O

## Thread Safety
- Current implementation is single-threaded
- Order book operations are not thread-safe
- Consider thread safety for future parallel processing

## Performance Considerations
- Minimize memory allocations in hot paths
- Use move semantics where appropriate
- Consider memory locality for performance-critical sections
- Profile before optimizing
