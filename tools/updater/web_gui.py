#!/usr/bin/env python3
"""
Web-based GUI for Updater Server
Connects to the backend server via HTTP API
"""

import requests
import json
from datetime import datetime
from flask import Flask, render_template_string, request, jsonify, redirect, url_for
import threading
import time

app = Flask(__name__)

# Configuration
BACKEND_URL = "http://localhost:8080"
REFRESH_INTERVAL = 5  # seconds

# Global state
server_info = {}
deployments = []
connection_status = False
last_update = None

# HTML template
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Updater Server - Management Console</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #f5f5f5;
            color: #333;
        }
        
        .header {
            background: linear-gradient(135deg, #2c3e50 0%, #34495e 100%);
            color: white;
            padding: 20px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        
        .header-content {
            display: flex;
            justify-content: space-between;
            align-items: center;
            max-width: 1200px;
            margin: 0 auto;
        }
        
        .title {
            font-size: 24px;
            font-weight: bold;
        }
        
        .status-indicator {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .status-dot {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            animation: pulse 2s infinite;
        }
        
        .status-dot.connected {
            background-color: #27ae60;
        }
        
        .status-dot.disconnected {
            background-color: #e74c3c;
        }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
        
        .container {
            max-width: 1200px;
            margin: 20px auto;
            padding: 0 20px;
        }
        
        .tabs {
            display: flex;
            background: white;
            border-radius: 8px 8px 0 0;
            overflow: hidden;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        
        .tab {
            padding: 15px 30px;
            background: #f8f9fa;
            border: none;
            cursor: pointer;
            transition: background-color 0.3s;
        }
        
        .tab.active {
            background: white;
            border-bottom: 3px solid #0078d4;
        }
        
        .tab:hover {
            background: #e9ecef;
        }
        
        .tab-content {
            background: white;
            padding: 30px;
            border-radius: 0 0 8px 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            min-height: 400px;
        }
        
        .tab-pane {
            display: none;
        }
        
        .tab-pane.active {
            display: block;
        }
        
        .card {
            background: white;
            border: 1px solid #dee2e6;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }
        
        .card h3 {
            margin-bottom: 15px;
            color: #495057;
            border-bottom: 2px solid #e9ecef;
            padding-bottom: 10px;
        }
        
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
        }
        
        .info-item {
            display: flex;
            justify-content: space-between;
            padding: 10px;
            background: #f8f9fa;
            border-radius: 4px;
        }
        
        .info-label {
            font-weight: bold;
            color: #495057;
        }
        
        .info-value {
            color: #0078d4;
        }
        
        .table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
        }
        
        .table th,
        .table td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #dee2e6;
        }
        
        .table th {
            background: #f8f9fa;
            font-weight: bold;
            color: #495057;
        }
        
        .table tr:hover {
            background: #f8f9fa;
        }
        
        .btn {
            padding: 8px 16px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            text-decoration: none;
            display: inline-block;
            transition: background-color 0.3s;
        }
        
        .btn-primary {
            background: #0078d4;
            color: white;
        }
        
        .btn-primary:hover {
            background: #106ebe;
        }
        
        .btn-danger {
            background: #dc3545;
            color: white;
        }
        
        .btn-danger:hover {
            background: #c82333;
        }
        
        .btn-success {
            background: #28a745;
            color: white;
        }
        
        .btn-success:hover {
            background: #218838;
        }
        
        .status-badge {
            padding: 4px 8px;
            border-radius: 12px;
            font-size: 12px;
            font-weight: bold;
        }
        
        .status-active {
            background: #d4edda;
            color: #155724;
        }
        
        .status-disabled {
            background: #f8d7da;
            color: #721c24;
        }
        
        .log-area {
            background: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 4px;
            padding: 15px;
            height: 300px;
            overflow-y: auto;
            font-family: 'Courier New', monospace;
            font-size: 12px;
        }
        
        .refresh-btn {
            background: #0078d4;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
        }
        
        .refresh-btn:hover {
            background: #106ebe;
        }
        
        .last-update {
            color: #6c757d;
            font-size: 12px;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="header">
        <div class="header-content">
            <div class="title">🔄 Updater Server Management Console</div>
            <div class="status-indicator">
                <div class="status-dot {{ 'connected' if connection_status else 'disconnected' }}"></div>
                <span>{{ 'Connected' if connection_status else 'Disconnected' }}</span>
                <button class="refresh-btn" onclick="refreshData()">Refresh</button>
            </div>
        </div>
    </div>
    
    <div class="container">
        <div class="tabs">
            <button class="tab active" onclick="showTab('dashboard')">Dashboard</button>
            <button class="tab" onclick="showTab('deployments')">Deployments</button>
            <button class="tab" onclick="showTab('logs')">Logs</button>
        </div>
        
        <div class="tab-content">
            <!-- Dashboard Tab -->
            <div id="dashboard" class="tab-pane active">
                <div class="card">
                    <h3>Server Status</h3>
                    <div class="info-grid">
                        <div class="info-item">
                            <span class="info-label">Status:</span>
                            <span class="info-value">{{ server_info.get('status', 'Unknown') }}</span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">Version:</span>
                            <span class="info-value">{{ server_info.get('version', 'Unknown') }}</span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">Protocol:</span>
                            <span class="info-value">{{ server_info.get('protocol', 'Unknown').upper() }}</span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">GUI Enabled:</span>
                            <span class="info-value">{{ 'Yes' if server_info.get('gui_enabled', False) else 'No' }}</span>
                        </div>
                    </div>
                </div>
                
                <div class="card">
                    <h3>Quick Actions</h3>
                    <button class="btn btn-primary" onclick="refreshData()">Refresh Status</button>
                </div>
            </div>
            
            <!-- Deployments Tab -->
            <div id="deployments" class="tab-pane">
                <div class="card">
                    <h3>Upload New Bundle</h3>
                    <div style="margin-bottom: 20px;">
                        <form id="uploadForm" enctype="multipart/form-data" style="display: flex; gap: 10px; align-items: center;">
                            <input type="file" id="bundleFile" name="file" accept=".raucb" style="flex: 1; padding: 8px; border: 1px solid #ddd; border-radius: 4px;">
                            <button type="submit" class="btn btn-primary">Upload Bundle</button>
                        </form>
                        <div id="uploadProgress" style="display: none; margin-top: 10px;">
                            <div style="background: #f0f0f0; border-radius: 4px; overflow: hidden;">
                                <div id="progressBar" style="background: #0078d4; height: 20px; width: 0%; transition: width 0.3s;"></div>
                            </div>
                            <div id="progressText" style="text-align: center; margin-top: 5px; font-size: 12px;">Uploading...</div>
                        </div>
                        <div id="uploadResult" style="margin-top: 10px;"></div>
                    </div>
                </div>
                
                <div class="card">
                    <h3>Deployments</h3>
                    {% if deployments %}
                        <table class="table">
                            <thead>
                                <tr>
                                    <th>Execution ID</th>
                                    <th>Version</th>
                                    <th>Size (MB)</th>
                                    <th>Filename</th>
                                    <th>Status</th>
                                    <th>Actions</th>
                                </tr>
                            </thead>
                            <tbody>
                                {% for deployment in deployments %}
                                <tr>
                                    <td>{{ deployment.execution_id }}</td>
                                    <td>{{ deployment.version }}</td>
                                    <td>{{ "%.1f"|format(deployment.size / (1024 * 1024)) }}</td>
                                    <td>{{ deployment.filename }}</td>
                                    <td>
                                        <span class="status-badge {{ 'status-active' if deployment.active else 'status-disabled' }}">
                                            {{ 'Active' if deployment.active else 'Disabled' }}
                                        </span>
                                    </td>
                                    <td>
                                        {% if deployment.active %}
                                            <button class="btn btn-danger" onclick="toggleDeployment('{{ deployment.execution_id }}', false)">
                                                Disable
                                            </button>
                                        {% else %}
                                            <button class="btn btn-success" onclick="toggleDeployment('{{ deployment.execution_id }}', true)">
                                                Enable
                                            </button>
                                        {% endif %}
                                    </td>
                                </tr>
                                {% endfor %}
                            </tbody>
                        </table>
                    {% else %}
                        <p>No deployments available.</p>
                    {% endif %}
                </div>
            </div>
            
            <!-- Logs Tab -->
            <div id="logs" class="tab-pane">
                <div class="card">
                    <h3>System Logs</h3>
                    <div class="log-area" id="logArea">
                        <div>[{{ last_update }}] Web GUI started</div>
                        <div>[{{ last_update }}] Connected to backend server</div>
                    </div>
                    <button class="btn btn-primary" onclick="clearLogs()">Clear Logs</button>
                </div>
            </div>
        </div>
        
        <div class="last-update">
            Last updated: {{ last_update }}
        </div>
    </div>
    
    <script>
        function showTab(tabName) {
            // Hide all tab panes
            const tabPanes = document.querySelectorAll('.tab-pane');
            tabPanes.forEach(pane => pane.classList.remove('active'));
            
            // Remove active class from all tabs
            const tabs = document.querySelectorAll('.tab');
            tabs.forEach(tab => tab.classList.remove('active'));
            
            // Show selected tab pane
            document.getElementById(tabName).classList.add('active');
            
            // Add active class to clicked tab
            event.target.classList.add('active');
        }
        
        function refreshData() {
            location.reload();
        }
        
        function toggleDeployment(executionId, enable) {
            const action = enable ? 'enable' : 'disable';
            if (confirm(`Are you sure you want to ${action} deployment ${executionId}?`)) {
                fetch(`/api/deployment/${executionId}/${action}`, {
                    method: 'POST'
                })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        alert(`Deployment ${executionId} ${action}d successfully!`);
                        refreshData();
                    } else {
                        alert(`Failed to ${action} deployment: ${data.error}`);
                    }
                })
                .catch(error => {
                    alert(`Error: ${error}`);
                });
            }
        }
        
        function clearLogs() {
            document.getElementById('logArea').innerHTML = '';
        }
        
        // File upload handling
        document.getElementById('uploadForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const fileInput = document.getElementById('bundleFile');
            const file = fileInput.files[0];
            
            if (!file) {
                alert('Please select a file to upload');
                return;
            }
            
            if (!file.name.endsWith('.raucb')) {
                alert('Please select a .raucb file');
                return;
            }
            
            // Show progress
            document.getElementById('uploadProgress').style.display = 'block';
            document.getElementById('uploadResult').innerHTML = '';
            
            const formData = new FormData();
            formData.append('file', file);
            
            const xhr = new XMLHttpRequest();
            
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percentComplete = (e.loaded / e.total) * 100;
                    document.getElementById('progressBar').style.width = percentComplete + '%';
                    document.getElementById('progressText').textContent = 
                        `Uploading... ${Math.round(percentComplete)}%`;
                }
            });
            
            xhr.addEventListener('load', function() {
                document.getElementById('uploadProgress').style.display = 'none';
                
                if (xhr.status === 200) {
                    const response = JSON.parse(xhr.responseText);
                    if (response.success) {
                        document.getElementById('uploadResult').innerHTML = 
                            '<div style="color: #28a745; font-weight: bold;">✓ ' + response.message + '</div>';
                        // Refresh the page to show new deployment
                        setTimeout(() => location.reload(), 2000);
                    } else {
                        document.getElementById('uploadResult').innerHTML = 
                            '<div style="color: #dc3545; font-weight: bold;">✗ Error: ' + response.error + '</div>';
                    }
                } else {
                    document.getElementById('uploadResult').innerHTML = 
                        '<div style="color: #dc3545; font-weight: bold;">✗ Upload failed: HTTP ' + xhr.status + '</div>';
                }
            });
            
            xhr.addEventListener('error', function() {
                document.getElementById('uploadProgress').style.display = 'none';
                document.getElementById('uploadResult').innerHTML = 
                    '<div style="color: #dc3545; font-weight: bold;">✗ Upload failed: Network error</div>';
            });
            
            xhr.open('POST', '/api/upload');
            xhr.send(formData);
        });
        
        // Auto-refresh every 30 seconds
        setInterval(refreshData, 30000);
    </script>
