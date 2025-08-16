# Updater - OTA Update Management System

A comprehensive OTA (Over-The-Air) update management solution with integrated GUI and server, built with **QML for easy maintenance** and **automatic bundle detection**.

## Features

### 🖥️ Modern QML GUI Application
- **Fixed Layout**: 1280x720 non-resizable window with top-bottom design
- **Bundle Information**: Static display of latest bundle with automatic detection
- **Real-time Logs**: Enhanced log viewer with auto-scroll functionality
- **Server Management**: Start/Stop server with real-time status indicators
- **Automatic Refresh**: Bundle scanning every 5 seconds (no manual refresh needed)
- **Clean Interface**: Modern, responsive design with consistent color scheme

### 🔧 Backend Server
- **Poll Endpoint**: Device update checking (`/DEFAULT/controller/v1/{device-id}`)
- **Download Endpoint**: Bundle file serving (`/DEFAULT/download/{filename}`)
- **Admin API**: Deployment management (`/admin/deployments`)
- **Bundle Discovery**: Automatic .raucb file detection and latest bundle identification
- **Real-time Logging**: Comprehensive logging system with timestamp formatting

## Quick Start

### 1. Install Dependencies

```bash
# Install dependencies using uv
uv sync

# Or manually install
pip install PyQt6 fastapi uvicorn
```

### 2. Run the Application

```bash
# Run in GUI mode (default) - QML-based interface
uv run main.py

# Or explicitly specify GUI mode
uv run main.py gui

# Run in CLI mode (server only)
uv run main.py cli
```

### 3. Add Bundles

```bash
# Copy your .raucb files to the bundle directory
cp your-bundle.raucb bundle/

# Bundles are automatically detected every 5 seconds
# The latest bundle is automatically identified and displayed
```

## Usage

### GUI Mode (Default) - QML Interface

The modern QML-based GUI provides:
- **Top Panel**: Bundle information display with latest bundle details
- **Bottom Panel**: Enhanced log viewer with auto-scroll functionality
- **Header**: Server status and controls with real-time indicators
- **Automatic Features**: Bundle scanning and server management

Features:
- ✅ **Fixed Layout**: 1280x720 non-resizable window
- ✅ **Auto Bundle Refresh**: Automatic scanning every 5 seconds
- ✅ **Latest Bundle Display**: Static text showing latest bundle information
- ✅ **Enhanced Log Viewer**: Bigger log area with auto-scroll
- ✅ **Server Management**: Start/Stop server with status indicators
- ✅ **Real-time Updates**: Live log updates and bundle detection
- ✅ **Modern UI**: Clean, responsive design with consistent theming

### CLI Mode (Server Only)

```bash
# Run server only (no GUI)
uv run main.py cli

# Server will be available at:
# - Main: http://localhost:8080
# - Admin: http://localhost:8080/admin/deployments
```

## API Endpoints

### Target Device Endpoints
- `GET /DEFAULT/controller/v1/{device-id}` - Poll for updates (returns HTTP 200/204)
- `GET /DEFAULT/download/{filename}` - Download bundle files

### Admin Management Endpoints
- `GET /admin/deployments` - List all deployments
- `POST /admin/deployments` - Create new deployment
- `PUT /admin/deployments/{id}` - Update deployment status
- `DELETE /admin/deployments/{id}` - Delete deployment

### Server Status Endpoints
- `GET /` - Server status and version information
- `GET /health` - Health check endpoint

## Configuration

### Environment Variables

```bash
# Server configuration
export UPDATER_PORT=8080                    # HTTP port
export UPDATER_HTTPS_PORT=8443              # HTTPS port  
export UPDATER_ENABLE_HTTPS=true            # Enable HTTPS
export UPDATER_EXTERNAL_HOST=192.168.1.101  # External host for URLs

# Bundle configuration  
export UPDATER_BUNDLE_DIR=bundle            # Bundle directory
export UPDATER_MAX_BUNDLE_SIZE=2147483648   # Max bundle size (2GB)

# Logging
export UPDATER_LOG_LEVEL=INFO               # Log level
export UPDATER_LOG_FILE=logs/updater.log    # Log file

# Security
export UPDATER_ENABLE_AUTH=true             # Enable authentication
export UPDATER_API_KEY=your-secret-key      # API key
```

### HTTPS/TLS Setup

```bash
# Generate certificates
./certs/generate_certs.sh

# Enable HTTPS
export UPDATER_ENABLE_HTTPS=true
uv run main.py  # GUI with HTTPS
uv run main.py cli  # Server only with HTTPS
```

## File Organization

