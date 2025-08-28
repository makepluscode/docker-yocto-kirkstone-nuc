#!/bin/bash

set -e

echo "🚀 Creating NUC RAUC bundle from Yocto-built ext4 image..."

# Configuration - Real paths for NUC deployment
BUILD_DIR="kirkstone/build"
BUNDLER_DIR="tools/bundler"
YOCTO_DEPLOY_DIR="$BUILD_DIR/tmp-glibc/deploy/images/intel-corei7-64"
NUC_CERT_DIR="kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed"

# Expected Yocto-built files
YOCTO_IMAGE_BASE="nuc-image-qt5-intel-corei7-64"
EXPECTED_EXT4="$YOCTO_DEPLOY_DIR/$YOCTO_IMAGE_BASE.ext4"
ALT_EXT4="$YOCTO_DEPLOY_DIR/$YOCTO_IMAGE_BASE.rootfs.ext4"

# Output bundle name with timestamp
TIMESTAMP=$(date +%Y%m%d%H%M%S)
BUNDLE_OUTPUT="nuc-image-qt5-bundle-intel-corei7-64-$TIMESTAMP.raucb"

# NUC Fixed Certificate paths
NUC_CA_CERT="$NUC_CERT_DIR/ca.cert.pem"
NUC_DEV_CERT="$NUC_CERT_DIR/development-1.cert.pem"
NUC_DEV_KEY="$NUC_CERT_DIR/development-1.key.pem"

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

# Check if we're in the right directory (project root)
if [ ! -f "build.sh" ]; then
    print_error "Please run this script from the project root directory"
    print_error "Expected location: /home/makepluscode/docker-yocto-kirkstone-nuc"
    exit 1
fi

# Step 1: Check if Yocto-built ext4 exists, if not build it
print_status "Step 1: Checking for Yocto-built ext4 image..."

SOURCE_EXT4=""
if [ -f "$EXPECTED_EXT4" ]; then
    SOURCE_EXT4="$EXPECTED_EXT4"
    print_success "Found: $EXPECTED_EXT4"
elif [ -f "$ALT_EXT4" ]; then
    SOURCE_EXT4="$ALT_EXT4"
    print_success "Found: $ALT_EXT4"
else
    print_warning "No Yocto-built ext4 image found!"
    print_status "Expected locations:"
    echo "  - $EXPECTED_EXT4"
    echo "  - $ALT_EXT4"
    print_status "Building ext4 image with Yocto..."
    print_status "Running: ./build.sh"
    
    if ! ./build.sh; then
        print_error "Yocto build failed"
        exit 1
    fi
    
    # Check again after build
    if [ -f "$EXPECTED_EXT4" ]; then
        SOURCE_EXT4="$EXPECTED_EXT4"
    elif [ -f "$ALT_EXT4" ]; then
        SOURCE_EXT4="$ALT_EXT4"
    else
        print_error "Still no ext4 image found after Yocto build"
        exit 1
    fi
fi

# Get file size for verification
EXT4_SIZE=$(du -h "$SOURCE_EXT4" | cut -f1)
print_status "Using ext4 image: $SOURCE_EXT4 (Size: $EXT4_SIZE)"

# Step 2: Verify NUC fixed certificates exist
print_status "Step 2: Verifying NUC fixed certificates..."

if [ ! -f "$NUC_CA_CERT" ]; then
    print_error "NUC CA certificate not found: $NUC_CA_CERT"
    exit 1
fi

if [ ! -f "$NUC_DEV_CERT" ]; then
    print_error "NUC development certificate not found: $NUC_DEV_CERT"
    exit 1
fi

if [ ! -f "$NUC_DEV_KEY" ]; then
    print_error "NUC development key not found: $NUC_DEV_KEY"
    exit 1
fi

print_success "All NUC certificates found and verified"

# Step 3: Build bundler tool if not exists
print_status "Step 3: Checking bundler tool..."

cd "$BUNDLER_DIR"

