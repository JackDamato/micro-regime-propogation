# Module 1: Market Data Ingestion & Order Book Processor - Layout

## Complete Directory Structure

```
module1/
├── include/                      # Public API headers
│   ├── common_constants.hpp      # System-wide constants and configuration
│   ├── dbn_reader.hpp           # Interface for reading Databento DBN files
│   ├── event_parser.hpp         # Market event parsing and validation
│   ├── exports.hpp              # Platform-specific export macros
│   ├── feature_engine.hpp       # Feature computation and management
│   ├── feature_snapshot.hpp     # Snapshot of computed features
│   ├── market_event.hpp         # Market event data structures
│   ├── order_book.hpp           # Order book implementation
│   ├── order_engine.hpp         # Market event processing engine
│   └── types.hpp                # Common type definitions and utilities
├── src/                         # Implementation source files
│   ├── core/                    # Core processing components
│   │   ├── event_parser.cpp    # Market event parsing implementation
│   │   ├── feature_engine.cpp  # Feature computation logic
│   │   ├── order_book.cpp      # Order book implementation
│   │   ├── order_book_metrics.cpp # Order book analytics
│   │   └── order_engine.cpp    # Event processing implementation
│   ├── data/                   # Data source handling
│   │   └── dbn_reader.cpp      # DBN file reading implementation
│   └── utils/                  # Utility components
│       ├── logger.cpp          # Logging utilities
│       ├── stats.cpp           # Statistical functions
│       └── timer.cpp           # High-precision timing utilities
├── tests/                      # Test suite
│   ├── CMakeLists.txt          # Test build configuration
│   ├── test_dbn_reader.cpp     # DBN reader test cases
│   └── test_pipeline.cpp       # End-to-end pipeline tests
├── bench/                      # Performance benchmarks
│   ├── feature_engine_bench.cpp # Feature computation benchmarks
│   └── order_book_bench.cpp    # Order book performance tests
├── scripts/                    # Build and utility scripts
├── CMakeLists.txt              # Main build configuration
└── README.md                   # Module documentation
```

## Detailed File Descriptions

### Core Headers (`include/`)

1. **common_constants.hpp**  
   Defines system-wide constants including default buffer sizes, price/size precision, and order book configuration parameters.

2. **dbn_reader.hpp**  
   Provides interface for reading and parsing Databento DBN files, supporting various market data record types.

3. **event_parser.hpp**  
   Handles parsing and validation of raw market events, converting them to internal representations.

4. **exports.hpp**  
   Contains platform-specific export macros for building shared libraries with proper symbol visibility.

5. **feature_engine.hpp**  
   Core feature computation engine that processes market data to generate trading signals and metrics.

6. **feature_snapshot.hpp**  
   Defines the structure for capturing and serializing feature snapshots at specific timestamps.

7. **market_event.hpp**  
   Contains data structures for representing market events including order actions, trades, and book updates.

8. **order_book.hpp**  
   Implements a high-performance limit order book with support for L2/L3 order book reconstruction.

9. **order_engine.hpp**  
   Orchestrates the processing of market events through the order book and feature pipeline.

10. **types.hpp**  
    Defines common types, enums, and utility functions used throughout the codebase.

### Core Implementation (`src/core/`)

1. **event_parser.cpp**  
   Implements parsing and validation logic for different market event types.

2. **feature_engine.cpp**  
   Contains the implementation of feature computation and management logic.

3. **order_book.cpp**  
   Core order book implementation including order matching and book maintenance.

4. **order_book_metrics.cpp**  
   Implements analytical functions for order book metrics and statistics.

5. **order_engine.cpp**  
   Processes market events through the order book and coordinates feature updates.

### Data Layer (`src/data/`)
1. **dbn_reader.cpp**  
   Implements the DBN file format reader with support for various market data record types.

### Utilities (`src/utils/`)
1. **logger.cpp**  
   Implements logging functionality with configurable log levels and outputs.

2. **stats.cpp**  
   Provides statistical functions and metrics calculations.

3. **timer.cpp**  
   High-precision timing utilities for performance measurement.

