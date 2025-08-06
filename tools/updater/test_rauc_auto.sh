#!/bin/bash

# RAUC Auto End-to-End Test Script
# This script performs automated testing of the complete RAUC update process
# Usage: ./test_rauc_auto.sh [bundle_file] [target_host]

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_BUNDLE="${SCRIPT_DIR}/../../image/nuc-image-qt5-bundle-intel-corei7-64.raucb"
DEFAULT_HOST="192.168.1.100"
DEFAULT_USER="root"
DEFAULT_PASSWORD="root"
LOG_FILE="${SCRIPT_DIR}/test_results_$(date +%Y%m%d_%H%M%S).log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test configuration
BUNDLE_FILE="${1:-$DEFAULT_BUNDLE}"
TARGET_HOST="${2:-$DEFAULT_HOST}"
REBOOT_TIMEOUT=300  # 5 minutes max wait for reboot
CONNECTION_RETRY_INTERVAL=15  # seconds

# Logging function
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

# Error handler
error_handler() {
    local line_number="$1"
    log_error "Test failed at line $line_number"
    log_error "Check log file: $LOG_FILE"
    exit 1
}

trap 'error_handler $LINENO' ERR

# Test functions
check_prerequisites() {
    log_info "=== Step 0: Prerequisites Check ==="
    
    # Check if we're in the right directory
    if [[ ! -d "src" || ! -f "pyproject.toml" ]]; then
        log_error "Please run this script from the tools/updater directory"
        exit 1
    fi
    
    # Check bundle file exists
    if [[ ! -f "$BUNDLE_FILE" ]]; then
        log_error "Bundle file not found: $BUNDLE_FILE"
        exit 1
    fi
    
    local bundle_size=$(du -h "$BUNDLE_FILE" | cut -f1)
    log_success "Bundle file found: $BUNDLE_FILE ($bundle_size)"
    
    # Check required tools
    for tool in uv ssh scp sshpass; do
        if ! command -v "$tool" &> /dev/null; then
            log_error "Required tool not found: $tool"
            exit 1
        fi
    done
    
    log_success "All prerequisites met"
}

get_initial_state() {
    log_info "=== Step 1: Get Initial System State ==="
    
    # Clean SSH host keys to avoid conflicts
    ssh-keygen -f "$HOME/.ssh/known_hosts" -R "$TARGET_HOST" 2>/dev/null || true
    
    # Get current slot information
    local rauc_status=$(ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10 "$DEFAULT_USER@$TARGET_HOST" "rauc status" 2>/dev/null)
    
    # Parse current booted slot
    if echo "$rauc_status" | grep -q "rootfs.0.*booted"; then
        INITIAL_SLOT="A"
        TARGET_SLOT="B"
        log_info "Currently booted from A slot (rootfs.0)"
        log_info "Will update to B slot (rootfs.1)"
    elif echo "$rauc_status" | grep -q "rootfs.1.*booted"; then
        INITIAL_SLOT="B" 
        TARGET_SLOT="A"
        log_info "Currently booted from B slot (rootfs.1)"
        log_info "Will update to A slot (rootfs.0)"
    else
        log_error "Could not determine current boot slot"
        log_error "RAUC status output:"
        echo "$rauc_status"
        exit 1
    fi
    
    # Get system info
    local uptime=$(ssh -o StrictHostKeyChecking=no "$DEFAULT_USER@$TARGET_HOST" "uptime")
    local df_info=$(ssh -o StrictHostKeyChecking=no "$DEFAULT_USER@$TARGET_HOST" "df -h /")
    
    log_success "Initial state captured:"
    log_info "  • Current slot: $INITIAL_SLOT"
    log_info "  • Target slot: $TARGET_SLOT"  
    log_info "  • Uptime: $uptime"
    log_info "  • Disk usage: $(echo "$df_info" | tail -1)"
}

setup_ssh_keys() {
    log_info "=== Step 2: Setup SSH Keys ==="
    
    log_info "Setting up passwordless SSH authentication..."
    if uv run rauc-updater setup-ssh --host "$TARGET_HOST" --password "$DEFAULT_PASSWORD"; then
        log_success "SSH key setup completed"
    else
        log_error "SSH key setup failed"
        exit 1
    fi
}

test_connection() {
    log_info "=== Step 3: Test Connection ==="
    
    log_info "Testing connection to target device..."
    if uv run rauc-updater test --host "$TARGET_HOST"; then
        log_success "Connection test passed"
    else
        log_error "Connection test failed"
        exit 1
    fi
}

perform_update() {
    log_info "=== Step 4: Perform RAUC Update ==="
    
    log_info "Starting RAUC update with bundle: $(basename "$BUNDLE_FILE")"
    local start_time=$(date +%s)
    
    if uv run rauc-updater update "$BUNDLE_FILE" --host "$TARGET_HOST" --verbose; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        log_success "RAUC update completed in ${duration} seconds"
    else
        log_error "RAUC update failed"
        exit 1
    fi
}

