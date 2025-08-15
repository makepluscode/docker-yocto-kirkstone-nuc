#!/bin/bash

# Updater Server Startup Script

echo "=== Updater Server Startup ==="

# Kill existing server processes
echo "Checking for existing server processes..."
PIDS=$(pgrep -f "uvicorn.*updater_server" || true)
if [ -n "$PIDS" ]; then
    echo "Killing existing server processes: $PIDS"
    pkill -f "uvicorn.*updater_server" || true
    sleep 2
    # Force kill if still running
    PIDS=$(pgrep -f "uvicorn.*updater_server" || true)
    if [ -n "$PIDS" ]; then
        echo "Force killing stubborn processes: $PIDS"
        pkill -9 -f "uvicorn.*updater_server" || true
    fi
else
    echo "No existing server processes found"
fi

# Check if port 8080 is still in use
if lsof -i :8080 > /dev/null 2>&1; then
    echo "Warning: Port 8080 is still in use. Attempting to free it..."
    lsof -ti:8080 | xargs kill -9 > /dev/null 2>&1 || true
    sleep 1
fi

# Check if uv is available
if ! command -v uv &> /dev/null; then
    echo "Error: uv is not installed. Please install uv first."
    echo "Visit: https://docs.astral.sh/uv/getting-started/installation/"
    exit 1
fi

# Check if bundle directory exists
if [ ! -d "bundle" ]; then
    echo "Warning: bundle directory not found. Creating it..."
    mkdir -p bundle
fi

# Check if there are any .raucb files in the bundle directory
if [ ! -f bundle/*.raucb ]; then
    echo "Warning: No .raucb bundle files found in bundle directory."
    echo "Please copy .raucb files to the bundle/ directory for deployments."
    echo "The server will start but won't have any deployments available."
else
    echo "Found bundle files:"
    ls -la bundle/*.raucb
fi

# Install dependencies if needed
echo "Installing/updating dependencies..."
uv sync

# Start the server
echo ""
echo "=== Starting Updater Server ==="
echo "Server URL: http://0.0.0.0:8080"
echo "Admin API: http://localhost:8080/admin/deployments"
echo "Press Ctrl+C to stop the server"
echo "==================================="
echo ""

# Determine port and protocol based on HTTPS setting
if [ "${UPDATER_ENABLE_HTTPS:-false}" = "true" ]; then
    PORT=${UPDATER_HTTPS_PORT:-8443}
    if [ -f "certs/server.crt" ] && [ -f "certs/server.key" ]; then
        echo "🔒 HTTPS/TLS mode enabled on port $PORT"
        exec uv run python -m uvicorn updater_server.main:app --host 0.0.0.0 --port $PORT --ssl-keyfile certs/server.key --ssl-certfile certs/server.crt --reload
    else
        echo "⚠️  HTTPS requested but certificates not found. Falling back to HTTP."
        PORT=${UPDATER_PORT:-8080}
        exec uv run python -m uvicorn updater_server.main:app --host 0.0.0.0 --port $PORT --reload
    fi
else
    PORT=${UPDATER_PORT:-8080}
    echo "🌐 HTTP mode on port $PORT"
    exec uv run python -m uvicorn updater_server.main:app --host 0.0.0.0 --port $PORT --reload
fi 