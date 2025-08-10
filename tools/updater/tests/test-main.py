#!/usr/bin/env python3
"""Test the main RAUC Updater GUI with debugging."""

import sys
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).parent / "src"))

from PyQt6.QtGui import QGuiApplication
from PyQt6.QtQml import qmlRegisterType, QQmlApplicationEngine
from PyQt6.QtCore import QUrl

from rauc_updater.gui.controller import RaucController

def main():
    app = QGuiApplication(sys.argv)
    app.setApplicationName("RAUC Updater")
    
    # Register controller
    qmlRegisterType(RaucController, "RaucUpdater", 1, 0, "RaucController")
    
    engine = QQmlApplicationEngine()
    
    # Create controller and set context
    controller = RaucController()
    engine.rootContext().setContextProperty("raucController", controller)
    
    # Load main QML
    qml_file = Path(__file__).parent / "src" / "rauc_updater" / "gui" / "qml" / "main.qml"
    print(f"Loading QML: {qml_file}")
    print(f"QML exists: {qml_file.exists()}")
    
    engine.load(QUrl.fromLocalFile(str(qml_file)))
    
    if not engine.rootObjects():
        print("❌ Failed to load QML file")
        return 1
    else:
        print("✅ QML loaded successfully")
    
    return app.exec()

if __name__ == "__main__":
    sys.exit(main())