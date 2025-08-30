#!/bin/bash

# Unified RAUC Bundle Creation Script
# This script consolidates all RAUC bundle creation functionality
# into a single, comprehensive tool that supports multiple workflows

set -e

# Script information
SCRIPT_VERSION="1.0.0"
SCRIPT_NAME="create-rauc-bundle.sh"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
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

print_debug() {
    if [ "${VERBOSE:-}" = "true" ]; then
        echo -e "${PURPLE}[DEBUG]${NC} $1"
    fi
}

print_header() {
    echo -e "${CYAN}=== $1 ===${NC}"
}

# Configuration variables with defaults
BUNDLE_NAME="nuc-image-qt5-bundle"
BUNDLE_VERSION="1.0.0"
BUNDLE_COMPATIBLE="intel-i7-x64-nuc-rauc"
BUNDLE_DESCRIPTION="Intel NUC Qt5 Image Bundle"
VERBOSE="false"
AUTO_BUILD="false"
EXTRACT_BUNDLE="false"
VERIFY_BUNDLE="true"
CLEANUP_TEMP="true"

# Directory paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUNDLER_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/kirkstone/build"
YOCTO_DEPLOY_DIR="$BUILD_DIR/tmp-glibc/deploy/images/intel-corei7-64"
KEYS_DIR="$PROJECT_ROOT/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed"
BUNDLE_TEMP_DIR="$BUNDLER_DIR/build/bundle-temp"
OUTPUT_DIR="$BUNDLER_DIR/build/output"

# Certificate paths
CA_CERT="$KEYS_DIR/ca.cert.pem"
DEV_CERT="$KEYS_DIR/development-1.cert.pem"
DEV_KEY="$KEYS_DIR/development-1.key.pem"

