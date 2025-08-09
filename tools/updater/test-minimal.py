#!/usr/bin/env python3
"""Minimal QML test to verify functionality."""

import sys
from pathlib import Path

# Add src to path for testing
sys.path.insert(0, str(Path(__file__).parent / "src"))

from PyQt6.QtGui import QGuiApplication
from PyQt6.QtQml import QQmlApplicationEngine
from PyQt6.QtCore import QUrl

def main():
    app = QGuiApplication(sys.argv)
    app.setApplicationName("RAUC Updater Test")
    
    engine = QQmlApplicationEngine()
    
    # Create a minimal QML file for testing
    minimal_qml = Path(__file__).parent / "test-minimal.qml"
    minimal_qml.write_text("""
import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    visible: true
    width: 600
    height: 400
    title: "RAUC Updater - Minimal Test"
    
    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"
        
        Column {
            anchors.centerIn: parent
            spacing: 20
            
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "RAUC Updater GUI Test"
                font.pixelSize: 24
                color: "#333333"
            }
            
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Test Button"
                onClicked: console.log("Button clicked!")
            }
        }
    }
}
""")
    
    engine.load(QUrl.fromLocalFile(str(minimal_qml)))
    
    if not engine.rootObjects():
        print("Failed to load QML")
        return 1
        
    print("QML loaded successfully")
    return app.exec()

if __name__ == "__main__":
    sys.exit(main())