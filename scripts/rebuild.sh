#!/bin/bash

# Exit on error
set -e

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Go to project root directory (one level up from scripts/)
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== Starting full rebuild ==="
echo "Project root: $PROJECT_ROOT"

# Remove existing build directory if it exists
if [ -d "$BUILD_DIR" ]; then
    echo "Removing existing build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create and enter build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Get number of CPU cores (Windows Git Bash compatible)
if [ -n "$NUMBER_OF_PROCESSORS" ]; then
    # Windows environment variable
    CPU_COUNT=$NUMBER_OF_PROCESSORS
else
    # Linux/MacOS
    CPU_COUNT=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
fi

# Run CMake configuration
echo "Running CMake configuration..."
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="C:/Users/jackd/OneDrive/Documents/vcpkg/scripts/buildsystems/vcpkg.cmake"

# Build the project
echo "Building project with $CPU_COUNT parallel jobs..."
cmake --build . --config Debug --parallel $CPU_COUNT

echo "=== Rebuild completed successfully ==="