#!/bin/bash

# Install Dependencies for Updater Application

set -e

echo "=== Installing Updater Dependencies ==="

# Check for Python
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3 is required but not installed"
    exit 1
fi

echo "✓ Python3 found: $(python3 --version)"

# Install system dependencies (Ubuntu/Debian)
if command -v apt &> /dev/null; then
    echo "📦 Installing system dependencies..."
    sudo apt update
    sudo apt install -y python3-pyqt6 python3-requests python3-pip
    echo "✓ System dependencies installed"
fi

# Try to install with uv if available
if command -v uv &> /dev/null; then
    echo "📦 Installing Python dependencies with uv..."
    uv sync
    echo "✓ Dependencies installed with uv"
else
    echo "📦 Installing Python dependencies with pip..."
    pip3 install -r requirements.txt
    echo "✓ Dependencies installed with pip"
fi

# Test PyQt6 installation
echo "🧪 Testing PyQt6 installation..."
if python3 -c "import PyQt6; print('PyQt6 version:', PyQt6.QtCore.qVersion())"; then
    echo "✅ PyQt6 is working correctly"
else
    echo "❌ PyQt6 installation failed"
    echo "💡 Try: sudo apt install python3-pyqt6"
    exit 1
fi

echo "🎉 All dependencies installed successfully!"
echo ""
echo "You can now run the updater with:"
echo "  ./updater.py"
echo "  or"
echo "  python3 updater.py"