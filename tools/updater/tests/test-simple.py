#!/usr/bin/env python3
"""Simple QML test."""

import sys
from pathlib import Path

# Add src to path for testing
sys.path.insert(0, str(Path(__file__).parent / "src"))

from PyQt6.QtGui import QGuiApplication
from PyQt6.QtQml import qmlRegisterType
from PyQt6.QtQuick import QQuickView
from PyQt6.QtCore import QUrl

def main():
    app = QGuiApplication(sys.argv)
    
    # Create simple QML string content
    qml_content = """
import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    visible: true
    width: 400
    height: 300
    title: "RAUC Updater Test"
    
    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"
        
        Text {
            anchors.centerIn: parent
            text: "Hello RAUC Updater!"
            font.pixelSize: 24
        }
    }
}
"""
    
    # Create view and set QML content
    view = QQuickView()
    
    # Try to load from string
    from PyQt6.QtQml import qmlRegisterType
    
    # Simple test without external files
    qml_file = Path(__file__).parent / "src" / "rauc_updater" / "gui" / "qml" / "main.qml"
    print(f"Loading QML from: {qml_file}")
    print(f"QML file exists: {qml_file.exists()}")
    
    if qml_file.exists():
        view.setSource(QUrl.fromLocalFile(str(qml_file)))
    else:
        print("QML file not found, using fallback")
        sys.exit(1)
    
    view.show()
    
    return app.exec()

if __name__ == "__main__":
    main()