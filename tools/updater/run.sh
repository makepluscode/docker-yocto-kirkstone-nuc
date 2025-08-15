#!/bin/bash

# Easy Run Script for Updater
# Automatically chooses the best available GUI option

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "🚀 Starting Updater..."

# Function to check if PyQt6 is available
check_pyqt6() {
    python3 -c "import PyQt6" 2>/dev/null
}

# Function to start server in background
start_server() {
    echo "📡 Starting server..."
    ./server.sh &
    SERVER_PID=$!
    sleep 3
    
    # Check if server is running
    if curl -s http://localhost:8080/ >/dev/null 2>&1; then
        echo "✅ Server started successfully"
        return 0
    else
        echo "❌ Server failed to start"
        return 1
    fi
}

# Function to stop server
stop_server() {
    echo "🛑 Stopping server..."
    if [ ! -z "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
    fi
    # Also kill any other server processes
    pkill -f "uvicorn.*updater_server" 2>/dev/null || true
    echo "✅ Server stopped"
}

# Trap to ensure server is stopped on exit
trap stop_server EXIT

# Check available GUI options
if check_pyqt6; then
    echo "🐍 Using Python GUI with integrated server management"
    ./updater.py
else
    echo "❌ PyQt6 not available. Options:"
    echo "   1. Install PyQt6: sudo apt install python3-pyqt6"
    echo "   2. Install deps: ./scripts/install_deps.sh"
    echo "   3. Run server only: ./server.sh"
    echo ""
    echo "🌐 Starting server-only mode..."
    
    if start_server; then
        echo ""
        echo "✅ Server running at:"
        echo "   🌐 Main: http://localhost:8080"
        echo "   ⚙️  Admin: http://localhost:8080/admin/deployments"
        echo ""
        echo "💡 Install PyQt6 for GUI: sudo apt install python3-pyqt6"
        echo "Press Ctrl+C to stop the server"
        
        # Keep server running
        wait $SERVER_PID
    else
        echo "❌ Failed to start server"
        exit 1
    fi
fi