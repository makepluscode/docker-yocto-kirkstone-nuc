#!/bin/bash

set -e

echo "Building rauc-hawkbit-cpp..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

echo "Build completed successfully!"
echo "Binary location: build/rauc-hawkbit-cpp" 