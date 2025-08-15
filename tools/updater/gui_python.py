#!/usr/bin/env python3
"""
Python-based Qt GUI for Updater Server
Connects to the backend server via HTTP API
"""

import sys
import json
import requests
from datetime import datetime
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QTextEdit, QTableWidget, QTableWidgetItem,
    QGroupBox, QGridLayout, QProgressBar, QMessageBox, QTabWidget,
    QSplitter, QFrame, QHeaderView
)
from PyQt6.QtCore import QTimer, QThread, pyqtSignal, Qt
from PyQt6.QtGui import QFont, QPalette, QColor


class BackendManager(QThread):
    """Thread for managing backend communication."""
    
    # Signals
    connection_status_changed = pyqtSignal(bool)
    deployments_updated = pyqtSignal(list)
    server_info_updated = pyqtSignal(dict)
    error_occurred = pyqtSignal(str)
    
    def __init__(self, server_url="http://localhost:8080"):
        super().__init__()
        self.server_url = server_url
        self.is_connected = False
        self.running = True
        
    def run(self):
        """Main thread loop for backend communication."""
        while self.running:
            try:
                # Check server connection
                response = requests.get(f"{self.server_url}/", timeout=5)
                if response.status_code == 200:
                    if not self.is_connected:
                        self.is_connected = True
                        self.connection_status_changed.emit(True)
                    
                    # Get server info
                    server_info = response.json()
                    self.server_info_updated.emit(server_info)
                    
                    # Get deployments
                    deployments_response = requests.get(f"{self.server_url}/admin/deployments", timeout=5)
                    if deployments_response.status_code == 200:
                        deployments = deployments_response.json()["deployments"]
                        self.deployments_updated.emit(deployments)
                    
                else:
                    if self.is_connected:
                        self.is_connected = False
                        self.connection_status_changed.emit(False)
                        
            except requests.exceptions.RequestException as e:
                if self.is_connected:
                    self.is_connected = False
                    self.connection_status_changed.emit(False)
                self.error_occurred.emit(f"Connection error: {str(e)}")
            
            # Sleep for 5 seconds before next check
            self.msleep(5000)
    
    def stop(self):
        """Stop the thread."""
        self.running = False
        self.wait()
    
    def enable_deployment(self, execution_id):
        """Enable a deployment."""
        try:
            # Get current deployment
            response = requests.get(f"{self.server_url}/admin/deployments")
            if response.status_code == 200:
                deployments = response.json()["deployments"]
                
                for deployment in deployments:
                    if deployment["execution_id"] == execution_id:
                        # Update deployment to active
                        deployment["active"] = True
                        
                        # Send update request
                        update_response = requests.put(
                            f"{self.server_url}/admin/deployments/{execution_id}",
                            json=deployment
                        )
                        return update_response.status_code == 200
            
            return False
        except Exception as e:
            self.error_occurred.emit(f"Failed to enable deployment: {str(e)}")
            return False
    
    def disable_deployment(self, execution_id):
        """Disable a deployment."""
        try:
            # Get current deployment
            response = requests.get(f"{self.server_url}/admin/deployments")
            if response.status_code == 200:
                deployments = response.json()["deployments"]
                
                for deployment in deployments:
                    if deployment["execution_id"] == execution_id:
                        # Update deployment to inactive
                        deployment["active"] = False
                        
                        # Send update request
                        update_response = requests.put(
                            f"{self.server_url}/admin/deployments/{execution_id}",
                            json=deployment
                        )
                        return update_response.status_code == 200
            
            return False
        except Exception as e:
            self.error_occurred.emit(f"Failed to disable deployment: {str(e)}")
            return False


