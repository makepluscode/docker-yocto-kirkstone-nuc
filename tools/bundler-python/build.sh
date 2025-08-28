#!/bin/bash

# Python RAUC Bundler Build Script
# Provides build, test, and deployment functionality for the Python bundler

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUNDLER_SCRIPT="$SCRIPT_DIR/bundler.py"
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

# Function to check Python version
check_python() {
    print_status "Checking Python version"
    
    if ! command -v python3 >/dev/null 2>&1; then
        print_error "Python 3 is not installed"
        return 1
    fi
    
    local python_version=$(python3 --version | cut -d' ' -f2)
    print_success "Python version: $python_version"
    
    # Check if Python version is 3.6+
    local major=$(echo $python_version | cut -d'.' -f1)
    local minor=$(echo $python_version | cut -d'.' -f2)
    
    if [ "$major" -lt 3 ] || ([ "$major" -eq 3 ] && [ "$minor" -lt 6 ]); then
        print_error "Python 3.6 or higher is required"
        return 1
    fi
    
    return 0
}

# Function to check RAUC availability
check_rauc() {
    print_status "Checking RAUC availability"
    
    if command -v rauc >/dev/null 2>&1; then
        local rauc_version=$(rauc --version 2>/dev/null | head -1 || echo "Unknown")
        print_success "RAUC is available: $rauc_version"
        return 0
    else
        print_warning "RAUC is not installed - required for actual bundle creation"
        print_warning "Install with: sudo apt-get install rauc (Ubuntu/Debian)"
        return 1
    fi
}

# Function to run syntax check
check_syntax() {
    print_status "Checking Python syntax"
    
    if python3 -m py_compile "$BUNDLER_SCRIPT"; then
        print_success "Python syntax check passed"
        return 0
    else
        print_error "Python syntax check failed"
        return 1
    fi
}

# Function to run basic tests
run_tests() {
    print_status "Running basic functionality tests"
    
    # Test help output
    if python3 "$BUNDLER_SCRIPT" --help >/dev/null 2>&1; then
        print_success "Help output test passed"
    else
        print_error "Help output test failed"
        return 1
    fi
    
    # Test version output
    if python3 "$BUNDLER_SCRIPT" --version >/dev/null 2>&1; then
        print_success "Version output test passed"
    else
        print_error "Version output test failed"
        return 1
    fi
    
    # Test argument validation
    if ! python3 "$BUNDLER_SCRIPT" 2>/dev/null; then
        print_success "Argument validation test passed"
    else
        print_error "Argument validation test failed"
        return 1
    fi
    
    return 0
}

# Function to create test files
create_test_files() {
    print_status "Creating test files"
    
    mkdir -p "$TEST_DIR"
    
    # Create a small test rootfs file
    local test_rootfs="$TEST_DIR/test-rootfs.ext4"
    if [ ! -f "$test_rootfs" ]; then
        print_status "Creating test rootfs file"
        dd if=/dev/zero of="$test_rootfs" bs=1M count=1 2>/dev/null
        print_success "Test rootfs created: $test_rootfs"
    fi
    
    # Check if RAUC keys exist
    local keys_dir="/home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed"
    if [ -f "$keys_dir/development-1.cert.pem" ] && [ -f "$keys_dir/development-1.key.pem" ]; then
        print_success "RAUC keys found at $keys_dir"
        return 0
    else
        print_warning "RAUC keys not found at $keys_dir"
        print_warning "Bundle creation test will skip signing"
        return 1
    fi
}

# Function to run integration test
run_integration_test() {
    print_status "Running integration test"
    
    if ! check_rauc; then
        print_warning "Skipping integration test - RAUC not available"
        return 0
    fi
    
    if ! create_test_files; then
        print_warning "Skipping integration test - test files creation failed"
        return 0
    fi
    
    local test_rootfs="$TEST_DIR/test-rootfs.ext4"
    local test_output="$BUILD_DIR/test-bundle.raucb"
    local keys_dir="/home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed"
    
    mkdir -p "$BUILD_DIR"
    
    # Remove existing test bundle
    rm -f "$test_output"
    
    # Test bundle creation with signing
    if [ -f "$keys_dir/development-1.cert.pem" ] && [ -f "$keys_dir/development-1.key.pem" ]; then
        print_status "Testing bundle creation with signing"
        
        if python3 "$BUNDLER_SCRIPT" \
            --cert "$keys_dir/development-1.cert.pem" \
            --key "$keys_dir/development-1.key.pem" \
            --verbose \
            "$test_rootfs" \
            "$test_output"; then
            print_success "Bundle creation with signing test passed"
            
            # Verify bundle was created
            if [ -f "$test_output" ]; then
                local bundle_size=$(du -h "$test_output" | cut -f1)
                print_success "Bundle created successfully: $test_output ($bundle_size)"
                return 0
            else
                print_error "Bundle file was not created"
                return 1
            fi
        else
            print_error "Bundle creation with signing test failed"
            return 1
        fi
    else
        # Test bundle creation without signing
        print_status "Testing bundle creation without signing"
        
        if python3 "$BUNDLER_SCRIPT" \
            --verbose \
            "$test_rootfs" \
            "$test_output"; then
            print_success "Bundle creation without signing test passed"
            return 0
        else
            print_error "Bundle creation without signing test failed"
            return 1
        fi
    fi
}

# Function to install bundler
install_bundler() {
    print_status "Installing Python RAUC bundler"
    
    local install_dir="/usr/local/bin"
    local install_script="rauc-bundler-python"
    
    if [ ! -w "$install_dir" ]; then
        print_error "Cannot write to $install_dir. Run with sudo or choose different location."
        return 1
    fi
    
    cp "$BUNDLER_SCRIPT" "$install_dir/$install_script"
    chmod +x "$install_dir/$install_script"
    
    print_success "Bundler installed as $install_script"
    print_success "Usage: $install_script <rootfs.ext4> <output.raucb>"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  check      - Check dependencies and environment"
    echo "  test       - Run basic functionality tests"
    echo "  build      - Build and run all tests"
    echo "  integration - Run integration test with actual bundle creation"
    echo "  install    - Install bundler to system PATH"
    echo "  clean      - Clean build artifacts"
    echo "  help       - Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 check"
    echo "  $0 build"
    echo "  $0 test"
    echo "  $0 integration"
}

# Function to clean build artifacts
clean_build() {
    print_status "Cleaning build artifacts"
    
    rm -rf "$BUILD_DIR"
    rm -rf "$TEST_DIR"
    
    # Clean Python cache
    find "$SCRIPT_DIR" -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
    find "$SCRIPT_DIR" -name "*.pyc" -delete 2>/dev/null || true
    
    print_success "Build artifacts cleaned"
}

# Main function
main() {
    local command="${1:-build}"
    
    case "$command" in
        "check")
            print_status "Checking dependencies and environment"
            check_python && check_rauc && check_syntax
            ;;
        "test")
            print_status "Running basic tests"
            check_python && check_syntax && run_tests
            ;;
        "build")
            print_status "Building and testing Python RAUC bundler"
            check_python && check_syntax && run_tests
            ;;
        "integration")
            print_status "Running integration tests"
            check_python && check_syntax && run_integration_test
            ;;
        "install")
            install_bundler
            ;;
        "clean")
            clean_build
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