### Testing (`tests/`)
1. **test_dbn_reader.cpp**  
   Unit tests for the DBN file reader functionality.

2. **test_pipeline.cpp**  
   End-to-end tests for the market data processing pipeline.

### Benchmarks (`bench/`)
1. **feature_engine_bench.cpp**  
   Performance benchmarks for feature computation.

2. **order_book_bench.cpp**  
   Microbenchmarks for order book operations.

### Build System
1. **CMakeLists.txt**  
   Main build configuration file defining targets, dependencies, and build options.

2. **tests/CMakeLists.txt**  
   Test-specific build configuration and test case registration.
   - Platform-specific export/import macros
   - DLL/SO symbol visibility control

### Core Implementation (`src/core/`)

1. **order_book.cpp**
   - Full-depth order book implementation
   - Price-time priority matching engine
   - L3 snapshot generation

2. **feature_engine.cpp**
   - Rolling feature calculations
   - Microstructure metrics computation
   - Feature vector assembly

### Data Handling (`src/data/`)

1. **dbn_reader.cpp**
   - Databento DBN file format reader
   - Streaming decompression (zstd)
   - Event parsing and normalization


2. **event_parser.cpp**
   - Raw market data parsing
   - Event type detection and validation
   - Normalization to internal event format

### Utilities (`src/utils/`)

1. **logger.cpp**
   - High-performance logging
   - Log levels and filtering
   - Asynchronous log writing

2. **stats.cpp**
   - Performance metrics collection
   - Statistics calculation
   - Monitoring interface

3. **timer.cpp**
   - High-resolution timing utilities
   - Latency measurement
   - Rate limiting

### Tests (`tests/`)

1. **test_dbn_reader.cpp**
   - DBN file reader tests

2. **test_pipeline.cpp**
   - End-to-end tests for the market data processing pipeline.


### Benchmarks (`bench/`)

1. **order_book_bench.cpp**
   - Order matching performance
   - Memory usage profiling
   - Scaling characteristics

2. **feature_engine_bench.cpp**
   - Feature calculation speed
   - Memory access patterns
   - SIMD optimization verification

### Scripts (`scripts/`)

1. **build.sh**
   - Build configuration
   - Dependency management
   - Build artifact handling

2. **run_tests.sh**
   - Test execution
   - Coverage reporting
   - Continuous integration integration

### Build System (`CMakeLists.txt`)
- Module compilation settings
- Dependency management
- Test and benchmark targets
- Installation rules

### Documentation (`README.md`)
- Module overview
- Build and usage instructions
- API documentation
- Performance characteristics

## Data Flow

```
┌─────────────┐    ┌─────────────┐    ┌─────────────────┐
│ Data Source │───▶│  Event      │───▶│    Order       │
│  (DBN/UDP)  │    │  Parser    │    │    Book        │
└─────────────┘    └─────────────┘    └────────┬────────┘
                                               │
┌─────────────┐    ┌─────────────┐    ┌───────▼───────┐
│ Monitoring  │    │  Feature    │◀───┤  Snapshot     │
│ & Logging   │◀──▶│  Engine     │    │  Emitter      │
└─────────────┘    └─────────────┘    └───────┬───────┘
                                               │
                                         ┌─────▼─────┐
                                         │  Output   │
                                         │  Buffer   │
                                         └───────────┘
```

## Build Instructions

1. Clone the repository
2. Install dependencies (CMake, C++20 compiler, zstd)
3. Configure build:
   ```bash
   mkdir build && cd build
   cmake ..
   ```
4. Build:
   ```bash
   cmake --build .
   ```
5. Run tests:
   ```bash
   ctest
   ```

## Performance Targets

- <10µs per event processing
- <100µs snapshot emission interval
- <1ms end-to-end latency (99th percentile)
- Support for 1M+ events/second

## Dependencies

- C++20 or later
- zstd (for DBN decompression)
- Google Test (for testing)
- Google Benchmark (for benchmarking)
- CMake 3.20+

## Contributing

1. Fork the repository
2. Create a feature branch
3. Write tests for new features
4. Submit a pull request

## License

[Specify License]
