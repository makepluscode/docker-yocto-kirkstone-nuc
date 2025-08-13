#!/bin/bash

# Hawkbit Server Startup Script

echo "Starting Hawkbit Server..."

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
    echo "The server will start but won't have any deployments available."
fi

# Install dependencies if needed
echo "Installing dependencies..."
uv sync

# Start the server
echo "Starting server on http://localhost:8080"
echo "Press Ctrl+C to stop the server"
echo ""

uv run uvicorn hawkbit_server.main:app --host 0.0.0.0 --port 8080 --reload 