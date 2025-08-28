#!/bin/bash

# Test Python RAUC Bundler with Real Yocto ext4 Image
# This script specifically tests with the actual Yocto-built ext4 image

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NATIVE_BUNDLER="$SCRIPT_DIR/bundler_native.py"
BUILD_DIR="$SCRIPT_DIR/build"

# Real Yocto image path
YOCTO_IMAGE="/home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-intel-corei7-64.ext4"

# RAUC keys
KEYS_DIR="/home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed"
CERT_FILE="$KEYS_DIR/development-1.cert.pem"
KEY_FILE="$KEYS_DIR/development-1.key.pem"
CA_FILE="$KEYS_DIR/ca.cert.pem"

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

# Function to check if real image exists
check_real_image() {
    print_status "Checking for real Yocto ext4 image"
    
    if [ ! -f "$YOCTO_IMAGE" ]; then
        print_error "Real Yocto ext4 image not found: $YOCTO_IMAGE"
        print_error "Please build the Yocto image first:"
        print_error "  cd /home/makepluscode/docker-yocto-kirkstone-nuc"
        print_error "  ./build.sh"
        return 1
    fi
    
    local image_size=$(du -h "$YOCTO_IMAGE" | cut -f1)
    local image_bytes=$(stat -c%s "$YOCTO_IMAGE" 2>/dev/null || stat -f%z "$YOCTO_IMAGE" 2>/dev/null)
    
    print_success "Found real ext4 image: $image_size ($image_bytes bytes)"
    
    # Check if it's actually an ext4 filesystem
    if command -v file >/dev/null 2>&1; then
        local file_type=$(file "$YOCTO_IMAGE")
        print_status "File type: $file_type"
        
        if echo "$file_type" | grep -q "ext[2-4]"; then
            print_success "Confirmed: This is a real ext filesystem"
        else
            print_warning "Warning: File type doesn't clearly indicate ext filesystem"
        fi
    fi
    
    return 0
}

# Function to check keys
check_keys() {
    print_status "Checking RAUC signing keys"
    
    if [ ! -f "$CERT_FILE" ] || [ ! -f "$KEY_FILE" ] || [ ! -f "$CA_FILE" ]; then
        print_error "RAUC keys not found in: $KEYS_DIR"
        return 1
    fi
    
    print_success "RAUC keys found"
    print_status "Certificate: $(basename "$CERT_FILE")"
    print_status "Private key: $(basename "$KEY_FILE")"
    print_status "CA cert: $(basename "$CA_FILE")"
    
    return 0
}

# Function to create unsigned bundle with real image
test_unsigned_real_bundle() {
    print_status "Creating unsigned bundle with real Yocto ext4 image"
    
    mkdir -p "$BUILD_DIR"
    local output_bundle="$BUILD_DIR/real-unsigned.raucb"
    rm -f "$output_bundle"
    
    local start_time=$(date +%s.%N 2>/dev/null || date +%s)
    
    if python3 "$NATIVE_BUNDLER" --verbose "$YOCTO_IMAGE" "$output_bundle"; then
        local end_time=$(date +%s.%N 2>/dev/null || date +%s)
        
        if [ -f "$output_bundle" ]; then
            local bundle_size=$(stat -c%s "$output_bundle" 2>/dev/null || stat -f%z "$output_bundle" 2>/dev/null)
            local input_size=$(stat -c%s "$YOCTO_IMAGE" 2>/dev/null || stat -f%z "$YOCTO_IMAGE" 2>/dev/null)
            
            local duration
            if command -v bc >/dev/null 2>&1; then
                duration=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "N/A")
                local size_mb=$(echo "scale=2; $bundle_size / 1024 / 1024" | bc 2>/dev/null)
                local input_mb=$(echo "scale=2; $input_size / 1024 / 1024" | bc 2>/dev/null)
                local compression=$(echo "scale=1; $bundle_size * 100 / $input_size" | bc 2>/dev/null)
                
                print_success "Bundle created in ${duration}s"
                print_success "Input: ${input_mb}MB → Output: ${size_mb}MB (${compression}% compression)"
            else
                print_success "Bundle created successfully"
                print_success "Bundle size: $(du -h "$output_bundle" | cut -f1)"
            fi
            
            return 0
        fi
    fi
    
    print_error "Failed to create unsigned bundle"
    return 1
}

