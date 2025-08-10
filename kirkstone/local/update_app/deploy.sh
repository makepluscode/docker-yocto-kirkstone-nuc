#!/usr/bin/env bash
# Deploy update_app binary to target device
# Usage: ./deploy.sh [user@]TARGET_IP

set -e
TARGET=${1:-root@192.168.1.100}

# Find built binary
BIN_PATH="build/update_app"
if [[ ! -f "$BIN_PATH" ]]; then
  echo "❌ update_app binary not found. Build it first."
  exit 1
fi

echo "📦 Found binary: $BIN_PATH"
echo "🚀 Copying to $TARGET ..."

scp "$BIN_PATH" "$TARGET:/usr/local/bin/update_app.new"

ssh "$TARGET" <<'EOF'
mv /usr/local/bin/update_app.new /usr/local/bin/update_app
chmod +x /usr/local/bin/update_app
EOF

echo "✅ Deployment done."