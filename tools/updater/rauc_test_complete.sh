#!/bin/bash

# RAUC Complete Test Script
# Comprehensive automated testing for RAUC updates from start to end
# Usage: ./rauc_test_complete.sh [options]

set -euo pipefail

# Script metadata
SCRIPT_VERSION="1.0.0"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Default configuration
DEFAULT_BUNDLE="$SCRIPT_DIR/image/nuc-image-qt5-bundle-intel-corei7-64.raucb"
DEFAULT_HOST="192.168.1.100"
DEFAULT_USER="root"
DEFAULT_PASSWORD="root"
DEFAULT_PORT="22"

# Test configuration
TEST_SCENARIOS=("basic" "double")
TEST_DEVICES=("192.168.1.100")
REBOOT_TIMEOUT=300
CONNECTION_RETRY_INTERVAL=15
LOG_DIR="$SCRIPT_DIR/logs"
RESULTS_DIR="$LOG_DIR/results_$(date +%Y%m%d_%H%M%S)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# Global variables
BUNDLE_FILE="$DEFAULT_BUNDLE"
TARGET_HOST="$DEFAULT_HOST"
TARGET_USER="$DEFAULT_USER"
TARGET_PASSWORD="$DEFAULT_PASSWORD"
TARGET_PORT="$DEFAULT_PORT"
TEST_MODE="single"
TEST_SCENARIO="basic"
VERBOSE=false
NETWORK_SETUP=false
CLEANUP_AFTER=true
LOG_FILE=""
INITIAL_SLOT=""
TARGET_SLOT=""
TEST_START_TIME=0

# Create necessary directories
mkdir -p "$LOG_DIR" "$RESULTS_DIR"

# Logging functions
setup_logging() {
    if [[ "$TEST_MODE" == "single" ]]; then
        LOG_FILE="$LOG_DIR/test_$(date +%Y%m%d_%H%M%S).log"
    else
        LOG_FILE="$LOG_DIR/batch_$(date +%Y%m%d_%H%M%S).log"
    fi
}

log() {
    local level="$1"
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${timestamp} [${level}] ${message}" | tee -a "$LOG_FILE"
}

log_info() { log "INFO" "${CYAN}$*${NC}"; }
log_success() { log "SUCCESS" "${GREEN}✅ $*${NC}"; }
log_error() { log "ERROR" "${RED}✗ $*${NC}"; }
log_warning() { log "WARNING" "${YELLOW}⚠ $*${NC}"; }
log_debug() { [[ "$VERBOSE" == "true" ]] && log "DEBUG" "${BLUE}🔍 $*${NC}"; }

# Progress indicator
show_progress() {
    local current="$1"
    local total="$2"
    local description="$3"
    local percent=$((current * 100 / total))
    local filled=$((current * 50 / total))
    local empty=$((50 - filled))
    
    printf "\r${BOLD}Progress:${NC} ["
    printf "%${filled}s" | tr ' ' '█'
    printf "%${empty}s" | tr ' ' '░'
    printf "] %d%% - %s" "$percent" "$description"
    
    if [[ $current -eq $total ]]; then
        echo ""
    fi
}

# Error handler
error_handler() {
    local line_number="$1"
    local exit_code="$2"
    log_error "Test failed at line $line_number with exit code $exit_code"
    log_error "Check log file: $LOG_FILE"
    
    if [[ "$TEST_MODE" == "batch" ]]; then
        generate_batch_report "FAILED"
    fi
    
    exit "$exit_code"
}

trap 'error_handler $LINENO $?' ERR

# Utility functions
check_command() {
    local cmd="$1"
    if ! command -v "$cmd" &> /dev/null; then
        log_error "Required command not found: $cmd"
        return 1
    fi
    log_debug "Command available: $cmd"
    return 0
}

clean_ssh_host_keys() {
    local host="$1"
    ssh-keygen -f "$HOME/.ssh/known_hosts" -R "$host" 2>/dev/null || true
    log_debug "SSH host keys cleaned for $host"
}

