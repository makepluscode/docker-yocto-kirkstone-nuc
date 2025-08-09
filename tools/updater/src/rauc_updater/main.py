"""Main GUI application for RAUC Updater."""

import sys
from pathlib import Path
from PyQt6.QtGui import QGuiApplication
from PyQt6.QtQml import qmlRegisterType, QQmlApplicationEngine
from PyQt6.QtCore import QUrl

from .controller import RaucController


def main():
    """Main entry point for GUI application."""
    app = QGuiApplication(sys.argv)
    app.setApplicationName("ARCRO")
    app.setApplicationVersion("1.0.0")
    app.setOrganizationName("ARCRO Tools")
    
    # Register custom types
    qmlRegisterType(RaucController, "RaucUpdater", 1, 0, "RaucController")
    
    # Create QML engine
    engine = QQmlApplicationEngine()
    
    # Set context properties
    controller = RaucController()
    engine.rootContext().setContextProperty("raucController", controller)
    
    # Load QML file
    qml_file = Path(__file__).parent / "qml" / "main.qml"
    engine.load(QUrl.fromLocalFile(str(qml_file)))
    
    # Check if QML loaded successfully
    if not engine.rootObjects():
        print("Failed to load QML file")
        sys.exit(1)
    
    # Run application
    sys.exit(app.exec())


if __name__ == "__main__":
    main()