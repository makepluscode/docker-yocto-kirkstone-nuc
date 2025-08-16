#!/usr/bin/env python3
"""
Main Entry Point for Updater Application
Supports both GUI and CLI modes
"""

import sys
import os
import argparse
from pathlib import Path

def main():
    """Main entry point with mode selection."""
    parser = argparse.ArgumentParser(description="Updater Application")
    parser.add_argument(
        "mode", 
        nargs="?", 
        default="gui",
        choices=["gui", "cli"],
        help="Run mode: gui (default) or cli"
    )
    
    args = parser.parse_args()
    
    # Add the project root to Python path
    project_root = Path(__file__).parent
    sys.path.insert(0, str(project_root))
    
    if args.mode == "gui":
        run_gui_mode()
    else:
        run_cli_mode()

def run_gui_mode():
    """Run the application in GUI mode."""
    try:
        # Import GUI components
        from gui.app import UpdaterGUI
        
        # Create and run QML application
        gui = UpdaterGUI()
        sys.exit(gui.run())
        
    except ImportError as e:
        print(f"❌ GUI mode requires PyQt6: {e}")
        print("💡 Install with: pip install PyQt6")
        print("💡 Or run in CLI mode: uv run main.py cli")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Failed to start GUI: {e}")
        sys.exit(1)

def run_cli_mode():
    """Run the application in CLI mode."""
    try:
        # Import server components
        import asyncio
        from server.main import main as server_main
        
        print("🚀 Starting Updater in CLI mode...")
        print("📡 Server will be available at: http://localhost:8080")
        print("⚙️  Admin interface: http://localhost:8080/admin/deployments")
        print("💡 Press Ctrl+C to stop the server")
        print()
        
        # Start the server
        asyncio.run(server_main())
        
    except ImportError as e:
        print(f"❌ Failed to import server components: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n🛑 Server stopped by user")
    except Exception as e:
        print(f"❌ Server error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 