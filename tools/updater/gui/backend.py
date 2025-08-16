#!/usr/bin/env python3
"""
Backend classes for QML interface
Provides the bridge between QML frontend and server functionality
"""

import os
import json
import requests
import subprocess
import threading
import time
from datetime import datetime
from pathlib import Path
from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot, QThread, QTimer
from PyQt6.QtQml import QQmlApplicationEngine


class ServerManager(QObject):
    """Manages the backend server process."""
    
    # Signals
    serverStarted = pyqtSignal()
    serverStopped = pyqtSignal()
    serverError = pyqtSignal(str)
    serverOutput = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
        self.server_process = None
        self.running = False
        self.server_script = Path(__file__).parent.parent / "server" / "main.py"
        
    @pyqtSlot()
    def initialize(self):
        """Initialize the server manager."""
        self.serverOutput.emit("Server manager initialized")
        
    @pyqtSlot()
    def startServer(self):
        """Start the backend server."""
        if self.server_process and self.server_process.poll() is None:
            self.serverOutput.emit("Server is already running")
            return
        
        try:
            # Start server process
            cmd = ["uv", "run", "python", "-m", "server.main"]
            if not self._command_exists("uv"):
                cmd = ["python3", str(self.server_script)]
            
            self.serverOutput.emit(f"Starting server with command: {' '.join(cmd)}")
            
            self.server_process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                cwd=Path(__file__).parent.parent
            )
            
            self.running = True
            
            # Start monitoring thread
            monitor_thread = threading.Thread(target=self._monitor_server, daemon=True)
            monitor_thread.start()
            
            # Wait a moment for server to start
            time.sleep(2)
            
            # Check if server is responsive
            if self._test_server_connection():
                self.serverStarted.emit()
                self.serverOutput.emit("✅ Server started successfully")
            else:
                self.serverOutput.emit("⚠️ Server started but not responding yet...")
                
        except Exception as e:
            self.serverError.emit(f"Failed to start server: {e}")
    
    @pyqtSlot()
    def stopServer(self):
        """Stop the backend server."""
        self.running = False
        
        if self.server_process:
            try:
                self.serverOutput.emit("Stopping server...")
                
                # Try graceful shutdown first
                self.server_process.terminate()
                
                # Wait for graceful shutdown
                try:
                    self.server_process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    # Force kill if needed
                    self.server_process.kill()
                    self.server_process.wait()
                
                self.serverStopped.emit()
                self.serverOutput.emit("✅ Server stopped")
                
            except Exception as e:
                self.serverError.emit(f"Error stopping server: {e}")
    
    def _monitor_server(self):
        """Monitor server output."""
        if not self.server_process:
            return
            
        for line in iter(self.server_process.stdout.readline, ''):
            if not self.running:
                break
            self.serverOutput.emit(line.strip())
    
    def _test_server_connection(self):
        """Test if server is responding."""
        try:
            response = requests.get("http://localhost:8080/", timeout=2)
            return response.status_code == 200
        except:
            return False
    
    def _command_exists(self, command):
        """Check if a command exists."""
        return subprocess.run(["which", command], capture_output=True).returncode == 0