wait_with_spinner() {
    local pid="$1"
    local message="$2"
    local spinner='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
    local i=0
    
    while kill -0 "$pid" 2>/dev/null; do
        printf "\r${CYAN}%s${NC} %c" "$message" "${spinner:i++%${#spinner}:1}"
        sleep 0.1
    done
    printf "\r${GREEN}✅ %s${NC}\n" "$message"
}

# Core test functions
check_prerequisites() {
    log_info "=== Step 0: Prerequisites Check ==="
    show_progress 1 7 "Checking prerequisites"
    
    # Check working directory
    if [[ ! -d "src" || ! -f "pyproject.toml" ]]; then
        log_error "Please run this script from the tools/updater directory"
        return 1
    fi
    log_debug "Working directory validated"
    
    # Check bundle file
    if [[ ! -f "$BUNDLE_FILE" ]]; then
        log_error "Bundle file not found: $BUNDLE_FILE"
        return 1
    fi
    
    local bundle_size=$(du -h "$BUNDLE_FILE" | cut -f1)
    log_success "Bundle file found: $(basename "$BUNDLE_FILE") ($bundle_size)"
    
    # Check required tools
    local required_tools=("uv" "ssh" "scp" "sshpass" "ping")
    for tool in "${required_tools[@]}"; do
        if ! check_command "$tool"; then
            return 1
        fi
    done
    
    # Check Python environment
    if ! uv run python --version &>/dev/null; then
        log_error "Python environment not ready. Run 'uv sync' first."
        return 1
    fi
    
    log_success "All prerequisites met"
    return 0
}

