# Product Requirements Document (PRD)
# Updater - OTA Update Management System

## 📋 **Executive Summary**

The Updater is a comprehensive OTA (Over-The-Air) update management system designed for embedded Linux devices, specifically targeting Intel NUC devices running Yocto-based systems. The application provides both GUI and CLI interfaces for managing bundle deployments, server control, and real-time monitoring.

## 🎯 **Product Vision**

Create a robust, user-friendly OTA update management system that simplifies the deployment and management of software updates for embedded devices, with emphasis on reliability, ease of use, and real-time monitoring capabilities.

## 🏗️ **Architecture Overview**

### **System Components**
- **GUI Interface**: PyQt6-based QML application with real-time monitoring
- **Backend Server**: FastAPI-based REST API server
- **Bundle Management**: Automated bundle scanning and deployment
- **Logging System**: Real-time log management and display
- **Target Communication**: HTTP-based update distribution

### **Technology Stack**
- **Frontend**: PyQt6, QML
- **Backend**: FastAPI, Python 3.9+
- **Build System**: uv (Python package manager)
- **Deployment**: PyInstaller (for Windows deployment)
- **Communication**: HTTP REST API

## 📐 **User Interface Requirements**

### **GUI Layout Specifications**
- **Window Size**: Fixed 1280x720 pixels (non-resizable)
- **Layout**: Top-bottom arrangement
  - **Top Panel**: Bundle information display (200px height)
  - **Bottom Panel**: Log viewer (remaining space)
- **Theme**: Modern dark/light theme with consistent color scheme

### **GUI Components**

#### **Header Section**
- Application title with icon
- Server status indicator with real-time animation
- Start/Stop server controls
- Server status text display

#### **Bundle Information Panel**
- Latest bundle display with static text
- Bundle details: Name, Version, Size, Creation Date
- "Select Bundle" button (informational)
- Automatic bundle refresh (every 5 seconds)

#### **Log Viewer Panel**
- Real-time log display with auto-scroll
- Monospace font for log readability
- Clear logs functionality
- Auto-scroll toggle option

### **Color Scheme**
```css
Primary Color: #2c3e50
Secondary Color: #34495e
Accent Color: #3498db
Success Color: #27ae60
Warning Color: #f39c12
Error Color: #e74c3c
Background Color: #ecf0f1
Text Color: #2c3e50
```

## 🔧 **Functional Requirements**

### **1. Server Management**
- **Start Server**: Initialize FastAPI server on port 8080
- **Stop Server**: Gracefully shutdown server
- **Server Status**: Real-time status monitoring
- **Network Binding**: Bind to 0.0.0.0 for network accessibility

### **2. Bundle Management**
- **Automatic Scanning**: Scan bundle directory every 5 seconds
- **Latest Detection**: Automatically identify latest bundle by modification time
- **Bundle Information**: Display name, version, size, creation date
- **File Support**: Support for .raucb bundle files

### **3. Deployment Management**
- **Deployment Creation**: Create deployments from latest bundle
- **Deployment Listing**: View all active deployments
- **Deployment Toggle**: Enable/disable deployments
- **API Integration**: REST API for deployment management

### **4. Logging System**
- **Real-time Logging**: Live log updates
- **Log Persistence**: Maintain last 100 log entries
- **Log Clearing**: Clear log history
- **Timestamp Format**: [HH:MM:SS] format for log entries

### **5. Target Device Communication**
- **Poll Endpoint**: `/DEFAULT/controller/v1/{device-id}`
- **Download Endpoint**: `/DEFAULT/download/{filename}`
- **Status Codes**: HTTP 200 (update available), HTTP 204 (no update)
- **Device Support**: Support for multiple device IDs

## 📁 **File Structure**