class DeploymentManager(QObject):
    """Manages deployment operations."""
    
    # Signals
    deploymentsUpdated = pyqtSignal(list)
    deploymentToggled = pyqtSignal(str, bool)
    deploymentAdded = pyqtSignal(str)
    deploymentError = pyqtSignal(str)
    bundlesScanned = pyqtSignal(list)
    
    def __init__(self):
        super().__init__()
        self.base_url = "http://localhost:8080"
        self.deployments = []
        
        # Start periodic refresh
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self.refreshDeployments)
        self.refresh_timer.start(5000)  # Refresh every 5 seconds
        
    @pyqtSlot()
    def refreshDeployments(self):
        """Refresh the list of deployments."""
        try:
            response = requests.get(f"{self.base_url}/admin/deployments", timeout=2)
            if response.status_code == 200:
                self.deployments = response.json()
                self.deploymentsUpdated.emit(self.deployments)
        except Exception as e:
            self.deploymentError.emit(f"Failed to refresh deployments: {e}")
    
    @pyqtSlot(str, bool)
    def toggleDeployment(self, deployment_id, active):
        """Toggle deployment active state."""
        try:
            url = f"{self.base_url}/admin/deployments/{deployment_id}"
            data = {"active": active}
            response = requests.put(url, json=data, timeout=5)
            
            if response.status_code == 200:
                self.deploymentToggled.emit(deployment_id, active)
                self.refreshDeployments()
            else:
                self.deploymentError.emit(f"Failed to toggle deployment: {response.text}")
                
        except Exception as e:
            self.deploymentError.emit(f"Error toggling deployment: {e}")
    
    @pyqtSlot(str, str, str)
    def addDeployment(self, name, bundle_file, version):
        """Add a new deployment."""
        try:
            # First, copy bundle file to bundle directory
            bundle_path = Path(__file__).parent.parent / "bundle" / Path(bundle_file).name
            if Path(bundle_file).exists():
                import shutil
                shutil.copy2(bundle_file, bundle_path)
                bundle_filename = bundle_path.name
            else:
                self.deploymentError.emit(f"Bundle file not found: {bundle_file}")
                return
            
            # Create deployment
            url = f"{self.base_url}/admin/deployments"
            data = {
                "name": name,
                "version": version,
                "filename": bundle_filename,
                "active": True
            }
            
            response = requests.post(url, json=data, timeout=5)
            
            if response.status_code == 200:
                self.deploymentAdded.emit(name)
                self.refreshDeployments()
            else:
                self.deploymentError.emit(f"Failed to add deployment: {response.text}")
                
        except Exception as e:
            self.deploymentError.emit(f"Error adding deployment: {e}")
    
    @pyqtSlot()
    def scanBundles(self):
        """Scan for bundle files and emit the list."""
        try:
            from .bundle_manager import bundle_manager
            
            bundles = bundle_manager.scan_bundles()
            self.bundlesScanned.emit(bundles)
            
        except Exception as e:
            self.deploymentError.emit(f"Error scanning bundles: {e}")
    
    @pyqtSlot(str)
    def createDeploymentFromLatest(self, name):
        """Create deployment from the latest bundle."""
        try:
            from .bundle_manager import bundle_manager
            
            deployment = bundle_manager.create_deployment_from_latest(name)
            if deployment:
                # Create deployment via API
                url = f"{self.base_url}/admin/deployments"
                response = requests.post(url, json=deployment, timeout=5)
                
                if response.status_code == 200:
                    self.deploymentAdded.emit(name)
                    self.refreshDeployments()
                else:
                    self.deploymentError.emit(f"Failed to create deployment: {response.text}")
            else:
                self.deploymentError.emit("No latest bundle available")
                
        except Exception as e:
            self.deploymentError.emit(f"Error creating deployment: {e}")


class LogManager(QObject):
    """Manages application logs."""
    
    # Signals
    logsUpdated = pyqtSignal(list)
    
    def __init__(self):
        super().__init__()
        self.logs = []
        self.max_logs = 1000
        
    @pyqtSlot(str)
    def addLog(self, message):
        """Add a log message."""
        timestamp = datetime.now().strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}"
        self.logs.append(log_entry)
        
        # Keep only the last max_logs entries
        if len(self.logs) > self.max_logs:
            self.logs = self.logs[-self.max_logs:]
        
        self.logsUpdated.emit(self.logs)
    
    @pyqtSlot()
    def clearLogs(self):
        """Clear all logs."""
        self.logs = []
        self.logsUpdated.emit(self.logs)
    
    @pyqtSlot(list)
    def setLogs(self, logs):
        """Set logs from external source."""
        self.logs = logs
        self.logsUpdated.emit(self.logs) 