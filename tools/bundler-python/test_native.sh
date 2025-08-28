#!/bin/bash

# Native Python RAUC Bundler Test Script
# Tests the native implementation without external RAUC tools

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NATIVE_BUNDLER="$SCRIPT_DIR/bundler_native.py"
BUILD_DIR="$SCRIPT_DIR/build"
TEST_DIR="$SCRIPT_DIR/test"

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

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies for native bundler"
    
    # Check Python 3
    if ! command -v python3 >/dev/null 2>&1; then
        print_error "Python 3 is not installed"
        return 1
    fi
    
    python_version=$(python3 --version | cut -d' ' -f2)
    print_success "Python version: $python_version"
    
    # Check OpenSSL (for signing)
    if command -v openssl >/dev/null 2>&1; then
        openssl_version=$(openssl version | cut -d' ' -f2)
        print_success "OpenSSL version: $openssl_version"
    else
        print_warning "OpenSSL not found - signing will not work"
    fi
    
    return 0
}

# Function to create test files
create_test_files() {
    print_status "Creating test files"
    
    mkdir -p "$BUILD_DIR" "$TEST_DIR"
    
    # Check for real Yocto-built ext4 image first
    local yocto_image="/home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-intel-corei7-64.ext4"
    
    if [ -f "$yocto_image" ]; then
        print_status "Using real Yocto-built ext4 image: $yocto_image"
        local image_size=$(du -h "$yocto_image" | cut -f1)
        print_success "Found real ext4 image: $image_size"
        
        # Copy to test directory for easier access
        cp "$yocto_image" "$TEST_DIR/nuc-image-qt5.ext4"
        
        # Also create smaller test files for quick tests
        if [ ! -f "$TEST_DIR/tiny-rootfs.ext4" ]; then
            print_status "Creating tiny test file for quick tests"
            dd if=/dev/zero of="$TEST_DIR/tiny-rootfs.ext4" bs=1M count=1 2>/dev/null
        fi
        
        return 0
    else
        print_warning "Real Yocto ext4 image not found at: $yocto_image"
        print_status "Creating dummy test files instead"
        
        # Create test rootfs files of different sizes
        local test_files=(
            "test/tiny-rootfs.ext4:1"
            "test/small-rootfs.ext4:5"
            "test/medium-rootfs.ext4:10"
        )
        
        for file_spec in "${test_files[@]}"; do
            local file_path="${file_spec%:*}"
            local size_mb="${file_spec#*:}"
            
            if [ ! -f "$file_path" ]; then
                print_status "Creating $file_path (${size_mb}MB)"
                dd if=/dev/zero of="$file_path" bs=1M count="$size_mb" 2>/dev/null
            fi
        done
        
        print_success "Test files created"
        return 1
    fi
}

# Function to run syntax tests
test_syntax() {
    print_status "Testing Python syntax"
    
    if python3 -m py_compile "$NATIVE_BUNDLER"; then
        print_success "Syntax check passed"
        return 0
    else
        print_error "Syntax check failed"
        return 1
    fi
}

# Function to run basic functionality tests
test_basic_functionality() {
    print_status "Testing basic functionality"
    
    local tests_passed=0
    local tests_total=0
    
    # Test help output
    ((tests_total++))
    if python3 "$NATIVE_BUNDLER" --help >/dev/null 2>&1; then
        print_success "Help output test passed"
        ((tests_passed++))
    else
        print_error "Help output test failed"
    fi
    
    # Test version output
    ((tests_total++))
    if python3 "$NATIVE_BUNDLER" --version >/dev/null 2>&1; then
        print_success "Version output test passed"
        ((tests_passed++))
    else
        print_error "Version output test failed"
    fi
    
    # Test argument validation
    ((tests_total++))
    if ! python3 "$NATIVE_BUNDLER" 2>/dev/null; then
        print_success "Argument validation test passed"
        ((tests_passed++))
    else
        print_error "Argument validation test failed"
    fi
    
    print_status "Basic functionality tests: $tests_passed/$tests_total passed"
    return $((tests_total - tests_passed))
}

# Function to test bundle creation without signing
test_unsigned_bundle() {
    print_status "Testing unsigned bundle creation"
    
    local test_rootfs="$TEST_DIR/tiny-rootfs.ext4"
    local output_bundle="$BUILD_DIR/test-unsigned.raucb"
    
    # Remove existing bundle
    rm -f "$output_bundle"
    
    # Create bundle
    if python3 "$NATIVE_BUNDLER" --verbose "$test_rootfs" "$output_bundle"; then
        # Verify bundle was created
        if [ -f "$output_bundle" ]; then
            local bundle_size=$(stat -c%s "$output_bundle" 2>/dev/null || stat -f%z "$output_bundle" 2>/dev/null)
            print_success "Unsigned bundle created: $output_bundle ($bundle_size bytes)"
            
            # Verify bundle content
            if tar -tf "$output_bundle" | grep -q manifest.raucm; then
                print_success "Bundle contains manifest.raucm"
            else
                print_error "Bundle missing manifest.raucm"
                return 1
            fi
            
            if tar -tf "$output_bundle" | grep -q tiny-rootfs.ext4; then
                print_success "Bundle contains rootfs file"
            else
                print_error "Bundle missing rootfs file"
                return 1
            fi
            
            return 0
        else
            print_error "Bundle file was not created"
            return 1
        fi
    else
        print_error "Bundle creation failed"
        return 1
    fi
}

