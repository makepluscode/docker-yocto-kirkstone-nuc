#!/usr/bin/env python3
"""Test script for RAUC Updater GUI."""

import sys
import os
from pathlib import Path

# Add src to path for testing
sys.path.insert(0, str(Path(__file__).parent / "src"))

try:
    from PyQt6.QtCore import QCoreApplication
    print("✅ PyQt6 available")
except ImportError:
    print("❌ PyQt6 not available - install with: pip install PyQt6")
    sys.exit(1)

try:
    from rauc_updater.gui.main import main
    print("✅ GUI module imports successfully")
except ImportError as e:
    print(f"❌ GUI module import failed: {e}")
    sys.exit(1)

try:
    from rauc_updater.core.connection import ConnectionConfig
    from rauc_updater.core.installer import InstallConfig
    print("✅ Core modules available")
except ImportError as e:
    print(f"❌ Core modules import failed: {e}")
    sys.exit(1)

print("✅ All imports successful!")
print("🚀 Starting GUI application...")

if __name__ == "__main__":
    # Test if we can start the GUI
    try:
        main()
    except Exception as e:
        print(f"❌ GUI failed to start: {e}")
        sys.exit(1)