#!/usr/bin/env bash
# Deploy update-service binary and configuration to target device
# Usage: ./deploy.sh [user@]TARGET_IP

set -e
TARGET=${1:-root@192.168.1.100}

# Find built binary
BIN_PATH="build/update-service"
if [[ ! -f "$BIN_PATH" ]]; then
  echo "❌ update-service binary not found. Build it first."
  exit 1
fi

echo "📦 Found binary: $BIN_PATH"
echo "🚀 Copying to $TARGET ..."

# Copy binary
scp "$BIN_PATH" "$TARGET:/usr/local/bin/update-service.new"

# Copy service files
scp "services/update-service.service" "$TARGET:/tmp/"
scp "dbus-service/org.freedesktop.UpdateService.conf" "$TARGET:/tmp/"
scp "dbus-service/org.freedesktop.UpdateService.service" "$TARGET:/tmp/"

ssh "$TARGET" <<'EOF'
# Stop existing service if running
systemctl stop update-service 2>/dev/null || true

# Install binary
mv /usr/local/bin/update-service.new /usr/local/bin/update-service
chmod +x /usr/local/bin/update-service

# Install systemd service
mv /tmp/update-service.service /etc/systemd/system/
chmod 644 /etc/systemd/system/update-service.service

# Install D-Bus configuration
mv /tmp/org.freedesktop.UpdateService.conf /etc/dbus-1/system.d/
chmod 644 /etc/dbus-1/system.d/org.freedesktop.UpdateService.conf

mv /tmp/org.freedesktop.UpdateService.service /usr/share/dbus-1/system-services/
chmod 644 /usr/share/dbus-1/system-services/org.freedesktop.UpdateService.service

# Reload systemd and D-Bus
systemctl daemon-reload
systemctl reload dbus 2>/dev/null || systemctl restart dbus

# Enable and start service
systemctl enable update-service
systemctl start update-service

echo "Update Service installed and started"
systemctl status update-service --no-pager
EOF

echo "✅ Deployment done."
