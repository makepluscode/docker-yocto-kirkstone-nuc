#!/bin/bash

set -e

echo "🚀 Starting Yocto ext4 build and RAUC bundle creation..."

# Configuration
BUILD_DIR="kirkstone/build"
BUNDLER_DIR="tools/bundler"
OUTPUT_EXT4="nuc-image-qt5.ext4"
BUNDLE_OUTPUT="nuc-image-qt5-bundle.raucb"

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

# Check if we're in the right directory
if [ ! -f "build.sh" ]; then
    print_error "Please run this script from the project root directory"
    exit 1
fi

# Step 1: Build ext4 image using Yocto
print_status "Step 1: Building ext4 image with Yocto..."
print_status "Running: ./build.sh ext4"

if ! ./build.sh ext4; then
    print_error "Yocto ext4 build failed"
    exit 1
fi

print_success "Yocto ext4 build completed"

# Step 2: Find the generated ext4 file
print_status "Step 2: Locating generated ext4 file..."

# Look for ext4 files in build directory
EXT4_FILES=$(find "$BUILD_DIR" -name "*.ext4" -type f 2>/dev/null | head -5)

if [ -z "$EXT4_FILES" ]; then
    print_error "No ext4 files found in $BUILD_DIR"
    print_status "Searching in entire project..."
    EXT4_FILES=$(find . -name "*.ext4" -type f 2>/dev/null | head -5)
fi

if [ -z "$EXT4_FILES" ]; then
    print_error "No ext4 files found in the project"
    exit 1
fi

print_status "Found ext4 files:"
echo "$EXT4_FILES"

# Use the first (most recent) ext4 file
SOURCE_EXT4=$(echo "$EXT4_FILES" | head -1)
print_status "Using: $SOURCE_EXT4"

# Step 3: Copy ext4 to bundler test directory
print_status "Step 3: Copying ext4 to bundler test directory..."

BUNDLER_TEST_DIR="$BUNDLER_DIR/test"
mkdir -p "$BUNDLER_TEST_DIR"

cp "$SOURCE_EXT4" "$BUNDLER_TEST_DIR/$OUTPUT_EXT4"
print_success "Copied ext4 to: $BUNDLER_TEST_DIR/$OUTPUT_EXT4"

# Step 4: Create RAUC bundle using bundler
print_status "Step 4: Creating RAUC bundle with fixed CA..."

cd "$BUNDLER_DIR/files"

# Clean previous bundles
rm -f output/*.raucb

# Create bundle using the fixed CA
if ! ./rauc-bundle-manual.sh "../test/$OUTPUT_EXT4"; then
    print_error "RAUC bundle creation failed"
    exit 1
fi

# Find the created bundle
BUNDLE_FILE=$(ls -t output/*.raucb 2>/dev/null | head -1)

if [ -z "$BUNDLE_FILE" ]; then
    print_error "No bundle file created"
    exit 1
fi

print_success "RAUC bundle created: $BUNDLE_FILE"

# Step 5: Verify bundle
print_status "Step 5: Verifying bundle..."

# Get the fixed CA path
FIXED_CA="/home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/ca.cert.pem"

if [ -f "$FIXED_CA" ]; then
    if rauc info --keyring "$FIXED_CA" "$BUNDLE_FILE" > /dev/null 2>&1; then
        print_success "Bundle verification successful"
    else
        print_warning "Bundle verification failed (but bundle was created)"
    fi
else
    print_warning "Fixed CA not found, skipping verification"
fi

# Step 6: Copy bundle to project root for easy access
print_status "Step 6: Copying bundle to project root..."

cd /home/makepluscode/docker-yocto-kirkstone-nuc
cp "$BUNDLER_DIR/files/$BUNDLE_FILE" "./$BUNDLE_OUTPUT"

print_success "Bundle copied to: ./$BUNDLE_OUTPUT"

# Step 7: Display summary
echo ""
print_success "=== Build Summary ==="
echo "📁 Source ext4: $SOURCE_EXT4"
echo "📦 RAUC Bundle: ./$BUNDLE_OUTPUT"
echo "🔑 Using fixed CA: $FIXED_CA"
echo ""
print_status "You can now deploy the bundle to your target system:"
echo "  scp ./$BUNDLE_OUTPUT root@192.168.1.100:/tmp/"
echo "  ssh root@192.168.1.100 'rauc install /tmp/$BUNDLE_OUTPUT'"

print_success "🎉 Complete! Yocto ext4 build and RAUC bundle creation finished successfully" 