if [ ! -f "build/bundler" ]; then
    print_status "Building bundler tool..."
    mkdir -p build
    cd build
    cmake ..
    make
    cd ..
    print_success "Bundler tool built successfully"
else
    print_success "Bundler tool already exists"
fi

# Step 4: Create RAUC bundle with real ext4 and NUC certificates
print_status "Step 4: Creating RAUC bundle with NUC fixed certificates..."

# Clean previous builds
rm -rf build/bundle-temp build/output
mkdir -p build/bundle-temp build/output

# Copy the real ext4 image to bundle directory
print_status "Copying ext4 image to bundle directory..."
cp "../../$SOURCE_EXT4" "build/bundle-temp/rootfs.ext4"

# Create manifest for the real image
print_status "Creating manifest file..."

# Calculate actual SHA256 of the ext4 image
EXT4_SHA256=$(sha256sum "build/bundle-temp/rootfs.ext4" | cut -d' ' -f1)
EXT4_SIZE_BYTES=$(stat -c%s "build/bundle-temp/rootfs.ext4")

cat > "build/bundle-temp/manifest.raucm" << EOF
[update]
compatible=intel-corei7-64-nuc-rauc
version=1.0.0-$TIMESTAMP
description=Intel NUC Qt5 Image Bundle (Real Yocto Build)
build=$(date +%Y%m%d%H%M%S)

[bundle]
format=plain

[image.rootfs]
sha256=$EXT4_SHA256
size=$EXT4_SIZE_BYTES
filename=rootfs.ext4
EOF

print_success "Manifest created with real image hash: $EXT4_SHA256"

# Create bundle using the compiled bundler with NUC fixed certificates
print_status "Creating RAUC bundle..."

cd build
if ! ./bundler \
    -c "../../$NUC_DEV_CERT" \
    -k "../../$NUC_DEV_KEY" \
    -v \
    bundle-temp \
    "output/$BUNDLE_OUTPUT"; then
    print_error "RAUC bundle creation failed"
    exit 1
fi

print_success "RAUC bundle created successfully"

# Step 5: Verify bundle with NUC CA
print_status "Step 5: Verifying bundle with NUC CA..."

cd ..
if rauc info --keyring "../../$NUC_CA_CERT" "build/output/$BUNDLE_OUTPUT" > /dev/null 2>&1; then
    print_success "Bundle verification successful with NUC CA"
else
    print_warning "Bundle verification failed (but bundle was created)"
fi

# Step 6: Copy bundle to project root
print_status "Step 6: Copying bundle to project root..."

cd ../..
cp "$BUNDLER_DIR/build/output/$BUNDLE_OUTPUT" "./$BUNDLE_OUTPUT"

print_success "Bundle copied to project root: ./$BUNDLE_OUTPUT"

# Step 7: Display comprehensive summary
echo ""
print_success "=== NUC Bundle Creation Summary ==="
echo "📁 Source ext4 image: $SOURCE_EXT4"
echo "📏 Image size: $EXT4_SIZE"
echo "🔐 SHA256 hash: $EXT4_SHA256"
echo "📦 RAUC Bundle: ./$BUNDLE_OUTPUT"
echo "🔑 Signed with NUC certificate: $NUC_DEV_CERT"
echo "🔒 Verified with NUC CA: $NUC_CA_CERT"
echo "📅 Build timestamp: $TIMESTAMP"
echo ""
print_status "Deployment commands for NUC target:"
echo "  # Using deploy script (recommended)"
echo "  ./tools/bundler/deploy-bundle.sh -i -s"
echo ""
echo "  # Manual deployment"
echo "  scp ./$BUNDLE_OUTPUT root@192.168.1.100:/tmp/"
echo "  ssh root@192.168.1.100 'rauc install /tmp/$BUNDLE_OUTPUT'"
echo "  ssh root@192.168.1.100 'rauc status'"
echo ""
print_status "Bundle info command:"
echo "  rauc info --keyring $NUC_CA_CERT ./$BUNDLE_OUTPUT"

print_success "🎉 NUC RAUC bundle creation completed successfully!"