```
updater/
├── main.py                    # 🎯 Main entry point
├── pyproject.toml            # 📦 Project configuration
├── README.md                 # 📖 Project documentation
├── PRD.md                   # 📋 Product Requirements Document
├── .gitignore               # 🚫 Git ignore rules
├── gui/                     # 🖥️ QML-based GUI components
│   ├── app.py              # QML application launcher
│   ├── backend.py          # Python backend managers
│   ├── bundle_manager.py   # Bundle management logic
│   └── qml/               # 📄 QML UI files
│       ├── main.qml       # Main application window
│       ├── bundleinfo.qml # Bundle info component
│       ├── serverlogviewer.qml # Log viewer component
│       └── qmldir        # QML module definition
├── server/                 # 🔧 Backend server
│   ├── main.py            # Server entry point
│   ├── config.py          # Configuration
│   ├── models.py          # Data models
│   ├── api/              # API endpoints
│   │   └── routes.py     # REST API endpoints
│   └── services/         # Business logic services
│       ├── bundle_service.py
│       ├── deployment_service.py
│       └── feedback_service.py
├── bundle/                 # 📦 Bundle files (.raucb)
├── certs/                  # 🔒 SSL certificates
├── logs/                   # 📝 Log files
└── scripts/                # 🔧 Utility scripts
```

## Architecture

The application uses a modern **QML-based architecture** with **automatic bundle detection**:

- **Main Entry Point** (`main.py`): Mode selection and application startup
- **QML Frontend** (`gui/qml/`): Declarative UI components with fixed 1280x720 layout
- **Python Backend** (`gui/backend.py`): Bridge between QML and server functionality
- **Bundle Management** (`gui/bundle_manager.py`): Automatic bundle scanning and latest detection
- **Server Layer** (`server/`): FastAPI-based backend with modular components
- **API Layer** (`server/api/`): HTTP endpoints and request handling
- **Service Layer** (`server/services/`): Business logic and operations

### QML Benefits

- **🔄 Easy Maintenance**: Separate QML files for different UI components
- **🎨 Declarative Design**: UI logic separated from business logic
- **📱 Fixed Layout**: Consistent 1280x720 window size for reliability
- **🔧 Modular**: Easy to add new components and modify existing ones
- **⚡ Performance**: Efficient rendering and real-time updates
- **🔄 Auto-refresh**: Automatic bundle scanning every 5 seconds

## Development

### Project Structure

- **QML Frontend**: Declarative UI with separate component files
- **Python Backend**: Clean separation between UI and business logic
- **Modular Design**: Easy to extend and maintain
- **Configuration**: Structured config with validation
- **Error Handling**: Custom exceptions and proper error reporting
- **Logging**: Structured logging with different output levels

### Building

```bash
# Install dependencies
uv sync

# Run in development mode
uv run main.py gui    # QML GUI mode
uv run main.py cli    # CLI mode
```

### QML Development

```bash
# Edit QML files for UI changes
gui/qml/main.qml              # Main application window (1280x720)
gui/qml/bundleinfo.qml        # Bundle information component
gui/qml/serverlogviewer.qml   # Log viewer component

# Python backend for business logic
gui/backend.py                # Server and deployment managers
gui/bundle_manager.py         # Bundle scanning and management
```

## Troubleshooting

### PyQt6/QML Installation Issues

```bash
# Install PyQt6 (includes QML support)
pip install PyQt6

# Or use uv
uv add PyQt6
```

### Server Connection Issues

1. Check if port 8080 is available: `netstat -tlnp | grep 8080`
2. Check firewall settings: `sudo ufw status`
3. Verify bundle directory exists: `ls -la bundle/`
4. Check logs in the GUI or `logs/` directory

### Bundle Not Detected

1. Ensure `.raucb` files are in `bundle/` directory
2. Check file permissions: `ls -la bundle/`
3. Wait for automatic refresh (every 5 seconds) or restart the application
4. Verify bundle files have `.raucb` extension

### QML Issues

1. Check QML syntax: `qmlscene gui/qml/main.qml`
2. Verify imports are correct
3. Check console for QML errors
4. Ensure PyQt6 is properly installed: `pip install PyQt6`

### File Dialog Limitation

The "Select Bundle" button currently shows informational messages instead of opening a file dialog due to QtQuick.Dialogs module limitations. Bundle files should be manually copied to the `bundle/` directory where they will be automatically detected.

## Production Deployment

- Use HTTPS/TLS with proper certificates
- Set `UPDATER_ENABLE_AUTH=true` with secure API keys
- Configure proper log rotation
- Use reverse proxy (nginx) for additional security
- Monitor server health and bundle deployments
- Ensure bundle directory has proper permissions
- Configure automatic bundle scanning for production environments

## Current Features

### ✅ **Implemented Features**
- **Fixed GUI Layout**: 1280x720 non-resizable window
- **Top-Bottom Design**: Bundle info on top, logs on bottom
- **Automatic Bundle Detection**: Scans every 5 seconds
- **Real-time Logging**: Enhanced log viewer with auto-scroll
- **Server Management**: Start/Stop with status indicators
- **Latest Bundle Display**: Static text showing bundle information
- **API Endpoints**: Complete REST API for device communication
- **Bundle Management**: Automatic latest bundle identification

### ⚠️ **Known Limitations**
- **File Dialog**: Not available (manual file management required)
- **QML Components**: Limited modularity due to import restrictions
- **Authentication**: Basic implementation only
- **Multi-device**: Single device target support

### 🎯 **Usage Summary**
- **GUI Mode**: `uv run main.py gui` - Modern QML interface
- **CLI Mode**: `uv run main.py cli` - Server-only mode
- **Bundle Management**: Copy `.raucb` files to `bundle/` directory
- **Automatic Detection**: Latest bundle automatically identified
- **Real-time Monitoring**: Live logs and server status 