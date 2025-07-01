# Market Data Processing Module

A high-performance C++20 library for processing financial market data, featuring low-latency order book construction, market data ingestion, and feature engineering capabilities. This module serves as the foundation for quantitative research and algorithmic trading systems.

## ✨ Features

### Core Functionality
- **Market Data Ingestion**
  - Native support for Databento DBN format with Zstandard compression
  - Efficient binary parsing with zero-copy operations
  - Support for only market by order (MBO) data

### Order Book Management
- **Real-time Order Book**
  - Ultra-fast order matching and book building
  - Support for price-time priority matching
  - Configurable depth levels for L2/L3 order books
  - Snapshot and incremental update mechanisms

### Advanced Features
- **Feature Engineering**
  - Built-in calculation of market microstructure features
  - Custom feature registration system
  - Time-series feature computation

### Performance
- **Optimized for HFT**
  - Lock-free data structures where applicable
  - Memory-efficient data representation
  - Low-latency event processing pipeline

### Developer Experience
- **Comprehensive Logging**
  - Configurable log levels
  - Custom log sinks
  - Structured logging support
- **Testing & Validation**
  - Extensive unit test coverage
  - Integration test suite
  - Performance benchmarking

## Dependencies

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.15+
- [Databento C++ Client Library](https://github.com/databento/databento-cpp)
- Zstandard (zstd) library
- OpenSSL

## Building

1. **Clone the repository**
   ```bash
   git clone --recurse-submodules https://github.com/jackdamato/micro-regime-project.git
   cd micro-regime-project/module1
   mkdir -p build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```

2. **Run tests**
   ```bash
   ctest --output-on-failure
   ```

## Example Usage

### Data Requirements

This module processes Market-By-Order (MBO) data in Databento's DBN format. Place your `.dbn.zst` files in the `data/` directory.

### Running Examples

1. **Process DBN file**
   ```bash
   ./examples/dbn_reader ../data/ESM3/20240102_093000.dbn.zst
   ```

2. **Build order book**
   ```bash
   ./examples/order_book_builder ../data/ESM3/20240102_093000.dbn.zst
   ```

3. **Extract features**
   ```bash
   ./examples/feature_extractor \
       --input ../data/ESM3/20240102_093000.dbn.zst \
       --output features.csv \
       --features mid_price,spread,order_imbalance
   ```

## API Overview

### 1. Market Data Processing

```cpp
#include <market_data/dbn_reader.hpp>

// Initialize reader
hft::DbnReader reader;
if (!reader.open("market_data.dbn.zst")) {
    std::cerr << "Failed to open file" << std::endl;
    return 1;
}
```

### 2. Order Book Management

```cpp
#include <order_book/order_book.hpp>

// Create order book
hft::OrderBook book{"ESM3", {.max_levels = 10}};

// Process events
hft::MarketEvent event;
while (reader.next_event(event)) {
    if (event.type == hft::RecordType::Mbo) {
        book.process_event(event);
    }
}
```

### 3. Feature Extraction

```cpp
#include <features/feature_engine.hpp>

// Configure features
hft::FeatureEngine engine{{"mid_price", "spread", "order_imbalance"}};

// Process market data
engine.process_event(market_event);
```