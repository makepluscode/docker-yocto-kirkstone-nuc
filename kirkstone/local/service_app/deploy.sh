#!/usr/bin/env bash
# Deploy service_app binary to target device
# Usage: ./deploy.sh [user@]TARGET_IP

set -e
TARGET=${1:-root@192.168.1.100}

# Find built binary
BIN_PATH="build/service_app"
if [[ ! -f "$BIN_PATH" ]]; then
  echo "❌ service_app binary not found. Build it first."
  exit 1
fi

echo "📦 Found binary: $BIN_PATH"
echo "🚀 Copying to $TARGET ..."

scp "$BIN_PATH" "$TARGET:/usr/local/bin/service_app.new"

ssh "$TARGET" <<'EOF'
systemctl stop service_app.service || true
mv /usr/local/bin/service_app.new /usr/local/bin/service_app
chmod +x /usr/local/bin/service_app
systemctl start service_app.service || true
EOF

echo "✅ Deployment done."