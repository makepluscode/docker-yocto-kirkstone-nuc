#!/bin/bash

# 네트워크 설정
IFACE="eno1"
HOST_IP="192.168.1.101"
NETMASK="255.255.255.0"

# 타겟 정보
TARGET_IP="192.168.1.100"
TARGET_USER="root"

set -e

echo "[1] Setting IP address for $IFACE..."
sudo ifconfig $IFACE $HOST_IP netmask $NETMASK up

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

echo "[5] Connecting to $TARGET_USER@$TARGET_IP via SSH..."
ssh $TARGET_USER@$TARGET_IP
