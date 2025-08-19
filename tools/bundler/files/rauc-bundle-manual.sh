#!/bin/bash

# Manual RAUC Bundle Creation Script
# This script demonstrates how to create RAUC bundles manually
# without the full Yocto build system

set -e

# Configuration
BUNDLE_NAME="nuc-image-qt5-bundle"
BUNDLE_VERSION="0.0.1"
BUNDLE_COMPATIBLE="intel-i7-x64-nuc-rauc"
BUNDLE_DESCRIPTION="Intel NUC Qt5 Image"

# Directory setup
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KEYS_DIR="${SCRIPT_DIR}/example-ca"
BUNDLE_DIR="${SCRIPT_DIR}/bundle-temp"
OUTPUT_DIR="${SCRIPT_DIR}/output"

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

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to calculate SHA256 hash
calculate_sha256() {
    local file="$1"
    if command_exists sha256sum; then
        sha256sum "$file" | cut -d' ' -f1
    elif command_exists shasum; then
        shasum -a 256 "$file" | cut -d' ' -f1
    else
        print_error "Neither sha256sum nor shasum found. Cannot calculate hash."
        exit 1
    fi
}

# Function to create manifest
create_manifest() {
    local manifest_file="$1"
    local rootfs_file="$2"
    
    print_status "Creating manifest file: $manifest_file"
    
    # Calculate file size and hash
    local file_size=$(stat -c%s "$rootfs_file" 2>/dev/null || stat -f%z "$rootfs_file" 2>/dev/null)
    local file_hash=$(calculate_sha256 "$rootfs_file")
    
    cat > "$manifest_file" << EOF
[update]
compatible=$BUNDLE_COMPATIBLE
version=$BUNDLE_VERSION
description=$BUNDLE_DESCRIPTION
build=$(date '+%Y%m%d%H%M%S')

[bundle]
format=plain

[image.rootfs]
filename=$(basename "$rootfs_file")
size=$file_size
sha256=$file_hash
EOF
    
    print_success "Manifest created successfully"
}

# Function to create bundle
create_bundle() {
    local bundle_dir="$1"
    local output_file="$2"
    local cert_file="$3"
    local key_file="$4"
    
    print_status "Creating RAUC bundle: $output_file"
    
    # Check if rauc is available
    if ! command_exists rauc; then
        print_error "RAUC is not installed. Please install RAUC tools first."
        print_error "On Ubuntu/Debian: sudo apt-get install rauc"
        print_error "On CentOS/RHEL: sudo yum install rauc"
        exit 1
    fi
    
    # Create bundle
    rauc bundle \
        --cert="$cert_file" \
        --key="$key_file" \
        "$bundle_dir" \
        "$output_file"
    
    print_success "Bundle created successfully: $output_file"
}

# Function to verify bundle
verify_bundle() {
    local bundle_file="$1"
    local keyring_file="$2"
    
    print_status "Verifying bundle: $bundle_file"
    
    # Show bundle info
    echo "=== Bundle Information ==="
    rauc info "$bundle_file"
    echo ""
    
    # Verify bundle
    if [ -n "$keyring_file" ] && [ -f "$keyring_file" ]; then
        echo "=== Bundle Verification ==="
        rauc verify --keyring="$keyring_file" "$bundle_file"
        print_success "Bundle verification completed"
    else
        print_warning "No keyring file provided, skipping verification"
    fi
}

# Function to extract bundle
extract_bundle() {
    local bundle_file="$1"
    local extract_dir="$2"
    
    print_status "Extracting bundle to: $extract_dir"
    
    mkdir -p "$extract_dir"
    rauc extract "$bundle_file" "$extract_dir"
    
    print_success "Bundle extracted successfully"
    echo "Extracted contents:"
    ls -la "$extract_dir"
}

# Main function
main() {
    print_status "Starting manual RAUC bundle creation"
    
    # Check if keys exist
    if [ ! -f "$KEYS_DIR/development-1.cert.pem" ] || [ ! -f "$KEYS_DIR/private/development-1.key.pem" ]; then
        print_error "RAUC keys not found. Please run create-example-keys.sh first."
        exit 1
    fi
    
    # Create output directory
    mkdir -p "$OUTPUT_DIR"
    
    # Check if rootfs file is provided
    if [ $# -eq 0 ]; then
        print_error "Usage: $0 <rootfs_file>"
        print_error "Example: $0 /path/to/rootfs.ext4"
        exit 1
    fi
    
    local rootfs_file="$1"
    
    # Check if rootfs file exists
    if [ ! -f "$rootfs_file" ]; then
        print_error "Rootfs file not found: $rootfs_file"
        exit 1
    fi
    
    # Create bundle directory
    mkdir -p "$BUNDLE_DIR"
    
    # Copy rootfs to bundle directory
    print_status "Copying rootfs file to bundle directory"
    cp "$rootfs_file" "$BUNDLE_DIR/"
    
    # Create manifest
    create_manifest "$BUNDLE_DIR/manifest.raucm" "$BUNDLE_DIR/$(basename "$rootfs_file")"
    
    # Create bundle
    local output_file="$OUTPUT_DIR/${BUNDLE_NAME}-${BUNDLE_VERSION}.raucb"
    create_bundle "$BUNDLE_DIR" "$output_file" \
        "$KEYS_DIR/development-1.cert.pem" \
        "$KEYS_DIR/private/development-1.key.pem"
    
    # Verify bundle
    verify_bundle "$output_file" "$KEYS_DIR/ca.cert.pem"
    
    # Optional: Extract bundle for inspection
    if [ "${2:-}" = "--extract" ]; then
        local extract_dir="$OUTPUT_DIR/extracted-${BUNDLE_NAME}"
        extract_bundle "$output_file" "$extract_dir"
    fi
    
    # Cleanup
    print_status "Cleaning up temporary files"
    rm -rf "$BUNDLE_DIR"
    
    print_success "Manual RAUC bundle creation completed!"
    echo "Bundle file: $output_file"
    echo "Bundle size: $(du -h "$output_file" | cut -f1)"
}

# Check if script is being sourced
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi 