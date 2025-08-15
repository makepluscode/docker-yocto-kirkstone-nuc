# Updater - Integrated OTA Update Management

A comprehensive OTA (Over-The-Air) update management solution with integrated GUI and server.

## Features

### 🖥️ Integrated GUI Application
- **Auto Server Management**: Automatically starts/stops backend server
- **Real-time Monitoring**: Live deployment status and server health
- **Deployment Control**: Enable/disable deployments with one click
- **Activity Logs**: Real-time server and operation logging
- **Clean Interface**: Modern Qt6-based user interface

### 🔧 Backend Server
- **Poll Endpoint**: Device update checking
- **Feedback Endpoint**: Update progress tracking  
- **Bundle Download**: Efficient file serving
- **Admin API**: Deployment management
- **Bundle Discovery**: Automatic .raucb file detection

## Quick Start

### 1. Install Dependencies

```bash
# Run the dependency installer
./scripts/install_deps.sh

# Or manually install
sudo apt install python3-pyqt6 python3-requests
uv sync  # or pip3 install -r requirements.txt
```

### 2. Run the Application

```bash
# Easy run (automatic mode selection)
./run.sh

# Or run GUI directly (if PyQt6 is installed)
./updater.py

# Or with Python
python3 updater.py
```

### 3. Add Bundles

```bash
# Copy your .raucb files to the bundle directory
cp your-bundle.raucb bundle/

# Bundles are automatically detected and available for deployment
```

## Usage

### GUI Application

The integrated `updater.py` provides:
- **Dashboard**: Server status and controls
- **Deployments**: Manage active/inactive deployments  
- **Logs**: Real-time activity monitoring

Features:
- ✅ **Auto Server Start**: Server starts when GUI launches
- ✅ **Auto Server Stop**: Server stops when GUI exits
- ✅ **Connection Monitoring**: Real-time server health checking
- ✅ **Deployment Management**: Enable/disable deployments
- ✅ **Live Logging**: Server output and application events

### Server Only (Headless)

```bash
# Run server only (no GUI)
./server.sh

# Or with HTTPS
export UPDATER_ENABLE_HTTPS=true
./server.sh

# Or use refactored server
uv run python -m updater_server.main_refactored
```

## API Endpoints

- `GET /{tenant}/controller/v1/{controller_id}` - Poll for updates
- `POST /{tenant}/controller/v1/{controller_id}/deploymentBase/{execution_id}/feedback` - Send feedback  
- `GET /download/{filename}` - Download bundle files
- `GET /admin/deployments` - List deployments
- `POST /admin/deployments` - Create deployment
- `PUT /admin/deployments/{id}` - Update deployment
- `DELETE /admin/deployments/{id}` - Delete deployment

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
./generate_certs.sh

# Enable HTTPS
export UPDATER_ENABLE_HTTPS=true
./updater.py  # GUI with HTTPS
./server.sh   # Server only with HTTPS
```

## File Organization

```
updater/
├── updater.py              # 🎯 Main integrated application
├── run.sh                  # 🚀 Easy launcher (automatic mode selection)
├── server.sh               # Server-only launcher
├── build.sh                # Build script
├── generate_certs.sh       # SSL certificate generator
├── bundle/                 # 📦 Bundle files (.raucb)
├── certs/                  # 🔒 SSL certificates
├── updater_server/         # 🔧 Backend server code
├── scripts/                # 🛠️ Utility scripts
└── logs/                   # 📝 Log files
```

## Architecture

The application uses a layered architecture:

- **GUI Layer** (`updater.py`): PyQt6-based interface with integrated server management
- **API Layer** (`updater_server/api/`): HTTP endpoints and request handling
- **Service Layer** (`updater_server/services/`): Business logic and operations
- **Storage Layer** (`updater_server/storage.py`): Data persistence and file management

See `REFACTORING_GUIDE.md` for detailed architecture documentation.

## Development

### Project Structure

- **Integrated App**: Single executable with GUI + server management
- **Refactored Backend**: Modular, maintainable server architecture  
- **Configuration**: Structured config with validation
- **Error Handling**: Custom exceptions and proper error reporting
- **Logging**: Structured logging with different output levels

### Building

```bash
# Build all components
./build.sh

# Build specific components
./build.sh backend
./build.sh certs

# Install system dependencies
./build.sh install
```

## Troubleshooting

### PyQt6 Installation Issues

```bash
# Install system PyQt6 package (recommended)
sudo apt install python3-pyqt6

# Or use the installer script
./scripts/install_deps.sh

# Or try with pip (may have permission issues)
pip3 install PyQt6
```

### Server Connection Issues

1. Check if port 8080 is available: `netstat -tlnp | grep 8080`
2. Check firewall settings: `sudo ufw status`
3. Verify bundle directory exists: `ls -la bundle/`
4. Check logs in the GUI or `logs/` directory

### Bundle Not Detected

1. Ensure `.raucb` files are in `bundle/` directory
2. Check file permissions: `ls -la bundle/`
3. Restart the application to refresh bundle list

## Production Deployment

- Use HTTPS/TLS with proper certificates
- Set `UPDATER_ENABLE_AUTH=true` with secure API keys
- Configure proper log rotation
- Use reverse proxy (nginx) for additional security
- Monitor server health and bundle deployments 