</body>
</html>
"""

def update_data():
    """Update data from backend server."""
    global server_info, deployments, connection_status, last_update
    
    try:
        # Check server connection
        response = requests.get(f"{BACKEND_URL}/", timeout=5)
        if response.status_code == 200:
            server_info = response.json()
            connection_status = True
            
            # Get deployments
            deployments_response = requests.get(f"{BACKEND_URL}/admin/deployments", timeout=5)
            if deployments_response.status_code == 200:
                deployments = deployments_response.json()["deployments"]
            else:
                deployments = []
        else:
            connection_status = False
            server_info = {}
            deployments = []
            
    except requests.exceptions.RequestException:
        connection_status = False
        server_info = {}
        deployments = []
    
    last_update = datetime.now().strftime("%H:%M:%S")

def background_updater():
    """Background thread to update data periodically."""
    while True:
        update_data()
        time.sleep(REFRESH_INTERVAL)

@app.route('/')
def index():
    """Main page."""
    return render_template_string(HTML_TEMPLATE, 
                                server_info=server_info,
                                deployments=deployments,
                                connection_status=connection_status,
                                last_update=last_update or "Never")

@app.route('/api/deployment/<execution_id>/<action>')
def toggle_deployment(execution_id, action):
    """Toggle deployment status."""
    try:
        if action not in ['enable', 'disable']:
            return jsonify({'success': False, 'error': 'Invalid action'})
        
        # Get current deployment
        response = requests.get(f"{BACKEND_URL}/admin/deployments")
        if response.status_code == 200:
            deployments_data = response.json()["deployments"]
            
            for deployment in deployments_data:
                if deployment["execution_id"] == execution_id:
                    # Update deployment status
                    deployment["active"] = (action == 'enable')
                    
                    # Send update request
                    update_response = requests.put(
                        f"{BACKEND_URL}/admin/deployments/{execution_id}",
                        json=deployment
                    )
                    
                    if update_response.status_code == 200:
                        return jsonify({'success': True})
                    else:
                        return jsonify({'success': False, 'error': f'HTTP {update_response.status_code}'})
            
            return jsonify({'success': False, 'error': 'Deployment not found'})
        else:
            return jsonify({'success': False, 'error': f'HTTP {response.status_code}'})
            
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})

@app.route('/api/upload', methods=['POST'])
def upload_bundle():
    """Upload a new bundle file."""
    try:
        if 'file' not in request.files:
            return jsonify({'success': False, 'error': 'No file provided'})
        
        file = request.files['file']
        if file.filename == '':
            return jsonify({'success': False, 'error': 'No file selected'})
        
        # Check if file is a .raucb file
        if not file.filename.endswith('.raucb'):
            return jsonify({'success': False, 'error': 'Only .raucb files are allowed'})
        
        # Save file to bundle directory
        import os
        bundle_dir = "bundle"
        if not os.path.exists(bundle_dir):
            os.makedirs(bundle_dir)
        
        filepath = os.path.join(bundle_dir, file.filename)
        file.save(filepath)
        
        # Get file size
        file_size = os.path.getsize(filepath)
        
        # Create deployment configuration
        base_execution_id = f"exec-{os.path.splitext(file.filename)[0]}"
        version = os.path.splitext(file.filename)[0].split("-")[-1] if "-" in os.path.splitext(file.filename)[0] else "1.0.0"
        
        # Check if deployment already exists and create unique execution_id if needed
        execution_id = base_execution_id
        counter = 1
        while True:
            # Check if this execution_id already exists
            check_response = requests.get(f"{BACKEND_URL}/admin/deployments")
            if check_response.status_code == 200:
                existing_deployments = check_response.json()["deployments"]
                existing_ids = [d["execution_id"] for d in existing_deployments]
                
                if execution_id not in existing_ids:
                    break
                else:
                    execution_id = f"{base_execution_id}-{counter}"
                    counter += 1
            else:
                break
        
        deployment_config = {
            "execution_id": execution_id,
            "version": version,
            "description": f"Bundle: {file.filename}",
            "download_url": f"/download/{file.filename}",
            "filename": file.filename,
            "size": file_size,
            "active": True
        }
        
        # Add deployment to backend
        response = requests.post(f"{BACKEND_URL}/admin/deployments", json=deployment_config)
        
        if response.status_code == 200:
            return jsonify({
                'success': True, 
                'message': f'Bundle {file.filename} uploaded successfully as {execution_id}',
                'deployment': deployment_config
            })
        else:
            # Remove uploaded file if deployment creation failed
            os.remove(filepath)
            error_detail = response.text if response.text else f'HTTP {response.status_code}'
            return jsonify({'success': False, 'error': f'Failed to create deployment: {error_detail}'})
            
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})

@app.route('/api/status')
def api_status():
    """API endpoint for status."""
    return jsonify({
        'connection_status': connection_status,
        'server_info': server_info,
        'deployments': deployments,
        'last_update': last_update
    })

if __name__ == '__main__':
    # Start background updater thread
    updater_thread = threading.Thread(target=background_updater, daemon=True)
    updater_thread.start()
    
    # Initial data update
    update_data()
    
    print("🌐 Web GUI starting on http://localhost:5000")
    print("📡 Backend server: http://localhost:8080")
    print("🔄 Auto-refresh every 5 seconds")
    print("=" * 50)
    
    # Start Flask app
    app.run(host='0.0.0.0', port=5000, debug=False) 