# Function to show help
show_help() {
    cat << EOF
$SCRIPT_NAME - Unified RAUC Bundle Creation Tool v$SCRIPT_VERSION

USAGE:
    $0 [OPTIONS] [EXT4_FILE]

DESCRIPTION:
    Creates RAUC update bundles for Intel NUC systems with multiple workflow support:
    - Auto mode: Automatically builds Yocto ext4 image and creates bundle
    - Manual mode: Creates bundle from existing ext4 file
    - Yocto mode: Uses existing Yocto-built ext4 images

OPTIONS:
    -h, --help              Show this help message
    -v, --verbose           Enable verbose output
    -a, --auto-build        Automatically build Yocto image if not found
    -e, --extract           Extract bundle after creation for inspection
    -n, --no-verify         Skip bundle verification
    -c, --no-cleanup        Keep temporary files after completion
    --name NAME             Set bundle name (default: $BUNDLE_NAME)
    --version VER           Set bundle version (default: $BUNDLE_VERSION)
    --compatible COMPAT     Set bundle compatibility string (default: $BUNDLE_COMPATIBLE)
    --description DESC      Set bundle description

WORKFLOWS:
    1. Auto workflow (no arguments):
       $0 -a
       Searches for Yocto-built ext4, builds if not found, creates bundle

    2. Manual workflow (with ext4 file):
       $0 /path/to/rootfs.ext4
       Creates bundle from specified ext4 file

    3. Yocto workflow (from project root):
       $0
       Uses existing Yocto-built ext4 images

EXAMPLES:
    # Auto build and create bundle
    $0 --auto-build --verbose

    # Create bundle from specific ext4 file
    $0 /path/to/custom-rootfs.ext4

    # Create and extract bundle for inspection
    $0 --extract --verbose

    # Custom bundle configuration
    $0 --name "custom-bundle" --version "2.0.0" /path/to/rootfs.ext4

ENVIRONMENT:
    The script expects to be run from the project root or bundler directory.
    Required certificates should be present in:
    $KEYS_DIR/
EOF
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Note: SHA256 calculation removed as RAUC auto-calculates hashes during bundle creation

# Function to check directory and setup paths
setup_environment() {
    print_debug "Setting up environment..."
    
    # Try to determine project root
    if [ ! -f "$PROJECT_ROOT/build.sh" ]; then
        # Maybe we're in bundler directory
        if [ -f "../../build.sh" ]; then
            PROJECT_ROOT="$(cd ../.. && pwd)"
            print_debug "Adjusted project root to: $PROJECT_ROOT"
        else
            print_error "Cannot determine project root directory"
            print_error "Please run from project root or ensure build.sh exists"
            exit 1
        fi
    fi
    
    # Update paths based on actual project root
    BUILD_DIR="$PROJECT_ROOT/kirkstone/build"
    YOCTO_DEPLOY_DIR="$BUILD_DIR/tmp-glibc/deploy/images/intel-corei7-64"
    KEYS_DIR="$PROJECT_ROOT/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed"
    CA_CERT="$KEYS_DIR/ca.cert.pem"
    DEV_CERT="$KEYS_DIR/development-1.cert.pem"
    DEV_KEY="$KEYS_DIR/development-1.key.pem"
    
    print_debug "Project root: $PROJECT_ROOT"
    print_debug "Keys directory: $KEYS_DIR"
}

# Function to check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    # Check for RAUC tool
    if ! command_exists rauc; then
        print_error "RAUC is not installed. Please install RAUC tools first."
        print_error "On Ubuntu/Debian: sudo apt-get install rauc"
        print_error "On CentOS/RHEL: sudo yum install rauc"
        exit 1
    fi
    
    # Check for required certificates
    local missing_files=()
    
    if [ ! -f "$CA_CERT" ]; then
        missing_files+=("CA certificate: $CA_CERT")
    fi
    
    if [ ! -f "$DEV_CERT" ]; then
        missing_files+=("Development certificate: $DEV_CERT")
    fi
    
    if [ ! -f "$DEV_KEY" ]; then
        missing_files+=("Development key: $DEV_KEY")
    fi
    
    if [ ${#missing_files[@]} -gt 0 ]; then
        print_error "Missing required certificate files:"
        for file in "${missing_files[@]}"; do
            echo "  - $file"
        done
        exit 1
    fi
    
    print_success "All prerequisites met"
}

# Function to find Yocto-built ext4 image
find_yocto_ext4() {
    print_debug "Searching for Yocto-built ext4 images..."
    
    local expected_files=(
        "$YOCTO_DEPLOY_DIR/nuc-image-qt5-intel-corei7-64.ext4"
        "$YOCTO_DEPLOY_DIR/nuc-image-qt5-intel-corei7-64.rootfs.ext4"
        "$YOCTO_DEPLOY_DIR/nuc-image-intel-corei7-64.ext4"
        "$YOCTO_DEPLOY_DIR/nuc-image-intel-corei7-64.rootfs.ext4"
    )
    
    for file in "${expected_files[@]}"; do
        if [ -f "$file" ]; then
            echo "$file"
            return 0
        fi
    done
    
    # Search for any ext4 files in deploy directory
    if [ -d "$YOCTO_DEPLOY_DIR" ]; then
        local found_ext4=$(find "$YOCTO_DEPLOY_DIR" -name "*.ext4" -type f 2>/dev/null | head -1)
        if [ -n "$found_ext4" ]; then
            echo "$found_ext4"
            return 0
        fi
    fi
    
    return 1
}

# Function to build Yocto image
build_yocto_image() {
    print_status "Building Yocto image..."
    
    cd "$PROJECT_ROOT"
    
    if [ ! -f "build.sh" ]; then
        print_error "build.sh not found in project root"
        return 1
    fi
    
    # Try different build commands
    if ! ./build.sh auto; then
        print_warning "Auto build failed, trying default build..."
        if ! ./build.sh; then
            print_error "Yocto build failed"
            return 1
        fi
    fi
    
    print_success "Yocto build completed"
    return 0
}

# Function to show ext4 search help
show_ext4_help() {
    print_error "No ext4 image found!"
    echo ""
    print_status "Expected locations:"
    echo "  - $YOCTO_DEPLOY_DIR/nuc-image-qt5-intel-corei7-64.ext4"
    echo "  - $YOCTO_DEPLOY_DIR/nuc-image-qt5-intel-corei7-64.rootfs.ext4"
    echo ""
    print_status "Options:"
    echo "  1. Build Yocto image first:"
    echo "     cd $PROJECT_ROOT"
    echo "     ./build.sh"
    echo ""
    echo "  2. Use auto-build mode:"
    echo "     $0 --auto-build"
    echo ""
    echo "  3. Specify custom ext4 file:"
    echo "     $0 /path/to/custom-image.ext4"
}

# Function to create manifest
create_manifest() {
    local manifest_file="$1"
    local rootfs_file="$2"
    
    print_status "Creating manifest: $(basename "$manifest_file")"
    
    local build_timestamp=$(date '+%Y%m%d%H%M%S')
    
    print_debug "Creating simplified manifest (RAUC will auto-calculate SHA256 and size)"
    
    cat > "$manifest_file" << EOF
[update]
compatible=$BUNDLE_COMPATIBLE
version=$BUNDLE_VERSION
description=$BUNDLE_DESCRIPTION
build=$build_timestamp

[bundle]
format=plain

[image.rootfs]
filename=$(basename "$rootfs_file")
EOF
    
    print_success "Manifest created successfully"
}

# Function to build bundler tool if needed
ensure_bundler_tool() {
    print_debug "Checking bundler tool..."
    
    local bundler_exe="$BUNDLER_DIR/build/bundler"
    
    if [ ! -f "$bundler_exe" ]; then
        print_status "Building bundler tool..."
        
        cd "$BUNDLER_DIR"
        mkdir -p build
        cd build
        
        if ! cmake ..; then
            print_error "CMake configuration failed"
            exit 1
        fi
        
        if ! make; then
            print_error "Build failed"
            exit 1
        fi
        
        print_success "Bundler tool built successfully"
    else
        print_debug "Bundler tool already exists"
    fi
}

# Function to create bundle
create_bundle() {
    local bundle_dir="$1"
    local output_file="$2"
    
    print_status "Creating RAUC bundle: $(basename "$output_file")"
    
    # Create bundle using rauc command directly
    if ! rauc bundle \
        --cert="$DEV_CERT" \
        --key="$DEV_KEY" \
        "$bundle_dir" \
        "$output_file"; then
        print_error "RAUC bundle creation failed"
        exit 1
    fi
    
    print_success "Bundle created successfully"
}

# Function to verify bundle
verify_bundle() {
    local bundle_file="$1"
    
    if [ "$VERIFY_BUNDLE" != "true" ]; then
        print_debug "Bundle verification skipped"
        return 0
    fi
    
    print_status "Verifying bundle: $(basename "$bundle_file")"
    
    # Show bundle info
    echo ""
    print_header "Bundle Information"
    if ! rauc info "$bundle_file"; then
        print_warning "Failed to get bundle info"
    fi
    echo ""
    
    # Verify bundle signature
    print_header "Bundle Verification"
    if rauc verify --keyring="$CA_CERT" "$bundle_file"; then
        print_success "Bundle signature verification successful"
    else
        print_warning "Bundle signature verification failed (but bundle was created)"
    fi
}

# Function to extract bundle
extract_bundle() {
    local bundle_file="$1"
    
    if [ "$EXTRACT_BUNDLE" != "true" ]; then
        return 0
    fi
    
    local extract_dir="$OUTPUT_DIR/extracted-$(basename "$bundle_file" .raucb)"
    
    print_status "Extracting bundle to: $extract_dir"
    
    mkdir -p "$extract_dir"
    if rauc extract "$bundle_file" "$extract_dir"; then
        print_success "Bundle extracted successfully"
        echo ""
        print_status "Extracted contents:"
        ls -la "$extract_dir"
    else
        print_warning "Bundle extraction failed"
    fi
}

# Function to show deployment instructions
show_deployment_info() {
    local bundle_file="$1"
    local bundle_size=$(du -h "$bundle_file" | cut -f1)
    
    echo ""
    print_header "Deployment Information"
    echo "📦 Bundle file: $bundle_file"
    echo "📏 Bundle size: $bundle_size"
    echo "🔑 Signed with: $(basename "$DEV_CERT")"
    echo "🔒 Verified with: $(basename "$CA_CERT")"
    echo "📅 Created: $(date)"
    echo ""
    
    print_status "Deployment commands:"
    echo "  # Copy to target device"
    echo "  scp \"$bundle_file\" root@192.168.1.100:/tmp/"
    echo ""
    echo "  # Install on target"
    echo "  ssh root@192.168.1.100 'rauc install /tmp/$(basename "$bundle_file")'"
    echo ""
    echo "  # Check status"
    echo "  ssh root@192.168.1.100 'rauc status'"
    echo ""
    
    print_status "Alternative deployment:"
    echo "  # Using bundler deploy script"
    echo "  cd $BUNDLER_DIR"
    echo "  ./deploy-bundle.sh -i -s"
}

# Main function
main() {
    # Parse command line arguments
    local rootfs_file=""
    local show_help_flag=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help_flag=true
                shift
                ;;
            -v|--verbose)
                VERBOSE="true"
                shift
                ;;
            -a|--auto-build)
                AUTO_BUILD="true"
                shift
                ;;
            -e|--extract)
                EXTRACT_BUNDLE="true"
                shift
                ;;
            -n|--no-verify)
                VERIFY_BUNDLE="false"
                shift
                ;;
            -c|--no-cleanup)
                CLEANUP_TEMP="false"
                shift
                ;;
            --name)
                BUNDLE_NAME="$2"
                shift 2
                ;;
            --version)
                BUNDLE_VERSION="$2"
                shift 2
                ;;
            --compatible)
                BUNDLE_COMPATIBLE="$2"
                shift 2
                ;;
            --description)
                BUNDLE_DESCRIPTION="$2"
                shift 2
                ;;
            -*)
                print_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
            *)
                if [ -z "$rootfs_file" ]; then
                    rootfs_file="$1"
                else
                    print_error "Multiple ext4 files specified"
                    exit 1
                fi
                shift
                ;;
        esac
    done
    
    if [ "$show_help_flag" = "true" ]; then
        show_help
        exit 0
    fi
    
    print_header "Unified RAUC Bundle Creation Tool v$SCRIPT_VERSION"
    
    # Setup environment
    setup_environment
    check_prerequisites
    ensure_bundler_tool
    
    # Create output directories
    mkdir -p "$BUNDLE_TEMP_DIR" "$OUTPUT_DIR"
    
    # Determine ext4 file to use
    if [ -z "$rootfs_file" ]; then
        print_status "No ext4 file specified, searching for Yocto-built image..."
        
        if rootfs_file=$(find_yocto_ext4); then
            print_success "Found Yocto-built ext4 image: $rootfs_file"
        elif [ "$AUTO_BUILD" = "true" ]; then
            print_status "No ext4 image found, building with Yocto..."
            if build_yocto_image; then
                if rootfs_file=$(find_yocto_ext4); then
                    print_success "Built and found ext4 image: $rootfs_file"
                else
                    print_error "Build completed but no ext4 image found"
                    exit 1
                fi
            else
                print_error "Yocto build failed"
                exit 1
            fi
        else
            show_ext4_help
            exit 1
        fi
    else
        print_status "Using specified ext4 file: $rootfs_file"
    fi
    
    # Check if rootfs file exists
    if [ ! -f "$rootfs_file" ]; then
        print_error "Rootfs file not found: $rootfs_file"
        exit 1
    fi
    
    # Get file information
    local file_size=$(du -h "$rootfs_file" | cut -f1)
    print_status "Source ext4 image: $rootfs_file (Size: $file_size)"
    
    # Copy rootfs to bundle directory
    print_status "Preparing bundle contents..."
    cp "$rootfs_file" "$BUNDLE_TEMP_DIR/rootfs.ext4"
    
    # Create manifest
    create_manifest "$BUNDLE_TEMP_DIR/manifest.raucm" "$BUNDLE_TEMP_DIR/rootfs.ext4"
    
    # Generate output filename
    local timestamp=$(date +%Y%m%d%H%M%S)
    local output_file="$OUTPUT_DIR/${BUNDLE_NAME}-${BUNDLE_VERSION}-${timestamp}.raucb"
    
    # Create bundle
    create_bundle "$BUNDLE_TEMP_DIR" "$output_file"
    
    # Verify bundle
    verify_bundle "$output_file"
    
    # Extract bundle if requested
    extract_bundle "$output_file"
    
    # Copy bundle to project root for easy access
    local final_bundle="$PROJECT_ROOT/$(basename "$output_file")"
    cp "$output_file" "$final_bundle"
    print_success "Bundle copied to project root: $(basename "$final_bundle")"
    
    # Cleanup
    if [ "$CLEANUP_TEMP" = "true" ]; then
        print_debug "Cleaning up temporary files..."
        rm -rf "$BUNDLE_TEMP_DIR"
    fi
    
    # Show deployment information
    show_deployment_info "$final_bundle"
    
    print_success "🎉 RAUC bundle creation completed successfully!"
}

# Check if script is being sourced
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi