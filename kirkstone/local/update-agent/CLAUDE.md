# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a C++ client application that integrates RAUC (Robust Auto-Update Controller) with Eclipse Hawkbit for over-the-air updates. The application polls a Hawkbit server for available updates, downloads update bundles, and installs them using RAUC via D-Bus.

## Architecture

### Core Components
- **main.cpp**: Main application loop with polling and update orchestration
- **hawkbit_client.h/cpp**: HTTP client for Hawkbit server communication (polling, feedback, downloads)
- **rauc_client.h/cpp**: D-Bus client for RAUC service communication (installation, status)
- **config.h**: Centralized configuration constants

### Dependencies
- **DLT**: Diagnostic Log and Trace framework for logging
- **D-Bus**: Inter-process communication with RAUC daemon
- **libcurl**: HTTP client library for Hawkbit communication
- **json-c**: JSON parsing and generation

## Build Commands

### Development Build (Cross-compilation for target)
```bash
./build.sh              # Build with Yocto SDK cross-compiler

# Manual build
unset LD_LIBRARY_PATH
source /usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="/usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake"
make -j$(nproc)
```

### Local Development Build (Host system)
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake pkg-config \
  libdlt-dev libdbus-1-dev libcurl4-openssl-dev libjson-c-dev

# Build locally
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Yocto Integration Build
The application is integrated into the main Yocto build system via `meta-apps/recipes-apps/rauc-hawkbit-cpp/rauc-hawkbit-cpp_1.0.bb`. Build as part of the full image:
```bash
cd ../../..  # Return to kirkstone directory
source poky/oe-init-build-env
bitbake nuc-image-qt5
```

## Deployment

### Local Deployment to Target
```bash
./deploy.sh             # Deploy to default target (192.168.1.100)
./deploy.sh root@192.168.1.150  # Deploy to specific target
```

### Service Management
```bash
# Service control
systemctl start rauc-hawkbit-cpp
systemctl stop rauc-hawkbit-cpp
systemctl status rauc-hawkbit-cpp

# View logs
journalctl -u rauc-hawkbit-cpp -f
./dlt-receive.sh         # DLT log viewer
```

## Configuration

Configuration is currently hardcoded in `src/config.h`:
- **Hawkbit Server**: `HAWKBIT_SERVER_URL`, `HAWKBIT_TENANT`, `HAWKBIT_CONTROLLER_ID`
- **Timing**: `POLL_INTERVAL_SECONDS`, various timeout values
- **Paths**: `BUNDLE_DOWNLOAD_PATH`, `LOG_FILE_PATH`
- **DLT Contexts**: `DLT_HAWK_CONTEXT` (HAWK), `DLT_RAUC_CONTEXT` (RAUC), `DLT_UPDATE_CONTEXT` (UPDT)

## Development Workflow

1. **Code Changes**: Modify source files in `src/`
2. **Build**: Run `./build.sh` for cross-compilation or local cmake build
3. **Deploy**: Use `./deploy.sh` to deploy to target device
4. **Test**: Monitor via `journalctl` or DLT logs
5. **Integrate**: Changes are automatically included in Yocto builds

## Key Integration Points

### RAUC Integration
- Uses D-Bus to communicate with update-service broker (`org.freedesktop.UpdateService`) which forwards to RAUC
- Calls `InstallBundle()` method for updates
- Monitors installation progress via D-Bus signals
- Handles installation completion and error reporting

### Hawkbit Integration
- Implements Hawkbit DMF (Device Management Federation) API
- Polls `/controller/v1/{tenant}/{controllerId}` endpoint
- Downloads artifacts from `/controller/v1/{tenant}/{controllerId}/deploymentBase/{id}/download`
- Sends feedback to `/controller/v1/{tenant}/{controllerId}/deploymentBase/{id}/feedback`

### Logging Strategy
- Uses DLT with separate contexts for different subsystems
- HAWK context: Hawkbit communication and HTTP operations  
- RAUC context: RAUC D-Bus operations and bundle installation
- UPDT context: Update orchestration and state management

## Important Notes

- **Network Configuration**: Default target IP is 192.168.1.100, Hawkbit server at 192.168.1.101:8080
- **Security**: SSL verification is disabled by default (`ENABLE_SSL_VERIFICATION = false`)
- **Cross-compilation**: Always unset `LD_LIBRARY_PATH` before sourcing Yocto SDK environment
- **D-Bus Communication**: Requires RAUC service to be running on target system
- **Bundle Storage**: Downloaded bundles are stored at `/tmp/hawkbit_update.raucb`