# Function to test bundle creation with signing
test_signed_bundle() {
    print_status "Testing signed bundle creation"
    
    local keys_dir="/home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed"
    local cert_file="$keys_dir/development-1.cert.pem"
    local key_file="$keys_dir/development-1.key.pem"
    
    # Check if keys exist
    if [ ! -f "$cert_file" ] || [ ! -f "$key_file" ]; then
        print_warning "RAUC keys not found, skipping signed bundle test"
        return 0
    fi
    
    # Use real Yocto image if available, otherwise use test file
    local test_rootfs
    if [ -f "$TEST_DIR/nuc-image-qt5.ext4" ]; then
        test_rootfs="$TEST_DIR/nuc-image-qt5.ext4"
        print_status "Using real Yocto ext4 image for signed bundle test"
    else
        test_rootfs="$TEST_DIR/small-rootfs.ext4"
        print_status "Using dummy test file for signed bundle test"
    fi
    
    local output_bundle="$BUILD_DIR/test-signed-real.raucb"
    
    # Remove existing bundle
    rm -f "$output_bundle"
    
    # Create signed bundle
    if python3 "$NATIVE_BUNDLER" \
        --cert "$cert_file" \
        --key "$key_file" \
        --verbose \
        "$test_rootfs" \
        "$output_bundle"; then
        
        # Verify bundle was created
        if [ -f "$output_bundle" ]; then
            local bundle_size=$(stat -c%s "$output_bundle" 2>/dev/null || stat -f%z "$output_bundle" 2>/dev/null)
            local size_mb=$(echo "scale=2; $bundle_size / 1024 / 1024" | bc 2>/dev/null || echo "N/A")
            print_success "Signed bundle created: $output_bundle (${size_mb}MB)"
            
            # Verify bundle contains signature
            if tar -tf "$output_bundle" | grep -q manifest.sig; then
                print_success "Bundle contains digital signature"
            else
                print_error "Bundle missing digital signature"
                return 1
            fi
            
            # Show bundle contents
            print_status "Bundle contents:"
            tar -tf "$output_bundle"
            
            return 0
        else
            print_error "Signed bundle file was not created"
            return 1
        fi
    else
        print_error "Signed bundle creation failed"
        return 1
    fi
}

# Function to test bundle extraction and verification
test_bundle_extraction() {
    print_status "Testing bundle extraction and verification"
    
    local test_bundle="$BUILD_DIR/test-signed-real.raucb"
    local extract_dir="$BUILD_DIR/extracted"
    
    if [ ! -f "$test_bundle" ]; then
        print_warning "No test bundle found for extraction test"
        return 0
    fi
    
    # Clean extraction directory
    rm -rf "$extract_dir"
    mkdir -p "$extract_dir"
    
    # Extract bundle
    if tar -xf "$test_bundle" -C "$extract_dir"; then
        print_success "Bundle extracted successfully"
        
        # List extracted contents
        print_status "Extracted contents:"
        ls -la "$extract_dir"
        
        # Verify manifest content
        if [ -f "$extract_dir/manifest.raucm" ]; then
            print_status "Manifest content:"
            cat "$extract_dir/manifest.raucm"
            echo ""
        fi
        
        # Check if we have the real ext4 image
        local ext4_file=$(find "$extract_dir" -name "*.ext4" -type f | head -1)
        if [ -f "$ext4_file" ]; then
            local ext4_size=$(du -h "$ext4_file" | cut -f1)
            print_status "Extracted ext4 image: $(basename "$ext4_file") ($ext4_size)"
            
            # Verify ext4 filesystem if possible
            if command -v file >/dev/null 2>&1; then
                local file_type=$(file "$ext4_file")
                print_status "File type: $file_type"
            fi
        fi
        
        return 0
    else
        print_error "Bundle extraction failed"
        return 1
    fi
}

