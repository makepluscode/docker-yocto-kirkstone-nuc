#!/bin/bash

# RAUC Batch Testing Script
# Runs multiple test scenarios automatically

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/test_config.sh"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

BATCH_LOG_FILE="$RAUC_TEST_LOG_DIR/batch_test_$(date +%Y%m%d_%H%M%S).log"
RESULTS_DIR="$RAUC_TEST_LOG_DIR/results_$(date +%Y%m%d_%H%M%S)"

# Create results directory
mkdir -p "$RESULTS_DIR"

log_batch() {
    local level="$1"
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${timestamp} [${level}] ${message}" | tee -a "$BATCH_LOG_FILE"
}

log_info() { log_batch "INFO" "${CYAN}$*${NC}"; }
log_success() { log_batch "SUCCESS" "${GREEN}✅ $*${NC}"; }
log_error() { log_batch "ERROR" "${RED}✗ $*${NC}"; }
log_warning() { log_batch "WARNING" "${YELLOW}⚠ $*${NC}"; }

run_basic_test() {
    local device="$1"
    local test_name="basic_${device//\./_}"
    local result_file="$RESULTS_DIR/${test_name}.log"
    
    log_info "Running basic test on $device..."
    
    if "$SCRIPT_DIR/test_rauc_auto.sh" \
        "$RAUC_TEST_BUNDLE_DIR/$RAUC_TEST_BUNDLE_NAME" \
        "$device" > "$result_file" 2>&1; then
        log_success "Basic test passed on $device"
        return 0
    else
        log_error "Basic test failed on $device (see $result_file)"
        return 1
    fi
}

run_double_test() {
    local device="$1"
    local test_name="double_${device//\./_}"
    local result_file="$RESULTS_DIR/${test_name}.log"
    
    log_info "Running double update test on $device..."
    
    {
        echo "=== First Update ==="
        "$SCRIPT_DIR/test_rauc_auto.sh" \
            "$RAUC_TEST_BUNDLE_DIR/$RAUC_TEST_BUNDLE_NAME" \
            "$device"
        
        echo ""
        echo "=== Second Update (Return) ==="
        "$SCRIPT_DIR/test_rauc_auto.sh" \
            "$RAUC_TEST_BUNDLE_DIR/$RAUC_TEST_BUNDLE_NAME" \
            "$device"
    } > "$result_file" 2>&1
    
    if [[ $? -eq 0 ]]; then
        log_success "Double test passed on $device"
        return 0
    else
        log_error "Double test failed on $device (see $result_file)"
        return 1
    fi
}

run_network_setup() {
    log_info "Setting up network configuration..."
    
    # Check if connect.sh exists and run it
    local connect_script="$SCRIPT_DIR/../../connect.sh"
    if [[ -f "$connect_script" ]]; then
        log_info "Running network setup script..."
        if "$connect_script" > "$RESULTS_DIR/network_setup.log" 2>&1; then
            log_success "Network setup completed"
        else
            log_warning "Network setup had issues (continuing anyway)"
        fi
    else
        log_warning "connect.sh not found, skipping network setup"
    fi
    
    # Test basic connectivity
    for device in "${RAUC_TEST_DEVICES[@]}"; do
        if ping -c 2 -W 3 "$device" &>/dev/null; then
            log_success "Device $device is reachable"
        else
            log_error "Device $device is NOT reachable"
        fi
    done
}

