#!/bin/bash

# 네트워크 설정
IFACE="enp42s0"
HOST_IP="192.168.1.101"
NETMASK="255.255.255.0"

# 타겟 정보
TARGET_IP="192.168.1.100"
TARGET_USER="root"

set -e

echo "[1] Checking network configuration for $IFACE..."
CURRENT_IP=$(ip addr show $IFACE | grep "inet " | awk '{print $2}' | cut -d'/' -f1)
if [ "$CURRENT_IP" != "$HOST_IP" ]; then
    echo "Setting IP address for $IFACE..."
    sudo ifconfig $IFACE $HOST_IP netmask $NETMASK up
else
    echo "Network interface $IFACE already configured with IP $HOST_IP"
fi

echo "[2] Fixing SSH known_hosts permission if needed..."
KNOWN_HOSTS="$HOME/.ssh/known_hosts"
if [ -e "$KNOWN_HOSTS" ]; then
    if [ ! -w "$KNOWN_HOSTS" ]; then
        echo "Fixing permissions for $KNOWN_HOSTS"
        sudo chown $USER:$USER "$KNOWN_HOSTS"
        sudo chmod 600 "$KNOWN_HOSTS"
    fi
else
    mkdir -p "$HOME/.ssh"
    touch "$KNOWN_HOSTS"
    chmod 700 "$HOME/.ssh"
    chmod 600 "$KNOWN_HOSTS"
fi

echo "[3] Removing old host key for $TARGET_IP if exists..."
ssh-keygen -f "$KNOWN_HOSTS" -R "$TARGET_IP" 2>/dev/null || true

echo "[4] Adding new host key for $TARGET_IP..."
ssh-keyscan -H "$TARGET_IP" >> "$KNOWN_HOSTS" 2>/dev/null || true

echo "[5] Checking SSH key authentication..."
SSH_KEY="$HOME/.ssh/id_rsa"
if [ ! -f "$SSH_KEY" ]; then
    echo "[5a] Generating SSH key pair..."
    ssh-keygen -t rsa -b 2048 -f "$SSH_KEY" -N ""
    echo "SSH key generated at $SSH_KEY"
fi

echo "[5b] Testing SSH key authentication..."
if ssh -o BatchMode=yes -o ConnectTimeout=5 $TARGET_USER@$TARGET_IP exit 2>/dev/null; then
    echo "SSH key authentication successful!"
    echo "[6] Connecting to $TARGET_USER@$TARGET_IP via SSH (key-based)..."
    ssh $TARGET_USER@$TARGET_IP
else
    echo "SSH key authentication failed. First-time setup required."
    echo "[5c] Setting up SSH key authentication..."
    echo ""
    echo "FIRST TIME SETUP: You'll need to enter the password once to copy the SSH key."
    echo "After this setup, future connections will be password-free."
    echo ""
    echo "Your SSH public key:"
    cat "$SSH_KEY.pub"
    echo ""
    echo "Attempting to copy SSH key to target (password required)..."

    # Try ssh-copy-id first (most reliable)
    if ssh-copy-id -o ConnectTimeout=10 $TARGET_USER@$TARGET_IP 2>/dev/null; then
        echo "SSH key setup successful!"
    else
        echo ""
        echo "Automatic key setup failed. Manual setup required:"
        echo "1. Copy the public key above"
        echo "2. SSH to target: ssh $TARGET_USER@$TARGET_IP"
        echo "3. Run: mkdir -p ~/.ssh && echo 'PASTE_KEY_HERE' >> ~/.ssh/authorized_keys"
        echo "4. Run: chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys"
        echo ""
        echo "Or run this command manually: ssh-copy-id $TARGET_USER@$TARGET_IP"
        echo ""
        echo "Press Enter after completing the setup..."
        read
    fi

    echo "[6] Testing connection with SSH key..."
    if ssh -o BatchMode=yes -o ConnectTimeout=5 $TARGET_USER@$TARGET_IP exit 2>/dev/null; then
        echo "SSH key authentication working! Connecting..."
        ssh $TARGET_USER@$TARGET_IP
    else
        echo "SSH key setup incomplete. Connecting with password..."
        ssh $TARGET_USER@$TARGET_IP
    fi
fi
