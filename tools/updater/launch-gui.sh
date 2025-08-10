#!/bin/bash
set -e

echo "🚀 Launching ARCRO GUI..."
echo "📍 Working directory: $(pwd)"

# Check if we have the GUI components
if [ ! -f "src/rauc_updater/gui/main.py" ]; then
    echo "❌ GUI main.py not found"
    exit 1
fi

if [ ! -f "src/rauc_updater/gui/qml/main.qml" ]; then
    echo "❌ Main QML file not found"
    exit 1
fi

echo "✅ GUI components found"
echo "🎯 Launching with uv run..."

uv run python3 -c "
import sys
from pathlib import Path
sys.path.insert(0, 'src')
from rauc_updater.gui.main import main
print('🎮 Starting ARCRO GUI...')
main()
"