generate_batch_report() {
    local report_file="$RESULTS_DIR/test_report.html"
    local batch_end_time=$(date +%s)
    local total_duration=$((batch_end_time - BATCH_START_TIME))
    
    cat > "$report_file" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>RAUC Batch Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .success { color: green; }
        .error { color: red; }
        .warning { color: orange; }
        .test-result { margin: 10px 0; padding: 10px; border-left: 4px solid #ccc; }
        .test-result.success { border-left-color: green; background-color: #f0fff0; }
        .test-result.error { border-left-color: red; background-color: #fff0f0; }
        pre { background-color: #f5f5f5; padding: 10px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="header">
        <h1>RAUC Batch Test Report</h1>
        <p><strong>Generated:</strong> $(date)</p>
        <p><strong>Duration:</strong> $((total_duration / 60))m $((total_duration % 60))s</p>
        <p><strong>Results Directory:</strong> $RESULTS_DIR</p>
    </div>
    
    <h2>Test Configuration</h2>
    <pre>
Bundle: $RAUC_TEST_BUNDLE_DIR/$RAUC_TEST_BUNDLE_NAME
Devices: ${RAUC_TEST_DEVICES[*]}
Scenarios: ${RAUC_TEST_SCENARIOS[*]}
    </pre>
    
    <h2>Test Results</h2>
EOF

    # Add results for each test
    for result_file in "$RESULTS_DIR"/*.log; do
        if [[ -f "$result_file" ]]; then
            local test_name=$(basename "$result_file" .log)
            local status="success"
            
            if grep -q "SUCCESSFUL" "$result_file"; then
                status="success"
            else
                status="error"
            fi
            
            cat >> "$report_file" << EOF
    <div class="test-result $status">
        <h3>$test_name</h3>
        <details>
            <summary>View Details</summary>
            <pre>$(tail -20 "$result_file")</pre>
        </details>
    </div>
EOF
        fi
    done
    
    cat >> "$report_file" << EOF
</body>
</html>
EOF

    log_success "Test report generated: $report_file"
}

show_batch_help() {
    echo "RAUC Batch Testing Script"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help           Show this help"
    echo "  -s, --scenario NAME  Run specific scenario (basic|double|all)"
    echo "  -d, --device IP      Test specific device only"
    echo "  --network-setup      Run network setup first"
    echo "  --no-cleanup         Don't cleanup after tests"
    echo ""
    echo "Examples:"
    echo "  $0                           # Run all scenarios on all devices"
    echo "  $0 -s basic                  # Run basic tests only"
    echo "  $0 -d 192.168.1.100          # Test single device"
    echo "  $0 --network-setup -s double # Setup network then run double tests"
    echo ""
}

main() {
    BATCH_START_TIME=$(date +%s)
    
    local run_scenarios=("${RAUC_TEST_SCENARIOS[@]}")
    local run_devices=("${RAUC_TEST_DEVICES[@]}")
    local network_setup=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_batch_help
                exit 0
                ;;
            -s|--scenario)
                run_scenarios=("$2")
                shift 2
                ;;
            -d|--device)
                run_devices=("$2")
                shift 2
                ;;
            --network-setup)
                network_setup=true
                shift
                ;;
            --no-cleanup)
                export RAUC_TEST_CLEANUP_AFTER="false"
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    echo "=================================================="
    echo "         RAUC Batch Testing Script"
    echo "=================================================="
    echo "Scenarios: ${run_scenarios[*]}"
    echo "Devices: ${run_devices[*]}"
    echo "Results: $RESULTS_DIR"
    echo "=================================================="
    
    # Validate configuration
    if ! validate_test_config; then
        log_error "Configuration validation failed"
        exit 1
    fi
    
    # Setup network if requested
    if [[ "$network_setup" == "true" ]]; then
        run_network_setup
    fi
    
    local total_tests=0
    local passed_tests=0
    
    # Run tests for each device and scenario combination
    for device in "${run_devices[@]}"; do
        for scenario in "${run_scenarios[@]}"; do
            ((total_tests++))
            
            log_info "Starting $scenario test on $device ($total_tests)"
            
            case $scenario in
                basic)
                    if run_basic_test "$device"; then
                        ((passed_tests++))
                    fi
                    ;;
                double)
                    if run_double_test "$device"; then
                        ((passed_tests++))
                    fi
                    ;;
                *)
                    log_error "Unknown scenario: $scenario"
                    ;;
            esac
            
            # Small delay between tests
            sleep 5
        done
    done
    
    # Generate report
    generate_batch_report
    
    # Summary
    log_info "=================================================="
    log_info "           BATCH TEST SUMMARY"
    log_info "=================================================="
    log_info "Total Tests: $total_tests"
    log_success "Passed: $passed_tests"
    
    if [[ $((total_tests - passed_tests)) -gt 0 ]]; then
        log_error "Failed: $((total_tests - passed_tests))"
    fi
    
    local success_rate=$((passed_tests * 100 / total_tests))
    log_info "Success Rate: $success_rate%"
    log_info "Results Directory: $RESULTS_DIR"
    log_info "=================================================="
    
    if [[ $passed_tests -eq $total_tests ]]; then
        log_success "🎯 ALL TESTS PASSED!"
        exit 0
    else
        log_error "❌ SOME TESTS FAILED"
        exit 1
    fi
}

# Create logs directory
mkdir -p "$RAUC_TEST_LOG_DIR"

# Run main function
main "$@"