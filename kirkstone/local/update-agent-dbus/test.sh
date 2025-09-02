#!/bin/bash

set -e

echo "Running update-agent MOCKED-ONLY tests (no external dependencies)..."

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the project root."
    exit 1
fi

# Create build directory for mocked tests
mkdir -p build-mocked
cd build-mocked

# Copy the mocked CMakeLists file
cp ../tests/CMakeLists_mocked.txt CMakeLists.txt

# Configure with CMake for mocked tests only
echo "Configuring with CMake for mocked tests only..."
cmake .

# Build the mocked tests
echo "Building mocked tests..."
make -j$(nproc)

# Run tests locally
echo "Running mocked tests..."
ctest --output-on-failure --verbose

echo "Mocked tests completed successfully!"