# Function to create signed bundle with real image
test_signed_real_bundle() {
    print_status "Creating signed bundle with real Yocto ext4 image"
    
    mkdir -p "$BUILD_DIR"
    local output_bundle="$BUILD_DIR/real-signed.raucb"
    rm -f "$output_bundle"
    
    local start_time=$(date +%s.%N 2>/dev/null || date +%s)
    
    if python3 "$NATIVE_BUNDLER" \
        --cert "$CERT_FILE" \
        --key "$KEY_FILE" \
        --verbose \
        "$YOCTO_IMAGE" \
        "$output_bundle"; then
        
        local end_time=$(date +%s.%N 2>/dev/null || date +%s)
        
        if [ -f "$output_bundle" ]; then
            local bundle_size=$(stat -c%s "$output_bundle" 2>/dev/null || stat -f%z "$output_bundle" 2>/dev/null)
            local input_size=$(stat -c%s "$YOCTO_IMAGE" 2>/dev/null || stat -f%z "$YOCTO_IMAGE" 2>/dev/null)
            
            local duration
            if command -v bc >/dev/null 2>&1; then
                duration=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "N/A")
                local size_mb=$(echo "scale=2; $bundle_size / 1024 / 1024" | bc 2>/dev/null)
                local input_mb=$(echo "scale=2; $input_size / 1024 / 1024" | bc 2>/dev/null)
                local compression=$(echo "scale=1; $bundle_size * 100 / $input_size" | bc 2>/dev/null)
                
                print_success "Signed bundle created in ${duration}s"
                print_success "Input: ${input_mb}MB → Output: ${size_mb}MB (${compression}% compression)"
            else
                print_success "Signed bundle created successfully"
                print_success "Bundle size: $(du -h "$output_bundle" | cut -f1)"
            fi
            
            # Verify bundle contains signature
            if tar -tf "$output_bundle" | grep -q manifest.sig; then
                print_success "Bundle contains digital signature ✓"
            else
                print_error "Bundle missing digital signature ✗"
                return 1
            fi
            
            return 0
        fi
    fi
    
    print_error "Failed to create signed bundle"
    return 1
}

# Function to extract and examine bundle
extract_and_examine() {
    print_status "Extracting and examining real bundle"
    
    local bundle_file="$BUILD_DIR/real-signed.raucb"
    local extract_dir="$BUILD_DIR/real-extracted"
    
    if [ ! -f "$bundle_file" ]; then
        print_warning "No signed bundle found for extraction"
        return 0
    fi
    
    rm -rf "$extract_dir"
    mkdir -p "$extract_dir"
    
    if tar -xf "$bundle_file" -C "$extract_dir"; then
        print_success "Bundle extracted successfully"
        
        # Show contents
        print_status "Extracted contents:"
        ls -la "$extract_dir"
        echo
        
        # Show manifest
        if [ -f "$extract_dir/manifest.raucm" ]; then
            print_status "=== MANIFEST CONTENT ==="
            cat "$extract_dir/manifest.raucm"
            echo
        fi
        
        # Examine ext4 image
        local ext4_file=$(find "$extract_dir" -name "*.ext4" -type f | head -1)
        if [ -f "$ext4_file" ]; then
            print_status "=== EXT4 IMAGE DETAILS ==="
            print_status "File: $(basename "$ext4_file")"
            print_status "Size: $(du -h "$ext4_file" | cut -f1)"
            
            if command -v file >/dev/null 2>&1; then
                print_status "Type: $(file "$ext4_file")"
            fi
            
            # Try to get ext4 filesystem info if tools are available
            if command -v dumpe2fs >/dev/null 2>&1; then
                print_status "Filesystem info:"
                dumpe2fs -h "$ext4_file" 2>/dev/null | head -10 || true
            elif command -v fsck.ext4 >/dev/null 2>&1; then
                print_status "Filesystem check:"
                fsck.ext4 -n "$ext4_file" 2>/dev/null | head -5 || true
            fi
            echo
        fi
        
        # Examine signature
        if [ -f "$extract_dir/manifest.sig" ]; then
            print_status "=== DIGITAL SIGNATURE ==="
            local sig_size=$(stat -c%s "$extract_dir/manifest.sig" 2>/dev/null || stat -f%z "$extract_dir/manifest.sig" 2>/dev/null)
            print_status "Signature file size: $sig_size bytes"
            
            if command -v openssl >/dev/null 2>&1; then
                print_status "Signature details:"
                openssl cms -inform DER -in "$extract_dir/manifest.sig" -noout -print 2>/dev/null | head -10 || true
            fi
        fi
        
        return 0
    else
        print_error "Bundle extraction failed"
        return 1
    fi
}

