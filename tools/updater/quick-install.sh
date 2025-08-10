#!/bin/bash
set -e

echo "🚀 Quick Ubuntu Installation for RAUC Updater"

# Install system dependencies
echo "📦 Installing system dependencies..."
sudo apt update
sudo apt install -y python3-pip python3-venv python3-pyqt6 python3-pyqt6.qtquick sshpass

# Install Python package manager
if ! command -v uv &> /dev/null; then
    echo "📥 Installing uv package manager..."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    source ~/.bashrc
fi

# Install package locally
echo "📦 Installing RAUC Updater..."
uv sync

echo "✅ Installation complete!"
echo ""
echo "To run the GUI application:"
echo "  uv run python3 -c 'import sys; sys.path.insert(0, \"src\"); from rauc_updater.gui.main import main; main()'"
echo ""
echo "To run CLI:"  
echo "  uv run python3 -c 'import sys; sys.path.insert(0, \"src\"); from rauc_updater.cli import main; main()' --help"