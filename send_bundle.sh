#!/bin/bash
# Deploy RAUC bundle to target NUC device

set -e

# Show usage information
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    echo "Usage: $0 [TARGET_IP] [TARGET_USER] [--skip-network]"
    echo ""
    echo "Deploy RAUC bundle to target NUC device"
    echo ""
    echo "Arguments:"
    echo "  TARGET_IP       Target device IP address (default: 192.168.1.100)"
    echo "  TARGET_USER     SSH user on target device (default: root)"
    echo "  --skip-network  Skip network interface configuration"
    echo ""
    echo "Examples:"
    echo "  $0                          # Deploy to 192.168.1.100 as root"
    echo "  $0 192.168.1.150           # Deploy to 192.168.1.150 as root"
    echo "  $0 192.168.1.100 nuc       # Deploy to 192.168.1.100 as nuc user"
    echo "  $0 192.168.1.100 root --skip-network  # Skip network setup"
    echo ""
    echo "Prerequisites:"
    echo "  - RAUC bundle built with './build.sh bundle'"
    echo "  - Target device powered on and network accessible"
    echo "  - SSH access to target device"
    echo ""
    exit 0
fi

# Parse arguments
SKIP_NETWORK=false
for arg in "$@"; do
    if [[ "$arg" == "--skip-network" ]]; then
        SKIP_NETWORK=true
        break
    fi
done

# Network configuration (matching connect.sh)
IFACE="enp42s0"
HOST_IP="192.168.1.101"
NETMASK="255.255.255.0"

# Configuration
TARGET_IP="${1:-192.168.1.100}"
TARGET_USER="${2:-root}"
BUNDLE_PATH="kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-bundle-intel-corei7-64.raucb"
TARGET_DIR="/data"
BUNDLE_NAME="nuc-image-qt5-bundle-intel-corei7-64.raucb"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

setup_network() {
    print_status "Checking network interface $IFACE..."
    
    # Check if interface already has the correct IP
    CURRENT_IP=$(ip addr show $IFACE 2>/dev/null | grep "inet $HOST_IP" || true)
    if [[ -n "$CURRENT_IP" ]]; then
        print_status "Network interface already configured: $HOST_IP"
        return 0
    fi
    
    # Try to configure the interface
    print_status "Setting up network interface $IFACE..."
    if sudo ifconfig $IFACE $HOST_IP netmask $NETMASK up 2>/dev/null; then
        print_status "Network interface configured: $HOST_IP"
    else
        print_warning "Could not configure network interface $IFACE"
        print_warning "You may need to configure it manually or run with sudo"
        print_warning "Continuing anyway - target may still be reachable..."
        return 0  # Don't fail, just warn
    fi
}

setup_ssh_keys() {
    print_status "Setting up SSH keys for target connection..."
    
    # Fix SSH known_hosts permission if needed
    KNOWN_HOSTS="$HOME/.ssh/known_hosts"
    if [ -e "$KNOWN_HOSTS" ]; then
        if [ ! -w "$KNOWN_HOSTS" ]; then
            print_status "Fixing permissions for $KNOWN_HOSTS"
            sudo chown $USER:$USER "$KNOWN_HOSTS"
            sudo chmod 600 "$KNOWN_HOSTS"
        fi
    else
        mkdir -p "$HOME/.ssh"
        touch "$KNOWN_HOSTS"
        chmod 700 "$HOME/.ssh"
        chmod 600 "$KNOWN_HOSTS"
    fi
    
    # Remove old host key and add new one
    print_status "Updating SSH host key for $TARGET_IP..."
    ssh-keygen -f "$KNOWN_HOSTS" -R "$TARGET_IP" 2>/dev/null || true
    ssh-keyscan -H "$TARGET_IP" >> "$KNOWN_HOSTS" 2>/dev/null || true
}

# Check if bundle exists
if [[ ! -f "$BUNDLE_PATH" ]]; then
    print_error "RAUC bundle not found at: $BUNDLE_PATH"
    print_error "Please build the bundle first with: bitbake nuc-image-qt5-bundle"
    exit 1
fi

print_status "Found RAUC bundle: $BUNDLE_PATH"
# Follow symlink to get actual file size
REAL_BUNDLE_PATH=$(readlink -f "$BUNDLE_PATH")
print_status "Bundle size: $(du -h "$REAL_BUNDLE_PATH" | cut -f1)"

# Setup network and SSH (skip for localhost or local testing)
if [[ "$TARGET_IP" == "localhost" || "$TARGET_IP" == "127.0.0.1" ]]; then
    print_status "Skipping network setup for local target"
elif [[ "$SKIP_NETWORK" == "true" ]]; then
    print_status "Skipping network setup (--skip-network specified)"
    # Still setup SSH keys
    setup_ssh_keys
else
    # Setup network interface
    setup_network
    
    # Setup SSH keys
    setup_ssh_keys
    
    # Test connectivity
    print_status "Testing connection to target: $TARGET_IP"
    if ! ping -c 1 -W 3 "$TARGET_IP" &>/dev/null; then
        print_error "Cannot reach target device at $TARGET_IP"
        print_error "Please check network connectivity and target IP address"
        print_warning "Make sure the target device is powered on and connected"
        print_warning "Try using --skip-network if network is already configured"
        exit 1
    fi
    print_status "Target device is reachable"
fi

# Copy bundle to target
print_status "Copying RAUC bundle to target device..."
print_status "Source: $BUNDLE_PATH"
print_status "Destination: $TARGET_USER@$TARGET_IP:$TARGET_DIR/$BUNDLE_NAME"

if scp -o ConnectTimeout=10 -o StrictHostKeyChecking=no "$BUNDLE_PATH" "$TARGET_USER@$TARGET_IP:$TARGET_DIR/$BUNDLE_NAME"; then
    print_status "Bundle copied successfully to target"
else
    print_error "Failed to copy bundle to target"
    print_error "Check SSH connectivity and target disk space"
    exit 1
fi

# Verify bundle on target
print_status "Verifying bundle on target device..."
REMOTE_SIZE=$(ssh -o ConnectTimeout=10 -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_IP" "ls -la $TARGET_DIR/$BUNDLE_NAME 2>/dev/null | awk '{print \$5}'" || echo "0")
LOCAL_SIZE=$(stat -c%s "$REAL_BUNDLE_PATH")

if [[ "$REMOTE_SIZE" -eq "$LOCAL_SIZE" ]]; then
    print_status "Bundle verification successful (size: $LOCAL_SIZE bytes)"
else
    print_error "Bundle verification failed (local: $LOCAL_SIZE, remote: $REMOTE_SIZE)"
    exit 1
fi

print_status "========================================="
print_status "RAUC Bundle Deployment Complete!"
print_status "========================================="
print_status "Bundle location on target: $TARGET_DIR/$BUNDLE_NAME"
print_status ""
print_status "To install the update, run on the target device:"
print_status "  sudo rauc install $TARGET_DIR/$BUNDLE_NAME"
print_status ""
print_status "To check RAUC status:"
print_status "  rauc status"
print_status ""
print_warning "IMPORTANT: The system will require a reboot after installation"
print_warning "to switch to the new partition."