wait_for_reboot() {
    log_info "=== Step 5: Wait for System Reboot ==="
    
    log_info "Initiating system reboot..."
    ssh -o StrictHostKeyChecking=no "$DEFAULT_USER@$TARGET_HOST" "reboot" 2>/dev/null || true
    
    # Wait a bit for reboot to start
    sleep 10
    
    log_info "Waiting for system to come back online (timeout: ${REBOOT_TIMEOUT}s)..."
    local start_time=$(date +%s)
    local attempt=1
    
    while true; do
        local current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        
        if [[ $elapsed -gt $REBOOT_TIMEOUT ]]; then
            log_error "Reboot timeout exceeded (${REBOOT_TIMEOUT}s)"
            exit 1
        fi
        
        log_info "Attempt $attempt (${elapsed}s elapsed): Checking connectivity..."
        
        # Clean host keys that may have changed
        ssh-keygen -f "$HOME/.ssh/known_hosts" -R "$TARGET_HOST" 2>/dev/null || true
        
        if timeout 5 ssh -o StrictHostKeyChecking=no -o ConnectTimeout=3 "$DEFAULT_USER@$TARGET_HOST" "echo 'Online'" &>/dev/null; then
            log_success "System is back online after ${elapsed} seconds"
            break
        fi
        
        log_info "System not ready yet, waiting ${CONNECTION_RETRY_INTERVAL} seconds..."
        sleep $CONNECTION_RETRY_INTERVAL
        ((attempt++))
    done
}

verify_update() {
    log_info "=== Step 6: Verify Update Results ==="
    
    # Give system a moment to fully boot
    sleep 5
    
    # Get post-update status
    local rauc_status=$(ssh -o StrictHostKeyChecking=no "$DEFAULT_USER@$TARGET_HOST" "rauc status")
    local uptime=$(ssh -o StrictHostKeyChecking=no "$DEFAULT_USER@$TARGET_HOST" "uptime")
    local df_info=$(ssh -o StrictHostKeyChecking=no "$DEFAULT_USER@$TARGET_HOST" "df -h /")
    
    # Verify slot switch
    local current_slot=""
    if echo "$rauc_status" | grep -q "rootfs.0.*booted"; then
        current_slot="A"
    elif echo "$rauc_status" | grep -q "rootfs.1.*booted"; then
        current_slot="B"
    else
        log_error "Could not determine current boot slot after update"
        exit 1
    fi
    
    log_info "Post-update system state:"
    log_info "  • Current slot: $current_slot"
    log_info "  • Previous slot: $INITIAL_SLOT"
    log_info "  • Uptime: $uptime"
    log_info "  • Disk usage: $(echo "$df_info" | tail -1)"
    
    # Verify slot switch occurred
    if [[ "$current_slot" == "$TARGET_SLOT" ]]; then
        log_success "Slot switch verified: $INITIAL_SLOT → $current_slot"
    else
        log_error "Slot switch failed: Expected $TARGET_SLOT, got $current_slot"
        exit 1
    fi
    
    # Show detailed RAUC status
    log_info "Detailed RAUC status:"
    echo "$rauc_status" | while read -r line; do
        log_info "  $line"
    done
}

cleanup_test() {
    log_info "=== Step 7: Cleanup ==="
    
    log_info "Cleaning up temporary files..."
    if uv run rauc-updater cleanup --host "$TARGET_HOST"; then
        log_success "Cleanup completed"
    else
        log_warning "Cleanup had issues (non-critical)"
    fi
}

generate_summary() {
    log_info "=== Test Summary ==="
    
    local test_end_time=$(date +%s)
    local total_duration=$((test_end_time - TEST_START_TIME))
    local minutes=$((total_duration / 60))
    local seconds=$((total_duration % 60))
    
    log_success "🎯 End-to-End RAUC Update Test COMPLETED!"
    log_success "📊 Test Results:"
    log_success "  • Bundle: $(basename "$BUNDLE_FILE")"
    log_success "  • Target: $TARGET_HOST"
    log_success "  • Slot transition: $INITIAL_SLOT → $TARGET_SLOT"
    log_success "  • Total time: ${minutes}m ${seconds}s"
    log_success "  • Log file: $LOG_FILE"
    
    echo ""
    echo "=================================================="
    echo "           RAUC AUTO TEST SUCCESSFUL"
    echo "=================================================="
}

# Main execution
main() {
    TEST_START_TIME=$(date +%s)
    
    echo "=================================================="
    echo "     RAUC Automated End-to-End Test Script"
    echo "=================================================="
    echo "Bundle: $BUNDLE_FILE"
    echo "Target: $TARGET_HOST"
    echo "Log: $LOG_FILE"
    echo "Start: $(date)"
    echo "=================================================="
    
    check_prerequisites
    get_initial_state
    setup_ssh_keys
    test_connection
    perform_update
    wait_for_reboot
    verify_update
    cleanup_test
    generate_summary
}

# Script help
show_help() {
    echo "RAUC Automated End-to-End Test Script"
    echo ""
    echo "Usage: $0 [bundle_file] [target_host]"
    echo ""
    echo "Arguments:"
    echo "  bundle_file   Path to RAUC bundle (default: $DEFAULT_BUNDLE)"
    echo "  target_host   Target device IP (default: $DEFAULT_HOST)"
    echo ""
    echo "Examples:"
    echo "  $0                                           # Use defaults"
    echo "  $0 /path/to/bundle.raucb                    # Custom bundle"
    echo "  $0 /path/to/bundle.raucb 192.168.1.150     # Custom bundle and host"
    echo ""
    echo "Environment Variables:"
    echo "  TARGET_HOST         Override default host"
    echo "  TARGET_USER         Override default user (default: root)"
    echo "  TARGET_PASSWORD     Override default password (default: root)"
    echo "  REBOOT_TIMEOUT      Override reboot timeout (default: 300s)"
    echo ""
}

# Handle command line arguments
if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    show_help
    exit 0
fi

# Override defaults with environment variables if set
TARGET_HOST="${TARGET_HOST:-$DEFAULT_HOST}"
DEFAULT_USER="${TARGET_USER:-$DEFAULT_USER}"
DEFAULT_PASSWORD="${TARGET_PASSWORD:-$DEFAULT_PASSWORD}"
REBOOT_TIMEOUT="${REBOOT_TIMEOUT:-$REBOOT_TIMEOUT}"

# Run main function
main "$@"