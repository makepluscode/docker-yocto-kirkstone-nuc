"""RAUC GUI Controller - Bridge between QML and Python backend."""

import asyncio
from pathlib import Path
from typing import Optional
from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot, QThread, pyqtProperty
from PyQt6.QtWidgets import QFileDialog

from rauc_updater.core.connection import ConnectionConfig, SSHConnection
from rauc_updater.core.installer import InstallConfig, install_rauc_bundle
from rauc_updater.core.transfer import upload_bundle


class UpdateWorker(QThread):
    """Worker thread for RAUC update operations."""
    
    progressChanged = pyqtSignal(int)  # Progress percentage
    statusChanged = pyqtSignal(str)    # Status message
    logMessage = pyqtSignal(str)       # Log message
    updateCompleted = pyqtSignal(bool, str, dict)  # Success, message, timing_info
    
    def __init__(self, bundle_path: str, config: ConnectionConfig):
        super().__init__()
        self.bundle_path = bundle_path
        self.config = config
        self._cancelled = False
        
        # Time tracking
        import time
        self.time = time
        self.start_time = None
        self.setup_start_time = None
        self.update_start_time = None
        self.validate_start_time = None
        
        self.setup_duration = 0
        self.update_duration = 0
        self.validate_duration = 0
    
    def cancel(self):
        """Cancel the update operation."""
        self._cancelled = True
    
    def run(self):
        """Run the update process."""
        try:
            # Start timing
            self.start_time = self.time.time()
            self.setup_start_time = self.start_time
            
            self.statusChanged.emit("Connecting to target...")
            self.progressChanged.emit(10)
            
            if self._cancelled:
                return
            
            # Test connection
            try:
                with SSHConnection(self.config) as conn:
                    if not conn.test_rauc_availability():
                        self.updateCompleted.emit(False, "RAUC not available on target")
                        return
                    
                    # End Setup phase, start Update phase
                    self.setup_duration = self.time.time() - self.setup_start_time
                    self.update_start_time = self.time.time()
                    
                    self.statusChanged.emit("Uploading bundle...")
                    self.progressChanged.emit(25)  # Start of Step 2
                    
                    if self._cancelled:
                        return
                    
                    # Upload bundle
                    upload_result = upload_bundle(
                        conn,
                        self.bundle_path,
                        remote_path="/tmp",
                        progress_callback=self._upload_progress
                    )
                    
                    if not upload_result:
                        self.updateCompleted.emit(False, "Bundle upload failed")
                        return
                    
                    if self._cancelled:
                        return
                    
                    self.statusChanged.emit("Installing bundle...")
                    self.progressChanged.emit(50)  # Installation start within Step 2
                    
                    # Install bundle
                    install_config = InstallConfig(
                        reboot_after_install=True,
                        wait_for_reboot=True,
                        reboot_timeout=120  # 2 minutes timeout
                    )
                    
                    # End Update phase, start Validation phase
                    self.update_duration = self.time.time() - self.update_start_time
                    self.validate_start_time = self.time.time()
                    
                    result = install_rauc_bundle(conn, upload_result, install_config, self._install_progress)
                    
                    if result:
                        # End Validation phase
                        self.validate_duration = self.time.time() - self.validate_start_time
                        total_duration = self.time.time() - self.start_time
                        
                        self.progressChanged.emit(100)
                        self.statusChanged.emit("Update completed successfully")
                        # Wait a bit to show 100% progress
                        self.msleep(500)
                        
                        # Create timing info
                        timing_info = {
                            'setup_time': self.setup_duration,
                            'update_time': self.update_duration,
                            'validate_time': self.validate_duration,
                            'total_time': total_duration
                        }
                        
                        self.updateCompleted.emit(True, "Update completed successfully", timing_info)
                    else:
                        self.updateCompleted.emit(False, "Installation failed", {})
                        
            except Exception as e:
                # Handle connection loss gracefully (expected during reboot)
                error_msg = str(e).lower()
                if any(keyword in error_msg for keyword in ["connection", "ssh", "broken pipe", "timeout"]):
                    # This is likely due to target rebooting, treat as success
                    total_duration = self.time.time() - self.start_time if self.start_time else 0
                    
                    timing_info = {
                        'setup_time': getattr(self, 'setup_duration', 0),
                        'update_time': getattr(self, 'update_duration', 0),
                        'validate_time': total_duration - getattr(self, 'setup_duration', 0) - getattr(self, 'update_duration', 0),
                        'total_time': total_duration
                    }
                    
                    self.progressChanged.emit(100)
                    self.statusChanged.emit("Update completed - target rebooting")
                    self.updateCompleted.emit(True, "Update completed successfully", timing_info)
                else:
                    self.updateCompleted.emit(False, f"Update failed: {str(e)}", {})
                    
        except Exception as e:
            self.updateCompleted.emit(False, f"Update failed: {str(e)}", {})
    
    def _upload_progress(self, transferred: int, total: int):
        """Handle upload progress."""
        if total > 0:
            # Upload progress: 25-50% (25% range for upload within Step 2)
            progress = int((transferred / total) * 25) + 25  # 25-50% for upload
            self.progressChanged.emit(progress)
    
    def _install_progress(self, progress):
        """Handle installation progress."""
        # progress is an InstallProgress object
        # Installation progress: 50-85% (35% range for installation within Step 2)
        install_percentage = int(progress.percentage * 0.35) + 50  # 50-85% for installation
        self.progressChanged.emit(install_percentage)
        if progress.message:
            self.logMessage.emit(progress.message)


