#!/bin/bash

# Deploy CMake-built RAUC to target NUC
# Usage: ./deploy_cmake_rauc.sh [target_ip]

TARGET_IP=${1:-192.168.1.100}
TARGET_USER="root"
RAUC_BINARY="./build/rauc"

echo "Deploying CMake-built RAUC to $TARGET_USER@$TARGET_IP"

# Check if binary exists
if [ ! -f "$RAUC_BINARY" ]; then
    echo "Error: RAUC binary not found at $RAUC_BINARY"
    echo "Please run 'make' first to build the binary"
    exit 1
fi

# Setup network connection (skip for now)
echo "Skipping network setup - assuming connection already exists..."

# Copy RAUC binary to target
echo "Copying RAUC binary to target..."
scp -o StrictHostKeyChecking=no "$RAUC_BINARY" "$TARGET_USER@$TARGET_IP:/tmp/rauc_cmake"

# Test on target
echo "Testing RAUC on target..."
ssh -o StrictHostKeyChecking=no "$TARGET_USER@$TARGET_IP" << 'EOF'
echo "=== Testing CMake-built RAUC ==="
echo "Binary info:"
file /tmp/rauc_cmake
echo ""
echo "Testing --version:"
/tmp/rauc_cmake --version
echo ""
echo "Testing --help (first 10 lines):"
/tmp/rauc_cmake --help | head -10
echo ""
echo "Testing status command:"
/tmp/rauc_cmake status 2>&1 || echo "Note: Status may fail without proper config"
echo ""
echo "Comparing with original RAUC:"
echo "Original RAUC version:"
/usr/bin/rauc --version 2>&1 || echo "Original RAUC not found"
echo ""
echo "=== Test Complete ==="
EOF

echo "Deployment and testing complete!"