get_initial_state() {
    log_info "=== Step 1: Get Initial System State ==="
    show_progress 2 7 "Getting system state"
    
    clean_ssh_host_keys "$TARGET_HOST"
    
    # Test basic connectivity
    if ! ping -c 2 -W 3 "$TARGET_HOST" &>/dev/null; then
        log_error "Target host $TARGET_HOST is not reachable"
        return 1
    fi
    log_debug "Host $TARGET_HOST is reachable"
    
    # Get RAUC status
    local rauc_status
    if ! rauc_status=$(ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10 \
        "$TARGET_USER@$TARGET_HOST" "rauc status" 2>/dev/null); then
        log_error "Could not get RAUC status from $TARGET_HOST"
        return 1
    fi
    
    # Parse current slot
    if echo "$rauc_status" | grep -q "rootfs.0.*booted"; then
        INITIAL_SLOT="A"
        TARGET_SLOT="B"
    elif echo "$rauc_status" | grep -q "rootfs.1.*booted"; then
        INITIAL_SLOT="B" 
        TARGET_SLOT="A"
    else
        log_error "Could not determine current boot slot"
        echo "$rauc_status"
        return 1
    fi
    
    # Get additional system info
    local uptime=$(ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" "uptime" 2>/dev/null)
    local df_info=$(ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" "df -h /" 2>/dev/null)
    
    log_success "System state captured:"
    log_info "  • Current slot: $INITIAL_SLOT ($(echo "$rauc_status" | grep booted | cut -d'[' -f2 | cut -d']' -f1))"
    log_info "  • Target slot: $TARGET_SLOT"
    log_info "  • System uptime: $(echo "$uptime" | sed 's/.*up //' | sed 's/,.*user.*//')"
    log_info "  • Disk usage: $(echo "$df_info" | tail -1 | awk '{print $5 " used (" $4 " free)"}')"
    
    return 0
}

setup_ssh_keys() {
    log_info "=== Step 2: Setup SSH Authentication ==="
    show_progress 3 7 "Setting up SSH keys"
    
    log_debug "Setting up passwordless SSH authentication for $TARGET_USER@$TARGET_HOST"
    
    # First try the CLI tool method
    if uv run rauc-updater setup-ssh \
        --host "$TARGET_HOST" \
        --user "$TARGET_USER" \
        --port "$TARGET_PORT" \
        --password "$TARGET_PASSWORD" 2>/dev/null; then
        log_success "SSH key setup completed (via CLI tool)"
        return 0
    fi
    
    # If CLI tool fails, try manual SSH key setup
    log_warning "CLI tool SSH setup failed, trying manual method..."
    
    # Check if SSH connection already works
    if timeout 5 ssh -o StrictHostKeyChecking=no -o ConnectTimeout=3 \
        "$TARGET_USER@$TARGET_HOST" "echo 'test'" &>/dev/null; then
        log_success "SSH key authentication already working"
        return 0
    fi
    
    # Try manual SSH key copying with sshpass
    log_debug "Attempting manual SSH key copy with password..."
    if command -v sshpass &>/dev/null; then
        if sshpass -p "$TARGET_PASSWORD" ssh-copy-id -o StrictHostKeyChecking=no \
            -p "$TARGET_PORT" "$TARGET_USER@$TARGET_HOST" &>/dev/null; then
            log_success "SSH key setup completed (manual method)"
            return 0
        elif sshpass -p "$TARGET_PASSWORD" ssh-copy-id -f -o StrictHostKeyChecking=no \
            -p "$TARGET_PORT" "$TARGET_USER@$TARGET_HOST" &>/dev/null; then
            log_success "SSH key setup completed (manual method, forced)"
            return 0
        fi
    else
        log_warning "sshpass not available for manual SSH key setup"
    fi
    
    # Final check - maybe keys are already there but CLI detection failed
    if timeout 5 ssh -o StrictHostKeyChecking=no -o ConnectTimeout=3 \
        "$TARGET_USER@$TARGET_HOST" "echo 'test'" &>/dev/null; then
        log_success "SSH authentication working despite setup warnings"
        return 0
    fi
    
    log_error "All SSH key setup methods failed"
    return 1
}

test_connection() {
    log_info "=== Step 3: Test Connection and RAUC ==="
    show_progress 4 7 "Testing connection"
    
    # First try CLI tool method
    if uv run python -m src.cli test \
        --host "$TARGET_HOST" \
        --user "$TARGET_USER" \
        --port "$TARGET_PORT" 2>/dev/null; then
        log_success "Connection and RAUC test passed (CLI tool)"
        return 0
    fi
    
    # If CLI tool fails, try manual connection test
    log_warning "CLI tool connection test failed, trying manual method..."
    
    # Test basic SSH connectivity
    if ! timeout 10 ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 \
        "$TARGET_USER@$TARGET_HOST" "echo 'SSH test successful'" &>/dev/null; then
        log_error "Basic SSH connection failed"
        return 1
    fi
    log_debug "Basic SSH connection successful"
    
    # Test RAUC availability
    local rauc_version
    if rauc_version=$(ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" \
        "rauc --version" 2>/dev/null); then
        local version_line=$(echo "$rauc_version" | head -1)
        log_success "RAUC found: $version_line"
    else
        log_error "RAUC not found on target system"
        return 1
    fi
    
    # Test RAUC service status
    if ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" \
        "systemctl is-active rauc.service" &>/dev/null; then
        log_debug "RAUC service is active"
    else
        log_debug "RAUC service status unknown (may be normal)"
    fi
    
    # Check disk space for bundle
    local disk_info
    if disk_info=$(ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" \
        "df -h /tmp" 2>/dev/null); then
        local available=$(echo "$disk_info" | tail -1 | awk '{print $4}')
        log_debug "Available space in /tmp: $available"
    fi
    
    log_success "Connection and RAUC test passed (manual method)"
    return 0
}

perform_update() {
    log_info "=== Step 4: Perform RAUC Update ==="
    show_progress 5 7 "Performing update"
    
    local bundle_name=$(basename "$BUNDLE_FILE")
    log_info "Installing bundle: $bundle_name"
    log_info "Slot transition: $INITIAL_SLOT → $TARGET_SLOT"
    
    local start_time=$(date +%s)
    local update_args=(
        "$BUNDLE_FILE"
        --host "$TARGET_HOST"
        --user "$TARGET_USER"
        --port "$TARGET_PORT"
    )
    
    if [[ "$VERBOSE" == "true" ]]; then
        update_args+=(--verbose)
    fi
    
    # First try CLI tool method
    if uv run python -m src.cli update "${update_args[@]}" 2>/dev/null; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        log_success "RAUC update completed in ${duration} seconds (CLI tool)"
        return 0
    fi
    
    # If CLI tool fails, try manual method
    log_warning "CLI tool update failed, trying manual method..."
    
    local remote_path="/tmp/$bundle_name"
    
    # Upload bundle
    log_info "Uploading $bundle_name to $TARGET_HOST:$remote_path..."
    if ! time scp -o StrictHostKeyChecking=no -P "$TARGET_PORT" \
        "$BUNDLE_FILE" "$TARGET_USER@$TARGET_HOST:$remote_path"; then
        log_error "Bundle upload failed"
        return 1
    fi
    log_success "Bundle upload completed"
    
    # Install bundle
    log_info "Installing RAUC bundle on target device..."
    local install_start=$(date +%s)
    
    if ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" \
        "rauc install $remote_path"; then
        local install_end=$(date +%s)
        local install_duration=$((install_end - install_start))
        log_success "RAUC installation completed in ${install_duration} seconds"
    else
        log_error "RAUC installation failed"
        return 1
    fi
    
    # Clean up bundle if requested
    if [[ "$CLEANUP_AFTER" == "true" ]]; then
        if ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" \
            "rm -f $remote_path" 2>/dev/null; then
            log_debug "Remote bundle file cleaned up"
        else
            log_warning "Failed to clean up remote bundle file"
        fi
    fi
    
    local end_time=$(date +%s)
    local total_duration=$((end_time - start_time))
    log_success "RAUC update completed in ${total_duration} seconds (manual method)"
    return 0
}

wait_for_reboot() {
    log_info "=== Step 5: Wait for System Reboot ==="
    show_progress 6 7 "Waiting for reboot"
    
    # Initiate reboot
    log_debug "Initiating system reboot..."
    ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" "reboot" 2>/dev/null || true
    
    # Wait for system to go down
    sleep 10
    
    # Monitor reboot process
    local start_time=$(date +%s)
    local attempt=1
    
    log_info "Monitoring system reboot (timeout: ${REBOOT_TIMEOUT}s)..."
    
    while true; do
        local current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        
        if [[ $elapsed -gt $REBOOT_TIMEOUT ]]; then
            log_error "Reboot timeout exceeded (${REBOOT_TIMEOUT}s)"
            return 1
        fi
        
        clean_ssh_host_keys "$TARGET_HOST"
        
        if timeout 5 ssh -o StrictHostKeyChecking=no -o ConnectTimeout=3 \
            "$TARGET_USER@$TARGET_HOST" "echo 'Online'" &>/dev/null; then
            log_success "System back online after ${elapsed} seconds"
            return 0
        fi
        
        if [[ $((attempt % 3)) -eq 0 ]]; then
            log_debug "Attempt $attempt: Waiting for system... (${elapsed}s elapsed)"
        fi
        
        sleep "$CONNECTION_RETRY_INTERVAL"
        ((attempt++))
    done
}

verify_update() {
    log_info "=== Step 6: Verify Update Results ==="
    show_progress 7 7 "Verifying update"
    
    # Give system time to stabilize
    sleep 5
    
    # Get post-update status
    local rauc_status=$(ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" "rauc status")
    local uptime=$(ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_HOST" "uptime")
    
    # Verify slot switch
    local current_slot=""
    if echo "$rauc_status" | grep -q "rootfs.0.*booted"; then
        current_slot="A"
    elif echo "$rauc_status" | grep -q "rootfs.1.*booted"; then
        current_slot="B"
    else
        log_error "Could not determine current boot slot after update"
        return 1
    fi
    
    # Verify slot switch occurred
    if [[ "$current_slot" == "$TARGET_SLOT" ]]; then
        log_success "Slot switch verified: $INITIAL_SLOT → $current_slot ✅"
    else
        log_error "Slot switch failed: Expected $TARGET_SLOT, got $current_slot"
        return 1
    fi
    
    # Show system info
    log_info "Post-update system state:"
    log_info "  • Active slot: $current_slot"
    log_info "  • Boot time: $(echo "$uptime" | sed 's/.*up //' | sed 's/,.*user.*//')"
    
    if [[ "$VERBOSE" == "true" ]]; then
        log_debug "Detailed RAUC status:"
        echo "$rauc_status" | while read -r line; do
            log_debug "  $line"
        done
    fi
    
    return 0
}

cleanup_test() {
    if [[ "$CLEANUP_AFTER" == "true" ]]; then
        log_info "=== Step 7: Cleanup ==="
        log_debug "Cleaning up temporary files..."
        
        if uv run rauc-updater cleanup \
            --host "$TARGET_HOST" \
            --user "$TARGET_USER" \
            --port "$TARGET_PORT" 2>/dev/null; then
            log_success "Cleanup completed"
        else
            log_warning "Cleanup had issues (non-critical)"
        fi
    else
        log_debug "Cleanup skipped (disabled)"
    fi
}

# Single test execution
run_single_test() {
    local test_name="$TEST_SCENARIO"
    
    echo "=================================================="
    echo "     RAUC Complete Test - $test_name Test"
    echo "=================================================="
    echo "Bundle: $(basename "$BUNDLE_FILE")"
    echo "Target: $TARGET_USER@$TARGET_HOST:$TARGET_PORT"
    echo "Mode: Single test"
    echo "Start: $(date)"
    echo "=================================================="
    
    if ! check_prerequisites; then return 1; fi
    if ! get_initial_state; then return 1; fi
    if ! setup_ssh_keys; then return 1; fi
    if ! test_connection; then return 1; fi
    if ! perform_update; then return 1; fi
    if ! wait_for_reboot; then return 1; fi
    if ! verify_update; then return 1; fi
    cleanup_test
    
    return 0
}

# Double test (two consecutive updates)
run_double_test() {
    log_info "=== Running Double Update Test ==="
    log_info "This will perform two consecutive updates to return to original slot"
    
    local original_slot="$INITIAL_SLOT"
    
    # First update
    log_info "--- First Update: $INITIAL_SLOT → $TARGET_SLOT ---"
    if ! run_single_test; then
        return 1
    fi
    
    # Prepare for second update (swap slots)
    local temp_slot="$INITIAL_SLOT"
    INITIAL_SLOT="$TARGET_SLOT"
    TARGET_SLOT="$temp_slot"
    
    # Wait between updates
    log_info "Waiting 30 seconds between updates..."
    sleep 30
    
    # Second update
    log_info "--- Second Update: $INITIAL_SLOT → $TARGET_SLOT ---"
    if ! get_initial_state; then return 1; fi
    if ! perform_update; then return 1; fi
    if ! wait_for_reboot; then return 1; fi
    if ! verify_update; then return 1; fi
    cleanup_test
    
    if [[ "$TARGET_SLOT" == "$original_slot" ]]; then
        log_success "Double update completed: Returned to original slot ($original_slot)"
    else
        log_error "Double update failed: Expected return to $original_slot"
        return 1
    fi
    
    return 0
}

# Batch test execution
run_batch_test() {
    local total_tests=0
    local passed_tests=0
    
    echo "=================================================="
    echo "         RAUC Complete Test - Batch Mode"
    echo "=================================================="
    echo "Scenarios: ${TEST_SCENARIOS[*]}"
    echo "Devices: ${TEST_DEVICES[*]}"
    echo "Results: $RESULTS_DIR"
    echo "=================================================="
    
    for device in "${TEST_DEVICES[@]}"; do
        for scenario in "${TEST_SCENARIOS[@]}"; do
            ((total_tests++))
            TARGET_HOST="$device"
            TEST_SCENARIO="$scenario"
            
            local test_name="${scenario}_${device//\./_}"
            local result_file="$RESULTS_DIR/${test_name}.log"
            LOG_FILE="$result_file"
            
            log_info "Starting $scenario test on $device ($passed_tests/$total_tests)"
            
            case $scenario in
                basic)
                    if run_single_test > "$result_file" 2>&1; then
                        ((passed_tests++))
                        log_success "✅ $scenario test passed on $device"
                    else
                        log_error "❌ $scenario test failed on $device"
                    fi
                    ;;
                double)
                    if run_double_test > "$result_file" 2>&1; then
                        ((passed_tests++))
                        log_success "✅ $scenario test passed on $device"
                    else
                        log_error "❌ $scenario test failed on $device"
                    fi
                    ;;
                *)
                    log_error "Unknown scenario: $scenario"
                    ;;
            esac
            
            sleep 5  # Brief pause between tests
        done
    done
    
    generate_batch_report "$passed_tests" "$total_tests"
    
    if [[ $passed_tests -eq $total_tests ]]; then
        log_success "🎯 ALL BATCH TESTS PASSED! ($passed_tests/$total_tests)"
        return 0
    else
        log_error "❌ SOME TESTS FAILED ($passed_tests/$total_tests)"
        return 1
    fi
}

# Report generation
generate_batch_report() {
    local passed="$1"
    local total="$2"
    local report_file="$RESULTS_DIR/test_report.html"
    local test_end_time=$(date +%s)
    local total_duration=$((test_end_time - TEST_START_TIME))
    
    cat > "$report_file" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>RAUC Complete Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 8px; margin-bottom: 30px; }
        .header h1 { margin: 0; font-size: 2.5em; }
        .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-bottom: 30px; }
        .stat { background-color: #f8f9fa; padding: 20px; border-radius: 8px; text-align: center; border-left: 4px solid #667eea; }
        .stat h3 { margin: 0 0 10px 0; color: #333; }
        .stat .value { font-size: 2em; font-weight: bold; color: #667eea; }
        .success { color: #28a745; border-left-color: #28a745; }
        .error { color: #dc3545; border-left-color: #dc3545; }
        .test-result { margin: 15px 0; padding: 20px; border-radius: 8px; border-left: 4px solid #ccc; }
        .test-result.success { border-left-color: #28a745; background-color: #d4edda; }
        .test-result.error { border-left-color: #dc3545; background-color: #f8d7da; }
        .test-result h3 { margin: 0 0 15px 0; }
        details { margin-top: 10px; }
        summary { cursor: pointer; padding: 10px; background-color: rgba(0,0,0,0.1); border-radius: 4px; }
        pre { background-color: #f8f9fa; padding: 15px; border-radius: 4px; overflow-x: auto; margin-top: 10px; }
        .config { background-color: #e9ecef; padding: 20px; border-radius: 8px; margin-bottom: 30px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚀 RAUC Complete Test Report</h1>
            <p><strong>Generated:</strong> $(date)</p>
            <p><strong>Version:</strong> $SCRIPT_VERSION</p>
        </div>
        
        <div class="stats">
            <div class="stat">
                <h3>Total Duration</h3>
                <div class="value">$((total_duration / 60))m $((total_duration % 60))s</div>
            </div>
            <div class="stat">
                <h3>Total Tests</h3>
                <div class="value">$total</div>
            </div>
            <div class="stat success">
                <h3>Passed Tests</h3>
                <div class="value">$passed</div>
            </div>
            <div class="stat error">
                <h3>Failed Tests</h3>
                <div class="value">$((total - passed))</div>
            </div>
        </div>
        
        <div class="config">
            <h2>📋 Test Configuration</h2>
            <pre>Bundle: $(basename "$BUNDLE_FILE")
Devices: ${TEST_DEVICES[*]}
Scenarios: ${TEST_SCENARIOS[*]}
Results Directory: $RESULTS_DIR</pre>
        </div>
        
        <h2>📊 Test Results</h2>
EOF

    # Add individual test results
    for result_file in "$RESULTS_DIR"/*.log; do
        if [[ -f "$result_file" && "$result_file" != "$report_file" ]]; then
            local test_name=$(basename "$result_file" .log)
            local status="error"
            local status_icon="❌"
            
            if grep -q "Test COMPLETED" "$result_file" 2>/dev/null; then
                status="success"
                status_icon="✅"
            fi
            
            cat >> "$report_file" << EOF
        <div class="test-result $status">
            <h3>$status_icon $test_name</h3>
            <details>
                <summary>View Test Log</summary>
                <pre>$(tail -30 "$result_file" 2>/dev/null || echo "Log file empty or unreadable")</pre>
            </details>
        </div>
EOF
        fi
    done
    
    cat >> "$report_file" << EOF
        <div style="margin-top: 50px; text-align: center; color: #666; border-top: 1px solid #ddd; padding-top: 20px;">
            <p>Generated by RAUC Complete Test Script v$SCRIPT_VERSION</p>
            <p>$(date)</p>
        </div>
    </div>
</body>
</html>
EOF

    log_success "HTML report generated: $report_file"
}

generate_single_summary() {
    local test_end_time=$(date +%s)
    local total_duration=$((test_end_time - TEST_START_TIME))
    local minutes=$((total_duration / 60))
    local seconds=$((total_duration % 60))
    
    echo ""
    echo "=================================================="
    echo "           🎯 RAUC TEST COMPLETED!"
    echo "=================================================="
    log_success "Test Results:"
    log_success "  • Bundle: $(basename "$BUNDLE_FILE")"
    log_success "  • Target: $TARGET_USER@$TARGET_HOST"
    log_success "  • Scenario: $TEST_SCENARIO"
    log_success "  • Slot transition: $INITIAL_SLOT → $TARGET_SLOT"
    log_success "  • Total time: ${minutes}m ${seconds}s"
    log_success "  • Log file: $LOG_FILE"
    echo "=================================================="
}

# Network setup function
setup_network() {
    log_info "=== Network Setup ==="
    
    local connect_script="$SCRIPT_DIR/../../connect.sh"
    if [[ -f "$connect_script" ]]; then
        log_info "Running network setup script..."
        if "$connect_script" > "$LOG_DIR/network_setup.log" 2>&1; then
            log_success "Network setup completed"
        else
            log_warning "Network setup had issues (check $LOG_DIR/network_setup.log)"
        fi
    else
        log_warning "Network setup script not found: $connect_script"
    fi
    
    # Test connectivity
    for device in "${TEST_DEVICES[@]}"; do
        if ping -c 2 -W 3 "$device" &>/dev/null; then
            log_success "Device $device is reachable"
        else
            log_error "Device $device is NOT reachable"
        fi
    done
}

# Help function
show_help() {
    cat << EOF
RAUC Complete Test Script v$SCRIPT_VERSION

DESCRIPTION:
    Comprehensive automated testing for RAUC updates from start to end.
    Supports single device testing and batch testing across multiple devices.

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -h, --help              Show this help message
    -b, --bundle PATH       RAUC bundle file path (default: $DEFAULT_BUNDLE)
    -H, --host HOST         Target device IP address (default: $DEFAULT_HOST)
    -u, --user USER         SSH username (default: $DEFAULT_USER)
    -p, --port PORT         SSH port (default: $DEFAULT_PORT)
    -P, --password PASS     SSH password (default: $DEFAULT_PASSWORD)
    -s, --scenario NAME     Test scenario: basic|double (default: basic)
    -m, --mode MODE         Test mode: single|batch (default: single)
    -v, --verbose           Enable verbose output
    -n, --network-setup     Run network setup before testing
    -c, --no-cleanup        Don't cleanup after tests
    -t, --timeout SECONDS   Reboot timeout in seconds (default: $REBOOT_TIMEOUT)

TEST SCENARIOS:
    basic       Single RAUC update with A/B slot switching
    double      Two consecutive updates returning to original slot

EXAMPLES:
    # Basic single test (default)
    $0

    # Verbose test with custom bundle
    $0 -v -b /path/to/bundle.raucb

    # Double update test
    $0 -s double

    # Custom target device
    $0 -H 192.168.1.150 -P mypassword

    # Batch testing (requires configuration)
    $0 -m batch

    # Network setup + basic test
    $0 -n -v

ENVIRONMENT VARIABLES:
    TARGET_HOST         Override default host
    TARGET_USER         Override default user  
    TARGET_PASSWORD     Override default password
    REBOOT_TIMEOUT      Override reboot timeout
    RAUC_TEST_VERBOSE   Enable verbose mode

FILES:
    Bundle: $DEFAULT_BUNDLE
    Logs: $LOG_DIR/
    Results: $LOG_DIR/results_*/

REQUIREMENTS:
    - Python environment with uv
    - SSH access to target device
    - RAUC installed on target
    - sshpass for automatic SSH key setup

For more information, see docs/TESTING.md
EOF
}

# Command line argument parsing
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -b|--bundle)
                BUNDLE_FILE="$2"
                shift 2
                ;;
            -H|--host)
                TARGET_HOST="$2"
                shift 2
                ;;
            -u|--user)
                TARGET_USER="$2"
                shift 2
                ;;
            -p|--port)
                TARGET_PORT="$2"
                shift 2
                ;;
            -P|--password)
                TARGET_PASSWORD="$2"
                shift 2
                ;;
            -s|--scenario)
                TEST_SCENARIO="$2"
                shift 2
                ;;
            -m|--mode)
                TEST_MODE="$2"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -n|--network-setup)
                NETWORK_SETUP=true
                shift
                ;;
            -c|--no-cleanup)
                CLEANUP_AFTER=false
                shift
                ;;
            -t|--timeout)
                REBOOT_TIMEOUT="$2"
                shift 2
                ;;
            *)
                log_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
}

