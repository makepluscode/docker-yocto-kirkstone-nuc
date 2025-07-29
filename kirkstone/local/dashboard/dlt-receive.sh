#!/usr/bin/env bash
# dlt-receive.sh – Dashboard DLT log receiver
# Usage:
#   ./dlt-receive.sh [NUC_IP] [PORT] [EXTRA_DLT_OPTIONS]
# Example:
#   ./dlt-receive.sh                              # use default IP 192.168.1.100, port 3490
#   ./dlt-receive.sh 192.168.1.100               # custom IP, default port 3490
#   ./dlt-receive.sh 192.168.1.100 3491 -e DBO   # custom IP + port + filter for dashboard app
#   ./dlt-receive.sh 192.168.1.100 3490 -e RUC   # filter for RAUC manager logs
#
set -e

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
  sed -n '2,14p' "$0"
  exit 0
fi

# Default IP address
DEFAULT_IP="192.168.1.100"

if [[ -z "$1" ]]; then
  # Use default IP if no argument provided
  NUC_IP="$DEFAULT_IP"
  PORT="3490"
  echo "📡 Using default IP: $NUC_IP"
else
  NUC_IP="$1"
  PORT="${2:-3490}"
  shift 2 || true
fi

# Verify dlt-receive exists
if ! command -v dlt-receive >/dev/null 2>&1; then
  echo "❌ dlt-receive not found. Install it (e.g., sudo apt install dlt-daemon)" >&2
  exit 2
fi

echo "📡 Connecting to $NUC_IP:$PORT for Dashboard DLT logs..."
echo "💡 Dashboard app ID: DBO (Dashboard Application)"
echo "💡 RAUC manager context: RUC"
echo "💡 UI flow context: UIF"
echo "💡 Use -e DBO to filter dashboard logs only"
echo ""

# Use ASCII payload output (-a) by default. dlt-receive requires the host argument LAST.
exec dlt-receive -a -p "$PORT" "$@" "$NUC_IP" 