```
updater/
├── main.py                    # Application entry point
├── pyproject.toml            # Project configuration
├── README.md                 # Project documentation
├── .gitignore               # Git ignore rules
├── bundle/                  # Bundle storage directory
├── certs/                   # SSL certificates
├── logs/                    # Application logs
├── scripts/                 # Utility scripts
├── gui/                     # GUI application
│   ├── app.py              # Main GUI application
│   ├── backend.py          # Python backend managers
│   ├── bundle_manager.py   # Bundle management logic
│   └── qml/               # QML interface files
│       ├── main.qml       # Main application window
│       ├── bundleinfo.qml # Bundle info component
│       ├── serverlogviewer.qml # Log viewer component
│       └── qmldir        # QML module definition
└── server/                 # Backend server
    ├── main.py            # FastAPI server entry point
    ├── config.py          # Server configuration
    ├── models.py          # Data models
    ├── api/              # API routes
    │   └── routes.py     # REST API endpoints
    └── services/         # Business logic services
        ├── bundle_service.py
        ├── deployment_service.py
        └── feedback_service.py
```

## 🔌 **API Endpoints**

### **Server Status**
- `GET /` - Server status and version information
- `GET /health` - Health check endpoint

### **Deployment Management**
- `GET /admin/deployments` - List all deployments
- `POST /admin/deployments` - Create new deployment
- `PUT /admin/deployments/{id}` - Update deployment
- `DELETE /admin/deployments/{id}` - Delete deployment

### **Target Device Endpoints**
- `GET /DEFAULT/controller/v1/{device-id}` - Poll for updates
- `GET /DEFAULT/download/{filename}` - Download bundle file

## 🚀 **Usage Modes**

### **GUI Mode (Default)**
```bash
uv run main.py gui
# or
uv run main.py
```

### **CLI Mode**
```bash
uv run main.py cli
```

### **Development Setup**
```bash
uv sync          # Install dependencies
uv run main.py   # Run application
```

## 🔄 **Automatic Features**

### **Bundle Auto-Refresh**
- **Interval**: Every 5 seconds
- **Trigger**: Automatic timer-based scanning
- **Detection**: Latest bundle by modification time
- **Display**: Real-time updates in GUI

### **Server Auto-Start**
- **Mode**: Manual start/stop via GUI
- **Port**: 8080 (configurable)
- **Binding**: 0.0.0.0 for network access
- **Status**: Real-time status monitoring

### **Log Auto-Scroll**
- **Behavior**: Automatically scroll to latest log entry
- **Toggle**: User can disable auto-scroll
- **Performance**: Maintains last 100 log entries

## 🛠️ **Technical Implementation**

### **QML Components**
- **ApplicationWindow**: Main application container
- **ColumnLayout**: Top-bottom layout arrangement
- **Rectangle**: Panel containers with styling
- **TextArea**: Log display with auto-scroll
- **Button**: Interactive controls
- **Timer**: Auto-refresh functionality

### **Python Backend**
- **QObject**: Base class for backend managers
- **pyqtSignal**: Signal-based communication
- **pyqtSlot**: QML-callable methods
- **QTimer**: Periodic task scheduling
- **QQmlApplicationEngine**: QML engine integration

### **Server Architecture**
- **FastAPI**: Modern Python web framework
- **Pydantic**: Data validation and serialization
- **Uvicorn**: ASGI server
- **SQLite**: Local data storage (if needed)

## 🔒 **Security Considerations**

### **Network Security**
- **HTTPS Support**: Configurable SSL/TLS
- **Certificate Management**: SSL certificate generation
- **Access Control**: Basic authentication (future enhancement)

### **File Security**
- **Bundle Validation**: File integrity checks
- **Path Sanitization**: Prevent directory traversal
- **File Permissions**: Secure file access

## 📊 **Performance Requirements**

### **Response Times**
- **GUI Startup**: < 3 seconds
- **Bundle Scanning**: < 1 second
- **Server Start**: < 2 seconds
- **Log Updates**: Real-time (< 100ms)

