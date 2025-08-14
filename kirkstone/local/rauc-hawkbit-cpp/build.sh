#!/bin/bash

set -e

echo "Building rauc-hawkbit-cpp with Yocto SDK..."

# Unset LD_LIBRARY_PATH as required by Yocto SDK
unset LD_LIBRARY_PATH

# Source Yocto SDK environment
source /usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux

# Print environment info for debugging
echo "CC: $CC"
echo "CXX: $CXX"
echo "SYSROOT: $SDKTARGETSYSROOT"
echo "PKG_CONFIG_PATH: $PKG_CONFIG_PATH"

# Create build directory
mkdir -p build
cd build

# Configure with CMake using Yocto SDK toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE="/usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake"

# Build
make -j$(nproc)

echo "Build completed successfully!"
echo "Binary location: build/rauc-hawkbit-cpp"