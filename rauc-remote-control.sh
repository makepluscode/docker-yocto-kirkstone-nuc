#!/bin/bash

# RAUC Remote Control Script
# This script provides remote access to RAUC functionality via SSH tunnel

TARGET_IP="${1:-192.168.1.100}"
TARGET_USER="${2:-root}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_usage() {
    echo "Usage: $0 [TARGET_IP] [TARGET_USER]"
    echo "  TARGET_IP   - IP address of target device (default: 192.168.1.100)"
    echo "  TARGET_USER - SSH user on target device (default: root)"
    echo ""
    echo "Commands:"
    echo "  status      - Get RAUC system status"
    echo "  install     - Install RAUC bundle"
    echo "  info        - Get bundle information"
    echo "  mark        - Mark slot as good/bad"
    echo ""
}

rauc_status() {
    echo -e "${GREEN}Getting RAUC status from ${TARGET_USER}@${TARGET_IP}...${NC}"
    ssh ${TARGET_USER}@${TARGET_IP} "rauc status"
}

rauc_install() {
    local bundle_path="$1"
    if [ -z "$bundle_path" ]; then
        echo -e "${RED}Error: Bundle path required${NC}"
        echo "Usage: rauc_install <bundle_path_on_target>"
        return 1
    fi
    
    echo -e "${GREEN}Installing RAUC bundle ${bundle_path} on ${TARGET_USER}@${TARGET_IP}...${NC}"
    ssh ${TARGET_USER}@${TARGET_IP} "rauc install ${bundle_path}"
}

rauc_info() {
    local bundle_path="$1"
    if [ -z "$bundle_path" ]; then
        echo -e "${RED}Error: Bundle path required${NC}"
        echo "Usage: rauc_info <bundle_path_on_target>"
        return 1
    fi
    
    echo -e "${GREEN}Getting bundle info for ${bundle_path} from ${TARGET_USER}@${TARGET_IP}...${NC}"
    ssh ${TARGET_USER}@${TARGET_IP} "rauc info ${bundle_path}"
}

rauc_mark() {
    local slot="$1"
    local state="$2"
    if [ -z "$slot" ] || [ -z "$state" ]; then
        echo -e "${RED}Error: Slot and state required${NC}"
        echo "Usage: rauc_mark <slot> <good|bad|active>"
        return 1
    fi
    
    echo -e "${GREEN}Marking slot ${slot} as ${state} on ${TARGET_USER}@${TARGET_IP}...${NC}"
    ssh ${TARGET_USER}@${TARGET_IP} "rauc status mark-${state} ${slot}"
}

# D-Bus based remote control (requires D-Bus over network)
rauc_dbus_status() {
    echo -e "${GREEN}Getting RAUC status via D-Bus from ${TARGET_USER}@${TARGET_IP}...${NC}"
    ssh ${TARGET_USER}@${TARGET_IP} "dbus-send --system --print-reply --dest=de.pengutronix.rauc /de/pengutronix/rauc/Installer de.pengutronix.rauc.Installer.GetSlotStatus"
}

rauc_dbus_install() {
    local bundle_path="$1"
    if [ -z "$bundle_path" ]; then
        echo -e "${RED}Error: Bundle path required${NC}"
        echo "Usage: rauc_dbus_install <bundle_path_on_target>"
        return 1
    fi
    
    echo -e "${GREEN}Installing RAUC bundle ${bundle_path} via D-Bus on ${TARGET_USER}@${TARGET_IP}...${NC}"
    ssh ${TARGET_USER}@${TARGET_IP} "dbus-send --system --print-reply --dest=de.pengutronix.rauc /de/pengutronix/rauc/Installer de.pengutronix.rauc.Installer.Install string:\"${bundle_path}\""
}

# Test connectivity
test_connection() {
    echo -e "${YELLOW}Testing connection to ${TARGET_USER}@${TARGET_IP}...${NC}"
    if ssh -o ConnectTimeout=5 ${TARGET_USER}@${TARGET_IP} "echo 'Connection successful'"; then
        echo -e "${GREEN}✓ SSH connection successful${NC}"
        
        # Test if RAUC is available
        if ssh ${TARGET_USER}@${TARGET_IP} "which rauc > /dev/null"; then
            echo -e "${GREEN}✓ RAUC binary found${NC}"
        else
            echo -e "${RED}✗ RAUC binary not found${NC}"
        fi
        
        # Test if RAUC service is running
        if ssh ${TARGET_USER}@${TARGET_IP} "systemctl is-active rauc.service > /dev/null 2>&1"; then
            echo -e "${GREEN}✓ RAUC service is running${NC}"
        else
            echo -e "${YELLOW}⚠ RAUC service is not running${NC}"
        fi
        
        # Test D-Bus interface
        if ssh ${TARGET_USER}@${TARGET_IP} "dbus-send --system --print-reply --dest=de.pengutronix.rauc /de/pengutronix/rauc/Installer org.freedesktop.DBus.Introspectable.Introspect > /dev/null 2>&1"; then
            echo -e "${GREEN}✓ RAUC D-Bus interface is available${NC}"
        else
            echo -e "${YELLOW}⚠ RAUC D-Bus interface is not available${NC}"
        fi
    else
        echo -e "${RED}✗ SSH connection failed${NC}"
        return 1
    fi
}

# Main script logic
case "$3" in
    "status")
        rauc_status
        ;;
    "install")
        rauc_install "$4"
        ;;
    "info")
        rauc_info "$4"
        ;;
    "mark")
        rauc_mark "$4" "$5"
        ;;
    "dbus-status")
        rauc_dbus_status
        ;;
    "dbus-install")
        rauc_dbus_install "$4"
        ;;
    "test")
        test_connection
        ;;
    *)
        if [ $# -eq 0 ]; then
            print_usage
        else
            echo -e "${YELLOW}Running connection test...${NC}"
            test_connection
        fi
        ;;
esac