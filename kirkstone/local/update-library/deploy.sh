#!/bin/bash

# Network and target configuration (from connect.sh)
IFACE="enp42s0"
HOST_IP="192.168.1.101"
NETMASK="255.255.255.0"
TARGET_IP="192.168.1.100"
TARGET_USER="root"

set -e

echo "Deploying update-library to target device $TARGET_USER@$TARGET_IP..."

# Build the library and test application
echo "[1] Building update-library..."
./build.sh

# Remote deployment
echo "[2] Checking network configuration..."
CURRENT_IP=$(ip addr show $IFACE | grep "inet " | awk '{print $2}' | cut -d'/' -f1)
if [ "$CURRENT_IP" != "$HOST_IP" ]; then
    echo "Setting IP address for $IFACE..."
    sudo ifconfig $IFACE $HOST_IP netmask $NETMASK up
else
    echo "Network interface $IFACE already configured with IP $HOST_IP"
fi

echo "[3] Testing SSH connection to target..."
if ! ssh -o BatchMode=yes -o ConnectTimeout=5 $TARGET_USER@$TARGET_IP exit 2>/dev/null; then
    echo "SSH connection failed. Please run ./connect.sh first to set up SSH keys."
    exit 1
fi
echo "SSH connection successful"

echo "[4] Copying library files to target device..."
scp build/libupdate-library.a $TARGET_USER@$TARGET_IP:/tmp/libupdate-library.a-new

echo "[5] Copying test application to target device..."
scp build/update-test-app $TARGET_USER@$TARGET_IP:/tmp/update-test-app-new

echo "[6] Copying header files to target device..."
# Create temporary directory for headers
mkdir -p /tmp/update-library-headers
cp -r include/* /tmp/update-library-headers/
tar -czf /tmp/update-library-headers.tar.gz -C /tmp update-library-headers
scp /tmp/update-library-headers.tar.gz $TARGET_USER@$TARGET_IP:/tmp/
rm -rf /tmp/update-library-headers /tmp/update-library-headers.tar.gz

echo "[7] Deploying on target device..."
ssh $TARGET_USER@$TARGET_IP << 'EOF'
    echo "Creating library directories..."
    mkdir -p /usr/local/lib
    mkdir -p /usr/local/include/update-library
    mkdir -p /usr/local/bin

    echo "Installing library..."
    cp /tmp/libupdate-library.a-new /usr/local/lib/libupdate-library.a
    chmod 644 /usr/local/lib/libupdate-library.a

    echo "Installing test application..."
    cp /tmp/update-test-app-new /usr/local/bin/update-test-app
    chmod +x /usr/local/bin/update-test-app

    echo "Installing header files..."
    cd /tmp
    tar -xzf update-library-headers.tar.gz
    cp -r update-library-headers/* /usr/local/include/update-library/
    chmod -R 644 /usr/local/include/update-library/

    echo "Updating library cache..."
    ldconfig

    echo "Cleaning up temporary files..."
    rm -f /tmp/libupdate-library.a-new /tmp/update-test-app-new /tmp/update-library-headers.tar.gz
    rm -rf /tmp/update-library-headers

    echo "Verifying installation..."
    echo "Library file:"
    ls -la /usr/local/lib/libupdate-library.a
    echo "Test application:"
    ls -la /usr/local/bin/update-test-app
    echo "Header files:"
    ls -la /usr/local/include/update-library/
EOF

echo "Remote deployment completed!"
echo "Library installed to: /usr/local/lib/libupdate-library.a"
echo "Headers installed to: /usr/local/include/update-library/"
echo "Test application installed to: /usr/local/bin/update-test-app"
echo ""
echo "Test the installation with:"
echo "  ssh $TARGET_USER@$TARGET_IP /usr/local/bin/update-test-app --help"
