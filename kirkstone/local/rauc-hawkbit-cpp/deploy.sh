#!/bin/bash

# Network and target configuration (from connect.sh)
IFACE="enp42s0"
HOST_IP="192.168.1.101"
NETMASK="255.255.255.0"
TARGET_IP="192.168.1.100"
TARGET_USER="root"

set -e

echo "Deploying rauc-hawkbit-cpp to target device $TARGET_USER@$TARGET_IP..."

# Build the application
echo "[1] Building rauc-hawkbit-cpp..."
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

echo "[4] Copying binary to target device..."
scp build/rauc-hawkbit-cpp $TARGET_USER@$TARGET_IP:/tmp/rauc-hawkbit-cpp-new

echo "[5] Copying service file to target device..."
scp services/rauc-hawkbit-cpp.service $TARGET_USER@$TARGET_IP:/tmp/rauc-hawkbit-cpp.service-new

echo "[6] Deploying on target device..."
ssh $TARGET_USER@$TARGET_IP << 'EOF'
    echo "Stopping rauc-hawkbit-cpp service..."
    systemctl stop rauc-hawkbit-cpp || true
    
    echo "Installing new binary..."
    cp /tmp/rauc-hawkbit-cpp-new /usr/local/bin/rauc-hawkbit-cpp
    chmod +x /usr/local/bin/rauc-hawkbit-cpp
    
    echo "Installing service file..."
    cp /tmp/rauc-hawkbit-cpp.service-new /etc/systemd/system/rauc-hawkbit-cpp.service
    
    echo "Reloading systemd..."
    systemctl daemon-reload
    systemctl enable rauc-hawkbit-cpp.service
    
    echo "Starting service..."
    systemctl start rauc-hawkbit-cpp.service
    
    echo "Cleaning up temporary files..."
    rm -f /tmp/rauc-hawkbit-cpp-new /tmp/rauc-hawkbit-cpp.service-new
    
    echo "Checking service status..."
    systemctl status rauc-hawkbit-cpp --no-pager -l
EOF

echo "Remote deployment completed!"
echo "Check target status with: ssh $TARGET_USER@$TARGET_IP systemctl status rauc-hawkbit-cpp"
echo "View target logs with: ssh $TARGET_USER@$TARGET_IP journalctl -u rauc-hawkbit-cpp -f" 