# Environment variable overrides
apply_env_overrides() {
    TARGET_HOST="${TARGET_HOST:-$DEFAULT_HOST}"
    TARGET_USER="${TARGET_USER:-$DEFAULT_USER}"
    TARGET_PASSWORD="${TARGET_PASSWORD:-$DEFAULT_PASSWORD}"
    REBOOT_TIMEOUT="${REBOOT_TIMEOUT:-$REBOOT_TIMEOUT}"
    VERBOSE="${RAUC_TEST_VERBOSE:-$VERBOSE}"
    
    # Update device list if single mode
    if [[ "$TEST_MODE" == "single" ]]; then
        TEST_DEVICES=("$TARGET_HOST")
    fi
    
    # Update scenario list
    if [[ "$TEST_SCENARIO" != "basic" ]]; then
        TEST_SCENARIOS=("$TEST_SCENARIO")
    fi
}

# Main execution function
main() {
    TEST_START_TIME=$(date +%s)
    
    parse_arguments "$@"
    apply_env_overrides
    setup_logging
    
    # Network setup if requested
    if [[ "$NETWORK_SETUP" == "true" ]]; then
        setup_network
    fi
    
    # Execute test based on mode
    case $TEST_MODE in
        single)
            if [[ "$TEST_SCENARIO" == "double" ]]; then
                if run_double_test; then
                    generate_single_summary
                    exit 0
                else
                    exit 1
                fi
            else
                if run_single_test; then
                    generate_single_summary
                    exit 0
                else
                    exit 1
                fi
            fi
            ;;
        batch)
            if run_batch_test; then
                exit 0
            else
                exit 1
            fi
            ;;
        *)
            log_error "Unknown test mode: $TEST_MODE"
            exit 1
            ;;
    esac
}

# Script execution
main "$@"