# RAUC Hawkbit C++ Client - Build Instructions

## Overview

The `rauc-hawkbit-cpp` application has been successfully created and integrated into the Yocto build system. This application provides integration between RAUC (Robust Auto-Update Controller) and Eclipse Hawkbit for over-the-air updates.

## Application Structure

```
kirkstone/local/rauc-hawkbit-cpp/
├── CMakeLists.txt              # CMake build configuration
├── src/
│   ├── main.cpp               # Main application entry point
│   ├── hawkbit_client.h       # Hawkbit client header
│   ├── hawkbit_client.cpp     # Hawkbit client implementation
│   ├── rauc_client.h          # RAUC client header
│   └── rauc_client.cpp        # RAUC client implementation
├── services/
│   └── rauc-hawkbit-cpp.service # Systemd service file
├── build.sh                   # Local build script
├── deploy.sh                  # Local deployment script
├── dlt-receive.sh             # DLT logging script
└── README.md                  # Application documentation
```

## Yocto Integration

The application has been integrated into the Yocto build system:

- **Recipe**: `kirkstone/meta-apps/recipes-apps/rauc-hawkbit-cpp/rauc-hawkbit-cpp_1.0.bb`
- **Image**: Added to `kirkstone/meta-nuc/recipes-core/images/nuc-image-qt5.bb`
- **Dependencies**: DLT, DBus, libcurl, json-c, RAUC

## Building with Yocto

To build the application as part of the Yocto image:

```bash
cd kirkstone
source poky/oe-init-build-env
bitbake nuc-image-qt5
```

The application will be included in the final image and will start automatically after boot.

## Local Development

For local development and testing:

1. **Install dependencies** (on Ubuntu/Debian):
   ```bash
   sudo apt-get install build-essential cmake pkg-config \
     libdlt-dev libdbus-1-dev libcurl4-openssl-dev libjson-c-dev
   ```

2. **Build locally**:
   ```bash
   cd kirkstone/local/rauc-hawkbit-cpp
   ./build.sh
   ```

3. **Deploy locally** (requires sudo):
   ```bash
   ./deploy.sh
   ```

## Features

- **Hawkbit Integration**: Polls Hawkbit server for updates and sends feedback
- **RAUC Integration**: Uses DBus to communicate with RAUC service
- **DLT Logging**: Comprehensive logging with DLT framework
- **Systemd Service**: Runs as a systemd service with automatic restart
- **HTTP Communication**: Uses libcurl for HTTP requests to Hawkbit
- **JSON Processing**: Uses json-c for JSON payload handling

## Configuration

The application currently uses hardcoded configuration values:
- Hawkbit server URL: `https://hawkbit.example.com`
- Tenant: `DEFAULT`
- Controller ID: `nuc-device-001`
- Poll interval: 30 seconds

These should be made configurable via environment variables or configuration files in future versions.

## Service Management

The application runs as a systemd service:

```bash
# Check service status
systemctl status rauc-hawkbit-cpp

# Start/stop service
systemctl start rauc-hawkbit-cpp
systemctl stop rauc-hawkbit-cpp

# View logs
journalctl -u rauc-hawkbit-cpp -f

# View DLT logs
./dlt-receive.sh
```

## Dependencies

- **Build Dependencies**: dlt-daemon, cmake-native, pkgconfig-native, dbus, curl, json-c, rauc
- **Runtime Dependencies**: rauc, dbus, curl, json-c

## Next Steps

1. **Configuration**: Make the Hawkbit server configuration configurable
2. **Error Handling**: Improve error handling and recovery mechanisms
3. **Testing**: Add unit tests and integration tests
4. **Documentation**: Add API documentation and usage examples
5. **Security**: Implement proper authentication and security measures