class RaucController(QObject):
    """Main controller for RAUC GUI operations."""
    
    # Signals
    connectionStatusChanged = pyqtSignal(bool)
    progressChanged = pyqtSignal(int)
    statusChanged = pyqtSignal(str)
    logMessage = pyqtSignal(str)
    updateCompleted = pyqtSignal(bool, str, dict)  # Added timing_info parameter
    
    def __init__(self):
        super().__init__()
        self._host = "192.168.1.100"
        self._username = "root"
        self._port = 22
        self._bundle_path = ""
        self._is_connected = False
        self._is_updating = False
        self._progress = 0
        self._update_worker: Optional[UpdateWorker] = None
        self._timing_info = {}
        
        # Initialize progress to 0 to avoid null property issues
        self.progressChanged.emit(0)
    
    # Properties
    @pyqtProperty(str)
    def host(self):
        return self._host
    
    @host.setter
    def host(self, value):
        self._host = value
    
    @pyqtProperty(str)
    def username(self):
        return self._username
    
    @username.setter
    def username(self, value):
        self._username = value
    
    @pyqtProperty(int)
    def port(self):
        return self._port
    
    @port.setter
    def port(self, value):
        self._port = value
    
    @pyqtProperty(str)
    def bundlePath(self):
        return self._bundle_path
    
    @bundlePath.setter
    def bundlePath(self, value):
        self._bundle_path = value
    
    @pyqtProperty(bool, notify=connectionStatusChanged)
    def isConnected(self):
        return self._is_connected
    
    @pyqtProperty(bool)
    def isUpdating(self):
        return self._is_updating
    
    @pyqtProperty(int, notify=progressChanged)
    def progress(self):
        return self._progress
    
    @pyqtProperty('QVariant')
    def timingInfo(self):
        return self._timing_info
    
    # Slots (methods callable from QML)
    @pyqtSlot()
    def testConnection(self):
        """Test connection to target device."""
        self.statusChanged.emit("Testing connection...")
        self.logMessage.emit(f"Connecting to {self._username}@{self._host}:{self._port}")
        
        try:
            config = ConnectionConfig(
                host=self._host,
                username=self._username,
                port=self._port
            )
            
            with SSHConnection(config) as conn:
                if conn.test_rauc_availability():
                    self._is_connected = True
                    self.connectionStatusChanged.emit(True)
                    self.statusChanged.emit("Connected successfully")
                    self.logMessage.emit("✓ Connection successful, RAUC available")
                else:
                    self._is_connected = False
                    self.connectionStatusChanged.emit(False)
                    self.statusChanged.emit("Connection failed - RAUC not available")
                    self.logMessage.emit("✗ RAUC not available on target")
                    
        except Exception as e:
            self._is_connected = False
            self.connectionStatusChanged.emit(False)
            self.statusChanged.emit(f"Connection failed: {str(e)}")
            self.logMessage.emit(f"✗ Connection failed: {str(e)}")
    
    @pyqtSlot(result=str)
    def selectBundleFile(self):
        """Open file dialog to select RAUC bundle."""
        file_dialog = QFileDialog()
        file_path, _ = file_dialog.getOpenFileName(
            None,
            "Select RAUC Bundle",
            "",
            "RAUC Bundles (*.raucb);;All Files (*)"
        )
        
        if file_path:
            self._bundle_path = file_path
            self.logMessage.emit(f"Selected bundle: {file_path}")
            return file_path
        
        return ""
    
    @pyqtSlot()
    def startUpdate(self):
        """Start the RAUC update process."""
        if not self._bundle_path:
            self.statusChanged.emit("No bundle selected")
            return
        
        if not Path(self._bundle_path).exists():
            self.statusChanged.emit("Bundle file does not exist")
            return
        
        self._is_updating = True
        self.statusChanged.emit("Starting update...")
        self.logMessage.emit("Starting RAUC update process")
        
        # Create worker thread
        config = ConnectionConfig(
            host=self._host,
            username=self._username,
            port=self._port
        )
        
        self._update_worker = UpdateWorker(self._bundle_path, config)
        self._update_worker.progressChanged.connect(self._onProgressChanged)
        self._update_worker.statusChanged.connect(self.statusChanged)
        self._update_worker.logMessage.connect(self.logMessage)
        self._update_worker.updateCompleted.connect(self._onUpdateCompleted)
        
        self._update_worker.start()
    
    @pyqtSlot()
    def cancelUpdate(self):
        """Cancel the ongoing update."""
        if self._update_worker:
            self._update_worker.cancel()
            self.statusChanged.emit("Cancelling update...")
    
    @pyqtSlot()
    def resetState(self):
        """Reset controller to initial state."""
        self._bundle_path = ""
        self._is_updating = False
        self._progress = 0
        self.bundlePathChanged.emit("")
        self.isUpdatingChanged.emit(False)
        self.progressChanged.emit(0)
        self.statusChanged.emit("Ready")
    
    def _onProgressChanged(self, value: int):
        """Handle progress updates."""
        self._progress = value
        self.progressChanged.emit(value)
    
    def _onUpdateCompleted(self, success: bool, message: str, timing_info: dict = None):
        """Handle update completion."""
        self._is_updating = False
        self._progress = 100 if success else 0
        self._timing_info = timing_info or {}
        self.progressChanged.emit(self._progress)
        self.updateCompleted.emit(success, message, self._timing_info)
        
        if success:
            self.logMessage.emit("✓ Update completed successfully")
            self.statusChanged.emit("Update completed")
        else:
            self.logMessage.emit(f"✗ Update failed: {message}")
            self.statusChanged.emit(f"Update failed: {message}")
        
        if self._update_worker:
            self._update_worker.deleteLater()
            self._update_worker = None