# Function to run all real image tests
run_all_real_tests() {
    print_status "=== TESTING PYTHON RAUC BUNDLER WITH REAL YOCTO EXT4 IMAGE ==="
    echo
    
    local tests_passed=0
    local tests_total=0
    
    local test_functions=(
        "check_real_image"
        "check_keys"
        "test_unsigned_real_bundle"
        "test_signed_real_bundle"
        "extract_and_examine"
    )
    
    for test_func in "${test_functions[@]}"; do
        ((tests_total++))
        echo
        print_status "=== Running: $test_func ==="
        
        if $test_func; then
            print_success "$test_func PASSED"
            ((tests_passed++))
        else
            print_error "$test_func FAILED"
        fi
    done
    
    # Final summary
    echo
    print_status "=== FINAL SUMMARY ==="
    print_status "Tests passed: $tests_passed/$tests_total"
    
    if [ $tests_passed -eq $tests_total ]; then
        print_success "✓ All tests passed! Python bundler works with real Yocto ext4 images."
    else
        print_error "✗ Some tests failed. Check the output above."
        return 1
    fi
    
    # Show final bundle info
    echo
    print_status "=== GENERATED BUNDLES ==="
    if [ -d "$BUILD_DIR" ]; then
        find "$BUILD_DIR" -name "*.raucb" -type f -exec ls -lh {} \;
    fi
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  all        - Run all real image tests (default)"
    echo "  check      - Check real image and keys availability"
    echo "  unsigned   - Test unsigned bundle creation"
    echo "  signed     - Test signed bundle creation"
    echo "  extract    - Extract and examine bundle"
    echo "  clean      - Clean generated bundles"
    echo "  help       - Show this help message"
    echo ""
    echo "Real Image Path:"
    echo "  $YOCTO_IMAGE"
}

# Function to clean generated bundles
clean_bundles() {
    print_status "Cleaning generated bundles"
    
    if [ -d "$BUILD_DIR" ]; then
        rm -f "$BUILD_DIR"/real-*.raucb
        rm -rf "$BUILD_DIR/real-extracted"
        print_success "Real image bundles cleaned"
    fi
}

# Main function
main() {
    local command="${1:-all}"
    
    case "$command" in
        "all")
            run_all_real_tests
            ;;
        "check")
            check_real_image && check_keys
            ;;
        "unsigned")
            check_real_image && test_unsigned_real_bundle
            ;;
        "signed")
            check_real_image && check_keys && test_signed_real_bundle
            ;;
        "extract")
            extract_and_examine
            ;;
        "clean")
            clean_bundles
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        *)
            print_error "Unknown command: $command"
            show_usage
            exit 1
            ;;
    esac
}

# Check if script is being sourced
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi