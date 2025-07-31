# Data Ingestion Module

A C++20 library for processing and normalizing market data for micro-regime classification. This module handles the ingestion of market data, feature extraction, and normalization for use in machine learning models.

## Features

- **Data Processing**
  - Handles market data events with nanosecond precision
  - Maintains a rolling window of market state for feature calculation
  - Configurable depth levels for order book analysis
  
- **Feature Extraction**
  - Real-time calculation of market microstructure features
  - Normalization of features using configurable window sizes
  - Efficient storage of historical feature values

- **Performance**
  - Optimized for low-latency processing
  - Memory-efficient data structures
  - Configurable window sizes for different time horizons

## Dependencies

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.15+
- Standard Library with C++20 support

## Configuration

Key configuration parameters (defined in `include/common_constants.hpp`):

- `DEPTH_LEVELS`: Number of price levels to maintain in the order book (default: 10)
- `ROLLING_WINDOW`: Window size for feature input calculations (default: 1500 events)
- `WINDOW_SIZE`: Window size for feature normalization (default: 30000 events)
- `SNAPSHOT_INTERVAL_NS`: Interval for taking snapshots of the market state (default: 500ms)

## Building

1. **Create build directory and run CMake**
   ```bash
   mkdir -p build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```

2. **Run tests**
   ```bash
   ctest --output-on-failure
   ```

## Project Structure

- `include/`: Header files
  - `common_constants.hpp`: Global configuration parameters
  - (Other header files...)
- `src/`: Implementation files
- `tests/`: Unit tests

## Usage

Include the necessary headers and use the provided classes to process market data. The module is designed to be integrated into a larger market data processing pipeline.

```cpp
#include "include/common_constants.hpp"
// Other includes...

// Example usage would go here
```

## Contributing

Please refer to the project's coding standards and submit pull requests for any improvements or bug fixes.