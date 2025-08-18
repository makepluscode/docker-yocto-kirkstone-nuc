#!/bin/bash

# Dashboard Build Script for Yocto SDK
# This script sets up the Yocto SDK environment and builds the dashboard project

set -e  # Exit on any error

echo "🚀 Dashboard Build Script for Yocto SDK"
echo "========================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the dashboard directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "This script must be run from the dashboard directory"
    exit 1
fi

# Check if Yocto SDK is installed
SDK_PATH="/usr/local/oecore-x86_64"
if [ ! -d "$SDK_PATH" ]; then
    print_error "Yocto SDK not found at $SDK_PATH"
    print_error "Please install the Yocto SDK first"
    exit 1
fi

# Unset LD_LIBRARY_PATH if it's set (common issue with Yocto SDK)
if [ -n "$LD_LIBRARY_PATH" ]; then
    print_warning "Unsetting LD_LIBRARY_PATH for Yocto SDK compatibility"
    unset LD_LIBRARY_PATH
fi

print_status "Setting up Yocto SDK environment..."
source "$SDK_PATH/environment-setup-corei7-64-oe-linux"

# Verify environment setup
if [ -z "$CC" ] || [ -z "$CXX" ]; then
    print_error "Failed to set up Yocto SDK environment"
    exit 1
fi

print_success "Yocto SDK environment configured"
print_status "Compiler: $CC"
print_status "C++ Compiler: $CXX"
print_status "Sysroot: $PKG_CONFIG_SYSROOT_DIR"

# Clean previous build
if [ -d "build" ]; then
    print_status "Cleaning previous build..."
    rm -rf build
fi

# Create build directory
print_status "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
print_status "Configuring with CMake..."
cmake .. -DCMAKE_TOOLCHAIN_FILE="$SDK_PATH/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake"

# Check if CMake configuration was successful
if [ $? -ne 0 ]; then
    print_error "CMake configuration failed"
    exit 1
fi

print_success "CMake configuration completed"

# Build the project
print_status "Building dashboard..."
make -j$(nproc)

# Check if build was successful
if [ $? -ne 0 ]; then
    print_error "Build failed"
    exit 1
fi

print_success "Dashboard build completed successfully!"

# Check if the executable was created
if [ -f "dashboard" ]; then
    print_success "Executable created: $(pwd)/dashboard"
    print_status "File type: $(file dashboard)"
else
    print_warning "Executable not found, but build completed"
fi

echo ""
print_success "Build process completed!"
print_status "You can now run the dashboard with: ./build/dashboard"
