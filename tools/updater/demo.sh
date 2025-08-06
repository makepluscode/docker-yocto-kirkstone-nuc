#!/bin/bash

# RAUC Test Demo Script
# Demonstrates various usage scenarios of the complete test script

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_LOG="$SCRIPT_DIR/demo.log"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_demo() {
    echo -e "${BLUE}[DEMO]${NC} $*" | tee -a "$DEMO_LOG"
}

show_demo_header() {
    clear
    cat << EOF
==================================================
    🚀 RAUC Complete Test Script Demo
==================================================

This demo shows various ways to use the automated
RAUC testing script with different scenarios.

Available demos:
  1. Basic test (single update)
  2. Double test (round-trip update)
  3. Verbose test with custom parameters
  4. Batch test mode
  5. Network setup + test

Choose a demo or press Ctrl+C to exit.

EOF
}

demo_basic_test() {
    log_demo "=== Demo 1: Basic Single Update Test ==="
    echo "This will run a basic RAUC update test with default settings."
    echo "Target: 192.168.1.100"
    echo "Bundle: default bundle"
    echo "Scenario: A/B slot switch"
    echo ""
    read -p "Press Enter to continue or Ctrl+C to cancel..."
    
    log_demo "Running: ./rauc_test_complete.sh"
    echo "./rauc_test_complete.sh"
    echo ""
    echo "This would execute a complete end-to-end test including:"
    echo "✓ Prerequisites check"
    echo "✓ System state capture"
    echo "✓ SSH key setup"
    echo "✓ Connection testing"
    echo "✓ RAUC bundle update"
    echo "✓ System reboot monitoring"
    echo "✓ Update verification"
    echo "✓ Cleanup"
}

demo_double_test() {
    log_demo "=== Demo 2: Double Update Test ==="
    echo "This will run two consecutive updates to return to original slot."
    echo "Scenario: A→B→A or B→A→B"
    echo ""
    read -p "Press Enter to continue or Ctrl+C to cancel..."
    
    log_demo "Running: ./rauc_test_complete.sh -s double -v"
    echo "./rauc_test_complete.sh -s double -v"
    echo ""
    echo "This would perform:"
    echo "✓ First update (e.g., A→B)"
    echo "✓ Verification of first update"
    echo "✓ Second update (e.g., B→A)"  
    echo "✓ Verification of return to original slot"
}

demo_custom_test() {
    log_demo "=== Demo 3: Custom Parameters Test ==="
    echo "This shows how to use custom parameters for different scenarios."
    echo ""
    read -p "Press Enter to continue or Ctrl+C to cancel..."
    
    log_demo "Examples of custom parameter usage:"
    echo ""
    echo "# Custom target device:"
    echo "./rauc_test_complete.sh -H 192.168.1.150 -P mypassword"
    echo ""
    echo "# Custom bundle file:"
    echo "./rauc_test_complete.sh -b /path/to/custom-bundle.raucb -v"
    echo ""
    echo "# Extended timeout for slow networks:"
    echo "./rauc_test_complete.sh -t 600 -v"
    echo ""
    echo "# No cleanup (for debugging):"
    echo "./rauc_test_complete.sh -c -v"
}

demo_batch_test() {
    log_demo "=== Demo 4: Batch Test Mode ==="
    echo "This shows batch testing across multiple devices/scenarios."
    echo ""
    read -p "Press Enter to continue or Ctrl+C to cancel..."
    
    log_demo "Running: ./rauc_test_complete.sh -m batch -v"
    echo "./rauc_test_complete.sh -m batch -v"
    echo ""
    echo "This would run tests on multiple devices:"
    echo "✓ Device 192.168.1.100 - basic scenario"
    echo "✓ Device 192.168.1.100 - double scenario"
    echo "✓ Generate HTML report with results"
    echo "✓ Overall pass/fail summary"
}

demo_network_setup() {
    log_demo "=== Demo 5: Network Setup + Test ==="
    echo "This shows automatic network configuration before testing."
    echo ""
    read -p "Press Enter to continue or Ctrl+C to cancel..."
    
    log_demo "Running: ./rauc_test_complete.sh -n -v"
    echo "./rauc_test_complete.sh -n -v"
    echo ""
    echo "This would perform:"
    echo "✓ Network interface configuration"
    echo "✓ Connectivity testing"
    echo "✓ SSH key setup"
    echo "✓ Complete RAUC update test"
}

show_actual_command() {
    echo ""
    echo -e "${GREEN}=== Ready to run actual test? ===${NC}"
    echo "You can now run the actual test with any of these commands:"
    echo ""
    echo -e "${YELLOW}Basic test:${NC}"
    echo "  ./rauc_test_complete.sh"
    echo ""
    echo -e "${YELLOW}Verbose test:${NC}"
    echo "  ./rauc_test_complete.sh -v"
    echo ""
    echo -e "${YELLOW}Double update:${NC}"
    echo "  ./rauc_test_complete.sh -s double -v"
    echo ""
    echo -e "${YELLOW}Custom target:${NC}"
    echo "  ./rauc_test_complete.sh -H 192.168.1.150 -v"
    echo ""
    echo -e "${YELLOW}Full help:${NC}"
    echo "  ./rauc_test_complete.sh --help"
    echo ""
}

main_menu() {
    while true; do
        show_demo_header
        echo -n "Select demo (1-5): "
        read -r choice
        
        case $choice in
            1)
                demo_basic_test
                ;;
            2)
                demo_double_test
                ;;
            3)
                demo_custom_test
                ;;
            4)
                demo_batch_test
                ;;
            5)
                demo_network_setup
                ;;
            *)
                echo "Invalid choice. Please select 1-5."
                sleep 2
                continue
                ;;
        esac
        
        echo ""
        echo -n "Try another demo? (y/n): "
        read -r again
        
        if [[ "$again" != "y" && "$again" != "Y" ]]; then
            break
        fi
    done
    
    show_actual_command
}

# Check if script exists
if [[ ! -f "$SCRIPT_DIR/rauc_test_complete.sh" ]]; then
    echo "Error: rauc_test_complete.sh not found in $SCRIPT_DIR"
    exit 1
fi

# Initialize demo log
echo "RAUC Test Demo started at $(date)" > "$DEMO_LOG"

# Run main menu
main_menu

log_demo "Demo completed at $(date)"
echo ""
echo "Demo log saved to: $DEMO_LOG"