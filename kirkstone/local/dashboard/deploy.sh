#!/bin/bash

# Dashboard Deploy Script (Improved)
# This script validates the build environment, builds with Yocto toolchain, and deploys to target
# Usage: ./deploy.sh [user@]TARGET_IP
# Defaults: TARGET_IP=192.168.1.100  USER=root

set -e  # Exit on any error

# Default target
TARGET=${1:-root@192.168.1.100}

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

# Check if Yocto SDK is installed
SDK_PATH="/usr/local/oecore-x86_64"
if [ ! -d "$SDK_PATH" ]; then
    print_error "Yocto SDK not found at $SDK_PATH"
    print_error "Please install the Yocto SDK first"
    exit 1
fi

print_status "Setting up Yocto SDK environment..."
source "$SDK_PATH/environment-setup-corei7-64-oe-linux"

# Verify environment setup
if [ -z "$CC" ] || [ -z "$CXX" ]; then
    print_error "Failed to set up Yocto SDK environment"
    exit 1
fi

# Verify we're using the correct toolchain
if [[ "$CC" != *"x86_64-oe-linux"* ]]; then
    print_error "Wrong toolchain detected: $CC"
    print_error "Expected x86_64-oe-linux toolchain"
    exit 1
fi

print_success "Yocto SDK environment configured"
print_status "Compiler: $CC"
print_status "C++ Compiler: $CXX"
print_status "Sysroot: $PKG_CONFIG_SYSROOT_DIR"

# Clean and rebuild
print_status "Cleaning previous build..."
rm -rf build
mkdir -p build
cd build

# Configure with CMake using Yocto toolchain
print_status "Configuring with CMake (Yocto toolchain)..."
cmake .. -DCMAKE_TOOLCHAIN_FILE="$SDK_PATH/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake"

if [ $? -ne 0 ]; then
    print_error "CMake configuration failed"
    exit 1
fi

print_success "CMake configuration completed"

# Build the project
print_status "Building dashboard with Yocto toolchain..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    print_error "Build failed"
    exit 1
fi

# Verify the binary is built for target
print_status "Verifying binary compatibility..."
if ! file dashboard | grep -q "x86-64"; then
    print_error "Binary architecture verification failed"
    exit 1
fi

# Always copy QML files to target
print_status "Copying QML files to target..."
ssh "$TARGET" "mkdir -p /usr/share/dashboard/qml"
scp ../qml/*.qml "$TARGET:/usr/share/dashboard/qml/"

if [ $? -eq 0 ]; then
    print_success "QML files copied successfully"
else
    print_warning "Failed to copy QML files, but continuing deployment..."
fi

print_success "Dashboard build completed successfully!"
print_status "Binary: $(pwd)/dashboard"

# Go back to dashboard directory
cd ..

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
    print_error "Make sure SSH key authentication is set up"
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