# Function to run performance test
test_performance() {
    print_status "Testing performance with larger file"
    
    # Use real Yocto image if available for performance test
    local test_rootfs
    if [ -f "$TEST_DIR/nuc-image-qt5.ext4" ]; then
        test_rootfs="$TEST_DIR/nuc-image-qt5.ext4"
        print_status "Using real Yocto ext4 image for performance test"
    else
        test_rootfs="$TEST_DIR/medium-rootfs.ext4"
        print_status "Using dummy test file for performance test"
    fi
    
    local output_bundle="$BUILD_DIR/test-performance.raucb"
    
    # Remove existing bundle
    rm -f "$output_bundle"
    
    # Show input file size
    if [ -f "$test_rootfs" ]; then
        local input_size=$(du -h "$test_rootfs" | cut -f1)
        print_status "Input file size: $input_size"
    fi
    
    # Time the bundle creation
    local start_time=$(date +%s.%N 2>/dev/null || date +%s)
    
    if python3 "$NATIVE_BUNDLER" "$test_rootfs" "$output_bundle" >/dev/null 2>&1; then
        local end_time=$(date +%s.%N 2>/dev/null || date +%s)
        local duration
        if command -v bc >/dev/null 2>&1; then
            duration=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "N/A")
        else
            duration=$(expr $end_time - $start_time 2>/dev/null || echo "N/A")
        fi
        
        if [ -f "$output_bundle" ]; then
            local bundle_size=$(stat -c%s "$output_bundle" 2>/dev/null || stat -f%z "$output_bundle" 2>/dev/null)
            local size_mb
            if command -v bc >/dev/null 2>&1; then
                size_mb=$(echo "scale=2; $bundle_size / 1024 / 1024" | bc 2>/dev/null || echo "N/A")
            else
                size_mb=$(expr $bundle_size / 1024 / 1024 2>/dev/null || echo "N/A")
            fi
            
            print_success "Performance test completed in ${duration}s"
            print_success "Output bundle size: ${size_mb}MB"
            
            # Calculate compression ratio if possible
            if [ -f "$test_rootfs" ]; then
                local input_bytes=$(stat -c%s "$test_rootfs" 2>/dev/null || stat -f%z "$test_rootfs" 2>/dev/null)
                if command -v bc >/dev/null 2>&1 && [ "$input_bytes" -gt 0 ]; then
                    local compression_ratio=$(echo "scale=2; $bundle_size * 100 / $input_bytes" | bc 2>/dev/null || echo "N/A")
                    print_status "Compression ratio: ${compression_ratio}%"
                fi
            fi
            
            return 0
        fi
    fi
    
    print_error "Performance test failed"
    return 1
}

# Function to run all tests
run_all_tests() {
    print_status "Running comprehensive native bundler tests"
    
    local tests_passed=0
    local tests_total=0
    
    # Individual test functions
    local test_functions=(
        "check_dependencies"
        "create_test_files"
        "test_syntax"
        "test_basic_functionality"
        "test_unsigned_bundle"
        "test_signed_bundle"
        "test_bundle_extraction"
        "test_performance"
    )
    
    for test_func in "${test_functions[@]}"; do
        ((tests_total++))
        echo ""
        print_status "Running: $test_func"
        
        if $test_func; then
            print_success "$test_func completed successfully"
            ((tests_passed++))
        else
            print_error "$test_func failed"
        fi
    done
    
    # Final summary
    echo ""
    print_status "=== Test Summary ==="
    print_status "Tests passed: $tests_passed/$tests_total"
    
    if [ $tests_passed -eq $tests_total ]; then
        print_success "All tests passed! Native bundler is working correctly."
        return 0
    else
        print_error "Some tests failed. Check the output above for details."
        return 1
    fi
}

# Function to clean test artifacts
clean_test_artifacts() {
    print_status "Cleaning test artifacts"
    
    rm -rf "$BUILD_DIR"
    rm -rf "$TEST_DIR"
    
    print_success "Test artifacts cleaned"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  all        - Run all tests (default)"
    echo "  deps       - Check dependencies"
    echo "  syntax     - Test Python syntax"
    echo "  basic      - Run basic functionality tests"
    echo "  unsigned   - Test unsigned bundle creation"
    echo "  signed     - Test signed bundle creation"
    echo "  extract    - Test bundle extraction"
    echo "  perf       - Run performance test"
    echo "  clean      - Clean test artifacts"
    echo "  help       - Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 all"
    echo "  $0 signed"
    echo "  $0 clean"
}

# Main function
main() {
    local command="${1:-all}"
    
    case "$command" in
        "all")
            run_all_tests
            ;;
        "deps")
            check_dependencies
            ;;
        "syntax")
            test_syntax
            ;;
        "basic")
            create_test_files && test_basic_functionality
            ;;
        "unsigned")
            create_test_files && test_unsigned_bundle
            ;;
        "signed")
            create_test_files && test_signed_bundle
            ;;
        "extract")
            test_bundle_extraction
            ;;
        "perf")
            create_test_files && test_performance
            ;;
        "clean")
            clean_test_artifacts
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