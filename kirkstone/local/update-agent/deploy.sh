#!/bin/bash

# Network and target configuration (from connect.sh)
IFACE="enp42s0"
HOST_IP="192.168.1.101"
NETMASK="255.255.255.0"
TARGET_IP="192.168.1.100"
TARGET_USER="root"

set -e

echo "Deploying update-agent to target device $TARGET_USER@$TARGET_IP..."

# Build the application
echo "[1] Building update-agent..."
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
scp build/update-agent $TARGET_USER@$TARGET_IP:/tmp/update-agent-new

echo "[5] Copying service file to target device..."
scp services/update-agent.service $TARGET_USER@$TARGET_IP:/tmp/update-agent.service-new

echo "[6] Deploying on target device..."
ssh $TARGET_USER@$TARGET_IP << 'EOF'
    echo "Stopping update-agent service..."
    systemctl stop update-agent || true
    
    echo "Installing new binary..."
    cp /tmp/update-agent-new /usr/local/bin/update-agent
    chmod +x /usr/local/bin/update-agent
    
    echo "Installing service file..."
    cp /tmp/update-agent.service-new /etc/systemd/system/update-agent.service
    
    echo "Reloading systemd..."
    systemctl daemon-reload
    systemctl enable update-agent.service
    
    echo "Starting service..."
    systemctl start update-agent.service
    
    echo "Cleaning up temporary files..."
    rm -f /tmp/update-agent-new /tmp/update-agent.service-new
    
    echo "Checking service status..."
    systemctl status update-agent --no-pager -l
EOF

echo "Remote deployment completed!"
echo "Check target status with: ssh $TARGET_USER@$TARGET_IP systemctl status update-agent"
echo "View target logs with: ssh $TARGET_USER@$TARGET_IP journalctl -u update-agent -f" 