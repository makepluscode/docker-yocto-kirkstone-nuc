#!/usr/bin/env bash
# Deploy latest dashboard binary and service files to a target device via SSH.
# Usage: ./deploy_dashboard.sh [user@]TARGET_IP
# Defaults: TARGET_IP=192.168.1.100  USER=root

set -e
TARGET=${1:-root@192.168.1.100}

BUILD_DIR="$(dirname $(readlink -f $0))/kirkstone/build"

# Find latest compiled dashboard binary inside work directory
BIN_PATH=$(find "$BUILD_DIR/tmp-glibc/work" -type f -path "*dashboard*/image/usr/bin/dashboard" | sort | tail -n1)
if [[ -z "$BIN_PATH" ]]; then
  echo "❌ dashboard binary not found. Build it first with: bitbake dashboard"
  exit 1
fi

echo "📦 Found binary: $BIN_PATH"

SERVICE_DIR="$(dirname $(readlink -f $0))/kirkstone/meta-qt5-app/recipes-qt5/system-dashboard/files"
SERVICE_EGLFS="$SERVICE_DIR/dashboard-eglfs.service"

if [[ ! -f "$SERVICE_EGLFS" ]]; then
  echo "❌ Service file not found: $SERVICE_EGLFS"
  exit 1
fi

echo "🚀 Copying files to $TARGET ..."
scp "$BIN_PATH" "$TARGET:/usr/bin/dashboard.new"
scp "$SERVICE_EGLFS" "$TARGET:/lib/systemd/system/dashboard-eglfs.service"

ssh "$TARGET" <<'EOF'
set -e
systemctl stop dashboard-eglfs.service || true
mv /usr/bin/dashboard.new /usr/bin/dashboard
chmod +x /usr/bin/dashboard
systemctl daemon-reload
systemctl enable dashboard-eglfs.service
systemctl start dashboard-eglfs.service
EOF

echo "✅ Deployment done. Check status via: ssh $TARGET 'systemctl status dashboard-eglfs.service --no-pager'" 