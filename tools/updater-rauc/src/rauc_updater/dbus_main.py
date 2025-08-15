"""D-Bus only GUI application for RAUC Updater."""

import sys
from pathlib import Path
from PyQt6.QtGui import QGuiApplication
from PyQt6.QtQml import qmlRegisterType, QQmlApplicationEngine
from PyQt6.QtCore import QUrl

from .dbus_controller import DBusRaucController


def main():
    """Main entry point for D-Bus only GUI application."""
    app = QGuiApplication(sys.argv)
    app.setApplicationName("ARCRO D-Bus")
    app.setApplicationVersion("1.0.0")
    app.setOrganizationName("ARCRO Tools")
    
    # Register custom types
    qmlRegisterType(DBusRaucController, "RaucUpdater", 1, 0, "RaucController")
    
    # Create QML engine
    engine = QQmlApplicationEngine()
    
    # Set context properties
    controller = DBusRaucController()
    engine.rootContext().setContextProperty("raucController", controller)
    
    # Load QML file
    qml_file = Path(__file__).parent / "qml" / "main.qml"
    engine.load(QUrl.fromLocalFile(str(qml_file)))
    
    if not engine.rootObjects():
        return -1
    
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())