#!/bin/bash

# RAUC Test Configuration
# Source this file to set up test environment variables

# Default test configuration
export RAUC_TEST_BUNDLE_DIR="../../image"
export RAUC_TEST_BUNDLE_NAME="nuc-image-qt5-bundle-intel-corei7-64.raucb"
export RAUC_TEST_HOST="192.168.1.100"
export RAUC_TEST_USER="root"
export RAUC_TEST_PASSWORD="root"
export RAUC_TEST_PORT="22"

# Test timeouts (in seconds)
export RAUC_TEST_CONNECTION_TIMEOUT="10"
export RAUC_TEST_REBOOT_TIMEOUT="300"
export RAUC_TEST_RETRY_INTERVAL="15"

# Test behavior
export RAUC_TEST_CLEANUP_AFTER="true"
export RAUC_TEST_MARK_GOOD="false"
export RAUC_TEST_IGNORE_COMPATIBLE="false"
export RAUC_TEST_VERBOSE="true"

# Logging
export RAUC_TEST_LOG_DIR="./logs"
export RAUC_TEST_LOG_LEVEL="INFO"

# Multiple device testing
export RAUC_TEST_DEVICES=(
    "192.168.1.100"
    # Add more devices here for batch testing
    # "192.168.1.101"
    # "192.168.1.102"
)

# Test scenarios
export RAUC_TEST_SCENARIOS=(
    "basic"          # Basic A→B or B→A update
    "double"         # Double update A→B→A or B→A→B
    # "stress"       # Multiple rapid updates (future)
    # "recovery"     # Recovery scenarios (future)
)

# Network configuration for connect.sh integration
export RAUC_TEST_HOST_INTERFACE="enp42s0"
export RAUC_TEST_HOST_IP="192.168.1.101"
export RAUC_TEST_NETWORK_MASK="255.255.255.0"

# Function to display current configuration
show_test_config() {
    echo "=================================================="
    echo "           RAUC Test Configuration"
    echo "=================================================="
    echo "Bundle: $RAUC_TEST_BUNDLE_DIR/$RAUC_TEST_BUNDLE_NAME"
    echo "Target: $RAUC_TEST_USER@$RAUC_TEST_HOST:$RAUC_TEST_PORT"
    echo "Password: $RAUC_TEST_PASSWORD"
    echo "Timeouts: Connection=${RAUC_TEST_CONNECTION_TIMEOUT}s, Reboot=${RAUC_TEST_REBOOT_TIMEOUT}s"
    echo "Options: Cleanup=$RAUC_TEST_CLEANUP_AFTER, Verbose=$RAUC_TEST_VERBOSE"
    echo "Log Dir: $RAUC_TEST_LOG_DIR"
    echo "Devices: ${RAUC_TEST_DEVICES[*]}"
    echo "Scenarios: ${RAUC_TEST_SCENARIOS[*]}"
    echo "=================================================="
}

# Function to validate configuration
validate_test_config() {
    local errors=0
    
    # Check bundle file exists
    local bundle_path="$RAUC_TEST_BUNDLE_DIR/$RAUC_TEST_BUNDLE_NAME"
    if [[ ! -f "$bundle_path" ]]; then
        echo "ERROR: Bundle file not found: $bundle_path"
        ((errors++))
    fi
    
    # Check required tools
    for tool in uv ssh scp sshpass; do
        if ! command -v "$tool" &> /dev/null; then
            echo "ERROR: Required tool not found: $tool"
            ((errors++))
        fi
    done
    
    # Create log directory
    mkdir -p "$RAUC_TEST_LOG_DIR"
    
    if [[ $errors -eq 0 ]]; then
        echo "Configuration validation: PASSED"
        return 0
    else
        echo "Configuration validation: FAILED ($errors errors)"
        return 1
    fi
}

# Auto-validation when sourced
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    # Script is being executed directly
    show_test_config
    validate_test_config
else
    # Script is being sourced
    echo "RAUC test configuration loaded"
fi