class UpdaterGUI(QMainWindow):
    """Main GUI window for the Updater Server."""
    
    def __init__(self):
        super().__init__()
        self.backend_manager = BackendManager()
        self.deployments = []
        self.server_info = {}
        
        self.init_ui()
        self.setup_connections()
        self.backend_manager.start()
    
    def init_ui(self):
        """Initialize the user interface."""
        self.setWindowTitle("Updater Server - Management Console")
        self.setGeometry(100, 100, 1200, 800)
        
        # Set application style
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f5f5f5;
            }
            QGroupBox {
                font-weight: bold;
                border: 2px solid #cccccc;
                border-radius: 5px;
                margin-top: 1ex;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
            QPushButton {
                background-color: #0078d4;
                color: white;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #106ebe;
            }
            QPushButton:pressed {
                background-color: #005a9e;
            }
            QPushButton:disabled {
                background-color: #cccccc;
                color: #666666;
            }
            QTableWidget {
                gridline-color: #cccccc;
                background-color: white;
                alternate-background-color: #f9f9f9;
            }
            QHeaderView::section {
                background-color: #e1e1e1;
                padding: 5px;
                border: 1px solid #cccccc;
                font-weight: bold;
            }
        """)
        
        # Create central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Create main layout
        main_layout = QVBoxLayout(central_widget)
        main_layout.setSpacing(10)
        main_layout.setContentsMargins(10, 10, 10, 10)
        
        # Create header
        self.create_header(main_layout)
        
        # Create tab widget
        self.tab_widget = QTabWidget()
        main_layout.addWidget(self.tab_widget)
        
        # Create tabs
        self.create_dashboard_tab()
        self.create_deployments_tab()
        self.create_logs_tab()
    
    def create_header(self, parent_layout):
        """Create the header section."""
        header_frame = QFrame()
        header_frame.setFrameStyle(QFrame.Shape.StyledPanel)
        header_frame.setStyleSheet("""
            QFrame {
                background-color: #2c3e50;
                border-radius: 5px;
                padding: 10px;
            }
        """)
        
        header_layout = QHBoxLayout(header_frame)
        
        # App title
        title_label = QLabel("🔄 Updater Server Management Console")
        title_label.setStyleSheet("""
            QLabel {
                color: white;
                font-size: 18px;
                font-weight: bold;
            }
        """)
        header_layout.addWidget(title_label)
        
        header_layout.addStretch()
        
        # Connection status
        self.connection_indicator = QLabel("●")
        self.connection_indicator.setStyleSheet("""
            QLabel {
                color: #e74c3c;
                font-size: 16px;
                font-weight: bold;
            }
        """)
        header_layout.addWidget(self.connection_indicator)
        
        self.connection_text = QLabel("Disconnected")
        self.connection_text.setStyleSheet("""
            QLabel {
                color: white;
                font-size: 14px;
            }
        """)
        header_layout.addWidget(self.connection_text)
        
        # Refresh button
        refresh_btn = QPushButton("Refresh")
        refresh_btn.clicked.connect(self.refresh_data)
        header_layout.addWidget(refresh_btn)
        
        parent_layout.addWidget(header_frame)
    
    def create_dashboard_tab(self):
        """Create the dashboard tab."""
        dashboard_widget = QWidget()
        dashboard_layout = QVBoxLayout(dashboard_widget)
        
        # Server status group
        server_group = QGroupBox("Server Status")
        server_layout = QGridLayout(server_group)
        
        self.server_status_label = QLabel("Unknown")
        self.server_status_label.setStyleSheet("font-weight: bold; color: #e74c3c;")
        server_layout.addWidget(QLabel("Status:"), 0, 0)
        server_layout.addWidget(self.server_status_label, 0, 1)
        
        self.server_version_label = QLabel("Unknown")
        server_layout.addWidget(QLabel("Version:"), 1, 0)
        server_layout.addWidget(self.server_version_label, 1, 1)
        
        self.server_protocol_label = QLabel("Unknown")
        server_layout.addWidget(QLabel("Protocol:"), 2, 0)
        server_layout.addWidget(self.server_protocol_label, 2, 1)
        
        self.server_gui_label = QLabel("Unknown")
        server_layout.addWidget(QLabel("GUI Enabled:"), 3, 0)
        server_layout.addWidget(self.server_gui_label, 3, 1)
        
        dashboard_layout.addWidget(server_group)
        
        # Quick actions group
        actions_group = QGroupBox("Quick Actions")
        actions_layout = QHBoxLayout(actions_group)
        
        refresh_btn = QPushButton("Refresh Status")
        refresh_btn.clicked.connect(self.refresh_data)
        actions_layout.addWidget(refresh_btn)
        
        actions_layout.addStretch()
        
        dashboard_layout.addWidget(actions_group)
        dashboard_layout.addStretch()
        
        self.tab_widget.addTab(dashboard_widget, "Dashboard")
    
    def create_deployments_tab(self):
        """Create the deployments tab."""
        deployments_widget = QWidget()
        deployments_layout = QVBoxLayout(deployments_widget)
        
        # Deployments table
        self.deployments_table = QTableWidget()
        self.deployments_table.setColumnCount(6)
        self.deployments_table.setHorizontalHeaderLabels([
            "Execution ID", "Version", "Size (MB)", "Filename", "Status", "Actions"
        ])
        
        # Set table properties
        header = self.deployments_table.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.ResizeToContents)
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.ResizeToContents)
        header.setSectionResizeMode(3, QHeaderView.ResizeMode.Stretch)
        header.setSectionResizeMode(4, QHeaderView.ResizeMode.ResizeToContents)
        header.setSectionResizeMode(5, QHeaderView.ResizeMode.ResizeToContents)
        
        deployments_layout.addWidget(self.deployments_table)
        
        self.tab_widget.addTab(deployments_widget, "Deployments")
    
    def create_logs_tab(self):
        """Create the logs tab."""
        logs_widget = QWidget()
        logs_layout = QVBoxLayout(logs_widget)
        
        # Log text area
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setFont(QFont("Monospace", 10))
        logs_layout.addWidget(self.log_text)
        
        # Log controls
        log_controls = QHBoxLayout()
        
        clear_logs_btn = QPushButton("Clear Logs")
        clear_logs_btn.clicked.connect(self.log_text.clear)
        log_controls.addWidget(clear_logs_btn)
        
        log_controls.addStretch()
        
        logs_layout.addLayout(log_controls)
        
        self.tab_widget.addTab(logs_widget, "Logs")
    
    def setup_connections(self):
        """Setup signal connections."""
        self.backend_manager.connection_status_changed.connect(self.update_connection_status)
        self.backend_manager.deployments_updated.connect(self.update_deployments)
        self.backend_manager.server_info_updated.connect(self.update_server_info)
        self.backend_manager.error_occurred.connect(self.log_error)
    
    def update_connection_status(self, connected):
        """Update the connection status display."""
        if connected:
            self.connection_indicator.setStyleSheet("color: #27ae60; font-size: 16px; font-weight: bold;")
            self.connection_text.setText("Connected")
            self.log_message("Connected to backend server")
        else:
            self.connection_indicator.setStyleSheet("color: #e74c3c; font-size: 16px; font-weight: bold;")
            self.connection_text.setText("Disconnected")
            self.log_message("Disconnected from backend server")
    
    def update_server_info(self, server_info):
        """Update server information display."""
        self.server_info = server_info
        
        # Update labels
        self.server_status_label.setText("Running")
        self.server_status_label.setStyleSheet("font-weight: bold; color: #27ae60;")
        
        self.server_version_label.setText(server_info.get("version", "Unknown"))
        self.server_protocol_label.setText(server_info.get("protocol", "Unknown").upper())
        self.server_gui_label.setText("Yes" if server_info.get("gui_enabled", False) else "No")
    
    def update_deployments(self, deployments):
        """Update the deployments table."""
        self.deployments = deployments
        self.deployments_table.setRowCount(len(deployments))
        
        for row, deployment in enumerate(deployments):
            # Execution ID
            self.deployments_table.setItem(row, 0, QTableWidgetItem(deployment["execution_id"]))
            
            # Version
            self.deployments_table.setItem(row, 1, QTableWidgetItem(deployment["version"]))
            
            # Size in MB
            size_mb = deployment["size"] / (1024 * 1024)
            self.deployments_table.setItem(row, 2, QTableWidgetItem(f"{size_mb:.1f}"))
            
            # Filename
            self.deployments_table.setItem(row, 3, QTableWidgetItem(deployment["filename"]))
            
            # Status
            status = "Active" if deployment.get("active", True) else "Disabled"
            status_item = QTableWidgetItem(status)
            if status == "Active":
                status_item.setBackground(QColor("#d4edda"))
            else:
                status_item.setBackground(QColor("#f8d7da"))
            self.deployments_table.setItem(row, 4, status_item)
            
            # Actions
            actions_widget = QWidget()
            actions_layout = QHBoxLayout(actions_widget)
            actions_layout.setContentsMargins(2, 2, 2, 2)
            
            if deployment.get("active", True):
                disable_btn = QPushButton("Disable")
                disable_btn.clicked.connect(lambda checked, d=deployment: self.disable_deployment(d["execution_id"]))
                actions_layout.addWidget(disable_btn)
            else:
                enable_btn = QPushButton("Enable")
                enable_btn.clicked.connect(lambda checked, d=deployment: self.enable_deployment(d["execution_id"]))
                actions_layout.addWidget(enable_btn)
            
            self.deployments_table.setCellWidget(row, 5, actions_widget)
        
        self.log_message(f"Updated {len(deployments)} deployments")
    
    def enable_deployment(self, execution_id):
        """Enable a deployment."""
        if self.backend_manager.enable_deployment(execution_id):
            self.log_message(f"Enabled deployment: {execution_id}")
            QMessageBox.information(self, "Success", f"Deployment {execution_id} has been enabled.")
        else:
            self.log_message(f"Failed to enable deployment: {execution_id}")
            QMessageBox.warning(self, "Error", f"Failed to enable deployment {execution_id}")
    
    def disable_deployment(self, execution_id):
        """Disable a deployment."""
        if self.backend_manager.disable_deployment(execution_id):
            self.log_message(f"Disabled deployment: {execution_id}")
            QMessageBox.information(self, "Success", f"Deployment {execution_id} has been disabled.")
        else:
            self.log_message(f"Failed to disable deployment: {execution_id}")
            QMessageBox.warning(self, "Error", f"Failed to disable deployment {execution_id}")
    
    def refresh_data(self):
        """Manually refresh data from the backend."""
        self.log_message("Manual refresh requested")
        # The backend manager will automatically refresh every 5 seconds
    
    def log_message(self, message):
        """Add a message to the log."""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.append(f"[{timestamp}] {message}")
    
    def log_error(self, error):
        """Log an error message."""
        self.log_message(f"ERROR: {error}")
    
    def closeEvent(self, event):
        """Handle application close event."""
        self.backend_manager.stop()
        event.accept()


def main():
    """Main application entry point."""
    app = QApplication(sys.argv)
    
    # Set application properties
    app.setApplicationName("Updater Server GUI")
    app.setApplicationVersion("0.2.0")
    app.setOrganizationName("Updater Project")
    
    # Create and show the main window
    window = UpdaterGUI()
    window.show()
    
    # Start the application
    sys.exit(app.exec())


if __name__ == "__main__":
    main() 