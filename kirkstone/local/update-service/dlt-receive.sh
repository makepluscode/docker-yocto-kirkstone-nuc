#!/usr/bin/env bash
# Simple DLT receiver for update_app logs
# Usage: ./dlt-receive.sh [TARGET_IP]

set -e
TARGET_IP=${1:-192.168.1.100}

if ! command -v dlt-receive >/dev/null 2>&1; then
  echo "❌ dlt-receive not found"
  exit 1
fi

echo "📡 Connecting to $TARGET_IP:3490 ..."
exec dlt-receive -a "$TARGET_IP"