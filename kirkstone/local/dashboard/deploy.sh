#!/bin/bash

# Dashboard Deploy Script (Improved)
# This script builds the dashboard using build.sh and deploys to target
# Usage: ./deploy.sh [user@]TARGET_IP
# Defaults: TARGET_IP=192.168.1.100  USER=root

set -e  # Exit on any error

# Default target
TARGET=${1:-root@192.168.1.100}

# Network configuration (from connect.sh)
IFACE="enp42s0"
HOST_IP="192.168.1.101"
NETMASK="255.255.255.0"

# Extract target IP from TARGET variable
TARGET_IP=$(echo "$TARGET" | cut -d@ -f2)
TARGET_USER=$(echo "$TARGET" | cut -d@ -f1)

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

echo "🚀 Dashboard Deploy Script (Improved)"
echo "===================================="

# Check if we're in the dashboard directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "This script must be run from the dashboard directory"
    exit 1
fi

# Check if build.sh exists
if [ ! -f "build.sh" ]; then
    print_error "build.sh not found in current directory"
    exit 1
fi

# Build the dashboard using build.sh
print_status "Building dashboard using build.sh..."
if ./build.sh; then
    print_success "Dashboard build completed successfully!"
else
    print_error "Dashboard build failed"
    exit 1
fi

# Verify the binary was created
if [ ! -f "build/dashboard" ]; then
    print_error "Dashboard binary not found at build/dashboard"
    exit 1
fi

print_status "Binary: $(pwd)/build/dashboard"

# Network setup (from connect.sh)
print_status "Setting up network connection..."
print_status "Setting IP address for $IFACE..."
sudo ifconfig $IFACE $HOST_IP netmask $NETMASK up

print_status "Fixing SSH known_hosts permission if needed..."
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

print_status "Removing old host key for $TARGET_IP if exists..."
ssh-keygen -f "$KNOWN_HOSTS" -R "$TARGET_IP" 2>/dev/null || true

print_status "Adding new host key for $TARGET_IP..."
ssh-keyscan -H "$TARGET_IP" >> "$KNOWN_HOSTS" 2>/dev/null || true

# Check if service file exists
SERVICE_FILE="services/dashboard-eglfs.service"
if [ ! -f "$SERVICE_FILE" ]; then
    print_warning "Service file not found: $SERVICE_FILE"
    print_warning "Only binary will be deployed"
    DEPLOY_SERVICE=false
else
    print_success "Found service file: $SERVICE_FILE"
    DEPLOY_SERVICE=true
fi

print_status "Deploying to target: $TARGET"

# Test SSH connection
print_status "Testing SSH connection..."
if ! ssh -o ConnectTimeout=5 -o BatchMode=yes "$TARGET" exit 2>/dev/null; then
    print_error "Cannot connect to $TARGET via SSH"
    print_error "Make sure the target device is powered on and connected to the network"
    print_error "You may need to run: ./connect.sh first to set up the connection"
    exit 1
fi

print_success "SSH connection successful"

# Deploy files
print_status "Copying dashboard binary..."
scp "build/dashboard" "$TARGET:/usr/bin/dashboard.new"

if [ "$DEPLOY_SERVICE" = true ]; then
    print_status "Copying service file..."
    scp "$SERVICE_FILE" "$TARGET:/lib/systemd/system/dashboard-eglfs.service"
fi

# Always copy QML files to target
print_status "Copying QML files to target..."
ssh "$TARGET" "mkdir -p /usr/share/dashboard/qml"
scp qml/*.qml "$TARGET:/usr/share/dashboard/qml/"

if [ $? -eq 0 ]; then
    print_success "QML files copied successfully"
else
    print_warning "Failed to copy QML files, but continuing deployment..."
fi

# Execute deployment commands on target
print_status "Executing deployment commands on target..."
ssh "$TARGET" <<'EOF'
set -e

echo "Stopping dashboard service..."
systemctl stop dashboard-eglfs.service || true

echo "Replacing dashboard binary..."
mv /usr/bin/dashboard.new /usr/bin/dashboard
chmod +x /usr/bin/dashboard

echo "Reloading systemd..."
systemctl daemon-reload

echo "Enabling dashboard service..."
systemctl enable dashboard-eglfs.service

echo "Starting dashboard service..."
systemctl start dashboard-eglfs.service

echo "Waiting for service to start..."
sleep 3

echo "Checking service status..."
systemctl status dashboard-eglfs.service --no-pager

echo "Checking recent logs..."
journalctl -u dashboard-eglfs.service --no-pager -n 10
EOF

if [ $? -eq 0 ]; then
    print_success "Deployment completed successfully!"
    print_status "Dashboard service is now running on $TARGET"
    print_status "Check status: ssh $TARGET 'systemctl status dashboard-eglfs.service --no-pager'"
    print_status "Check logs: ssh $TARGET 'journalctl -u dashboard-eglfs.service --no-pager -f'"
else
    print_error "Deployment failed"
    exit 1
fi 