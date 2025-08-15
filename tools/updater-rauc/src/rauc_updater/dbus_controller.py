"""D-Bus only RAUC Controller for GUI."""

import subprocess
from pathlib import Path
from typing import Optional
from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot, pyqtProperty

from .config import DEFAULT_HOST, DEFAULT_USER, DEFAULT_PORT, DBUS_DEST, DBUS_PATH, DBUS_INTERFACE
from .utils import validate_bundle_file


class DBusRaucController(QObject):
    """D-Bus only controller for RAUC GUI operations."""
    
    # Signals
    connectionStatusChanged = pyqtSignal(bool)
    progressChanged = pyqtSignal(int)
    statusChanged = pyqtSignal(str)
    logMessage = pyqtSignal(str)
    updateCompleted = pyqtSignal(bool, str, dict)
    
    def __init__(self):
        super().__init__()
        self._bundle_path = ""
        self._is_connected = False
        self._is_updating = False
        self._progress = 0
        self._timing_info = {}
        
        # Initialize progress to 0
        self.progressChanged.emit(0)
        self.statusChanged.emit("Ready - Pure D-Bus Mode")
        self.logMessage.emit("Started in pure D-Bus only mode (no SSH)")
    
    # Properties
    @pyqtProperty(str)
    def bundlePath(self):
        return self._bundle_path
    
    @bundlePath.setter
    def bundlePath(self, value):
        self._bundle_path = value
    
    @pyqtProperty(bool)
    def isConnected(self):
        return self._is_connected
    
    @pyqtProperty(bool)
    def isUpdating(self):
        return self._is_updating
    
    @pyqtProperty(int)
    def progress(self):
        return self._progress
    
    @pyqtProperty('QVariant')
    def timingInfo(self):
        return self._timing_info
    
    @pyqtSlot(result=bool)
    def testConnection(self):
        """Test local D-Bus connection to RAUC service."""
        self.statusChanged.emit("Testing local D-Bus connection...")
        self.logMessage.emit("Testing local D-Bus connection to RAUC service")
        
        try:
            # Test local D-Bus connection to RAUC
            result = subprocess.run([
                'dbus-send', '--system', '--print-reply',
                f'--dest={DBUS_DEST}', DBUS_PATH,
                'org.freedesktop.DBus.Introspectable.Introspect'
            ], capture_output=True, timeout=10)
            
            if result.returncode == 0:
                self._is_connected = True
                self.connectionStatusChanged.emit(True)
                self.statusChanged.emit("D-Bus connection successful")
                self.logMessage.emit("✓ Local D-Bus connection to RAUC service established")
                return True
            else:
                self._is_connected = False
                self.connectionStatusChanged.emit(False)
                self.statusChanged.emit("D-Bus service not available")
                self.logMessage.emit("✗ RAUC D-Bus service not available locally")
                return False
                
        except Exception as e:
            self._is_connected = False
            self.connectionStatusChanged.emit(False)
            self.statusChanged.emit(f"D-Bus error: {str(e)}")
            self.logMessage.emit(f"✗ D-Bus error: {str(e)}")
            return False
    
    @pyqtSlot(result=str)
    def selectBundle(self):
        """Open file dialog to select RAUC bundle."""
        from PyQt6.QtWidgets import QFileDialog
        
        file_path, _ = QFileDialog.getOpenFileName(
            None,
            "Select RAUC Bundle",
            "",
            "RAUC Bundles (*.raucb);;All Files (*)"
        )
        
        if file_path:
            bundle = validate_bundle_file(file_path)
            if bundle:
                self._bundle_path = file_path
                self.logMessage.emit(f"Selected bundle: {file_path}")
                return file_path
            else:
                self.logMessage.emit("Invalid bundle file selected")
                return ""
        
        return ""
    
    @pyqtSlot()
    def startUpdate(self):
        """Start pure D-Bus RAUC update process."""
        if not self._bundle_path:
            self.statusChanged.emit("No bundle selected")
            self.logMessage.emit("✗ No bundle selected")
            return
        
        if not Path(self._bundle_path).exists():
            self.statusChanged.emit("Bundle file does not exist")
            self.logMessage.emit("✗ Bundle file does not exist")
            return
        
        if not self._is_connected:
            self.statusChanged.emit("Not connected to D-Bus service")
            self.logMessage.emit("✗ Not connected to D-Bus service")
            return
        
        self._is_updating = True
        self._progress = 0
        self.progressChanged.emit(0)
        self.statusChanged.emit("Starting D-Bus update...")
        self.logMessage.emit("Starting pure D-Bus RAUC update process")
        
        try:
            import time
            start_time = time.time()
            
            # Pure D-Bus approach - install bundle directly
            self.statusChanged.emit("Installing via D-Bus...")
            self.progressChanged.emit(25)
            self.logMessage.emit(f"Installing bundle {self._bundle_path} via D-Bus...")
            
            # Use D-Bus to install the bundle directly from local path
            dbus_cmd = [
                'dbus-send', '--system', '--print-reply',
                f'--dest={DBUS_DEST}', DBUS_PATH,
                f'{DBUS_INTERFACE}.Install',
                f'string:"{self._bundle_path}"'
            ]
            
            self.logMessage.emit(f"D-Bus command: {' '.join(dbus_cmd)}")
            result = subprocess.run(dbus_cmd, capture_output=True, timeout=300)
            
            self.progressChanged.emit(75)
            
            if result.returncode == 0:
                self.progressChanged.emit(95)
                self.logMessage.emit("✓ D-Bus installation completed")
                
                # Check installation status
                self.statusChanged.emit("Verifying installation...")
                status_cmd = [
                    'dbus-send', '--system', '--print-reply',
                    f'--dest={DBUS_DEST}', DBUS_PATH,
                    f'{DBUS_INTERFACE}.GetSlotStatus'
                ]
                
                status_result = subprocess.run(status_cmd, capture_output=True, timeout=10)
                if status_result.returncode == 0:
                    self.logMessage.emit("✓ Installation status verified")
                
                total_time = time.time() - start_time
                self._timing_info = {'total_time': total_time}
                self._finish_update(True, "Pure D-Bus update completed successfully")
            else:
                error_msg = result.stderr.decode() if result.stderr else "Unknown D-Bus error"
                self.logMessage.emit(f"D-Bus output: {result.stdout.decode()}")
                self.logMessage.emit(f"D-Bus error: {error_msg}")
                self._finish_update(False, f"D-Bus installation failed: {error_msg}")
                
        except subprocess.TimeoutExpired:
            self._finish_update(False, "Update operation timed out")
        except Exception as e:
            self._finish_update(False, f"Update failed: {str(e)}")
    
    def _finish_update(self, success: bool, message: str):
        """Finish the update process."""
        self._is_updating = False
        self._progress = 100 if success else 0
        self.progressChanged.emit(self._progress)
        
        if success:
            self.statusChanged.emit("Update completed")
            self.logMessage.emit("✓ " + message)
        else:
            self.statusChanged.emit("Update failed")
            self.logMessage.emit("✗ " + message)
        
        self.updateCompleted.emit(success, message, self._timing_info)
    
    @pyqtSlot()
    def resetState(self):
        """Reset controller to initial state."""
        self._bundle_path = ""
        self._is_updating = False
        self._progress = 0
        self._timing_info = {}
        self.progressChanged.emit(0)
        self.statusChanged.emit("Ready - D-Bus Mode")
        self.logMessage.emit("State reset")