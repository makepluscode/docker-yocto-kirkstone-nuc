#!/usr/bin/env python3
"""
QML-based GUI Application for Updater
Uses QML for declarative UI design and better maintainability
"""

import sys
import os
from pathlib import Path
from PyQt6.QtCore import QObject, pyqtSignal, QCoreApplication
from PyQt6.QtGui import QGuiApplication
from PyQt6.QtQml import QQmlApplicationEngine
from PyQt6.QtQuick import QQuickView

# Import backend managers
from .backend import ServerManager, DeploymentManager, LogManager


class UpdaterGUI(QObject):
    """Main GUI application using QML."""
    
    def __init__(self):
        super().__init__()
        self.app = None
        self.engine = None
        self.backend_managers = {}
        
    def run(self):
        """Run the QML application."""
        # Create Qt application
        self.app = QGuiApplication(sys.argv)
        self.app.setApplicationName("Updater")
        self.app.setApplicationVersion("1.0.0")
        
        # Create QML engine
        self.engine = QQmlApplicationEngine()
        
        # Register backend managers
        self._register_backend_managers()
        
        # Load main QML file
        qml_file = Path(__file__).parent / "qml" / "main.qml"
        if not qml_file.exists():
            print(f"❌ QML file not found: {qml_file}")
            return False
        
        self.engine.load(str(qml_file))
        
        # Check if QML loaded successfully
        if not self.engine.rootObjects():
            print("❌ Failed to load QML")
            return False
        
        # Run the application
        return self.app.exec()
    
    def _register_backend_managers(self):
        """Register Python backend objects with QML."""
        # Create backend managers
        self.backend_managers['serverManager'] = ServerManager()
        self.backend_managers['deploymentManager'] = DeploymentManager()
        self.backend_managers['logManager'] = LogManager()
        
        # Register with QML engine
        for name, manager in self.backend_managers.items():
            self.engine.rootContext().setContextProperty(name, manager)
            
        # Connect signals for logging
        self.backend_managers['serverManager'].serverOutput.connect(
            lambda msg: self.backend_managers['logManager'].addLog(msg)
        )
        self.backend_managers['deploymentManager'].deploymentError.connect(
            lambda msg: self.backend_managers['logManager'].addLog(f"❌ {msg}")
        )
        self.backend_managers['deploymentManager'].deploymentAdded.connect(
            lambda name: self.backend_managers['logManager'].addLog(f"✅ Added deployment: {name}")
        )
        self.backend_managers['deploymentManager'].deploymentToggled.connect(
            lambda id, active: self.backend_managers['logManager'].addLog(
                f"🔄 Deployment {id} {'enabled' if active else 'disabled'}"
            )
        )
        
        # Connect server status signals
        self.backend_managers['serverManager'].serverStarted.connect(
            lambda: self._update_server_status(True)
        )
        self.backend_managers['serverManager'].serverStopped.connect(
            lambda: self._update_server_status(False)
        )
        
        # Connect bundle scanning signals
        self.backend_managers['deploymentManager'].bundlesScanned.connect(
            lambda bundles: self._update_bundles(bundles)
        )
        
        # Connect log manager signals
        self.backend_managers['logManager'].logsUpdated.connect(
            lambda logs: self._update_logs(logs)
        )
        
        # Start automatic bundle scanning
        self._start_auto_refresh()
    
    def _update_server_status(self, running):
        """Update server status in QML."""
        if self.engine and self.engine.rootObjects():
            root = self.engine.rootObjects()[0]
            root.setProperty("serverRunning", running)
            root.setProperty("serverStatus", "Running" if running else "Stopped")
    
    def _update_bundles(self, bundles):
        """Update bundles list in QML."""
        if self.engine and self.engine.rootObjects():
            root = self.engine.rootObjects()[0]
            root.setProperty("bundles", bundles)
            if bundles:
                root.setProperty("latestBundle", bundles[0])
    
    def _update_logs(self, logs):
        """Update logs in QML."""
        if self.engine and self.engine.rootObjects():
            root = self.engine.rootObjects()[0]
            root.setProperty("logs", logs)
    
    def _start_auto_refresh(self):
        """Start automatic bundle refresh."""
        # Initial scan
        self.backend_managers['deploymentManager'].scanBundles()
        
        # Set up timer for periodic refresh (every 5 seconds)
        from PyQt6.QtCore import QTimer
        self.auto_refresh_timer = QTimer()
        self.auto_refresh_timer.timeout.connect(
            lambda: self.backend_managers['deploymentManager'].scanBundles()
        )
        self.auto_refresh_timer.start(5000)  # 5 seconds


def main():
    """Main entry point for QML GUI."""
    gui = UpdaterGUI()
    return gui.run()


if __name__ == "__main__":
    sys.exit(main())