### **Resource Usage**
- **Memory**: < 100MB RAM
- **CPU**: < 5% during idle
- **Disk**: Minimal (logs + bundle storage)

## 🧪 **Testing Strategy**

### **Unit Testing**
- **Backend Services**: Python unit tests
- **API Endpoints**: FastAPI test client
- **QML Components**: Manual testing

### **Integration Testing**
- **GUI-Server Integration**: End-to-end testing
- **Bundle Management**: File system integration
- **Target Communication**: HTTP client testing

### **Manual Testing**
- **GUI Functionality**: Manual UI testing
- **Server Operations**: Manual server testing
- **Bundle Deployment**: Manual deployment testing

## 🚀 **Deployment Options**

### **Windows Deployment**
- **PyInstaller**: Create standalone executable
- **Single File**: Self-contained application
- **No Dependencies**: All dependencies bundled

### **Linux Deployment**
- **Package Manager**: uv-based installation
- **System Integration**: Desktop integration
- **Service Mode**: Systemd service (future)

### **Development Deployment**
- **Local Development**: Direct Python execution
- **Hot Reload**: Development mode with auto-reload
- **Debug Mode**: Enhanced logging and debugging

## 📈 **Future Enhancements**

### **Planned Features**
- **File Dialog**: Native file selection dialog
- **Advanced Authentication**: User management system
- **Database Integration**: Persistent data storage
- **Web Interface**: Browser-based management
- **Multi-device Support**: Multiple target management

### **Performance Improvements**
- **Async Operations**: Non-blocking UI operations
- **Caching**: Bundle metadata caching
- **Compression**: Bundle compression support
- **Delta Updates**: Incremental update support

### **Security Enhancements**
- **Digital Signatures**: Bundle signature verification
- **Encryption**: Bundle encryption support
- **Access Control**: Role-based access control
- **Audit Logging**: Comprehensive audit trails

## 📝 **Documentation**

### **User Documentation**
- **Installation Guide**: Step-by-step setup
- **User Manual**: Feature usage guide
- **Troubleshooting**: Common issues and solutions

### **Developer Documentation**
- **API Reference**: Complete API documentation
- **Architecture Guide**: System design documentation
- **Contributing Guide**: Development guidelines

## ✅ **Current Status**

### **Completed Features**
- ✅ GUI application with QML interface
- ✅ FastAPI backend server
- ✅ Bundle management system
- ✅ Real-time logging system
- ✅ Automatic bundle detection
- ✅ Server start/stop functionality
- ✅ Target device communication
- ✅ Deployment management API
- ✅ Fixed window layout (1280x720)
- ✅ Top-bottom GUI layout
- ✅ Auto-refresh functionality

### **Known Limitations**
- ⚠️ File dialog not available (manual file management)
- ⚠️ Limited QML component modularity
- ⚠️ Basic authentication only
- ⚠️ Single-device target support

### **Testing Status**
- ✅ GUI functionality tested
- ✅ Server functionality tested
- ✅ Bundle management tested
- ✅ Target communication tested
- ✅ Auto-refresh tested

## 🎯 **Success Metrics**

### **Functional Metrics**
- **Bundle Detection**: 100% accuracy
- **Server Reliability**: 99.9% uptime
- **GUI Responsiveness**: < 100ms response time
- **Log Accuracy**: Real-time log updates

### **User Experience Metrics**
- **Ease of Use**: Intuitive interface
- **Learning Curve**: < 5 minutes to basic operation
- **Error Handling**: Clear error messages
- **Performance**: Smooth operation

### **Technical Metrics**
- **Code Quality**: Clean, maintainable code
- **Documentation**: Comprehensive documentation
- **Testing Coverage**: > 80% test coverage
- **Performance**: Efficient resource usage

---

**Document Version**: 1.0  
**Last Updated**: August 16, 2025  
**Status**: Implementation Complete  
**Next Review**: September 16, 2025 