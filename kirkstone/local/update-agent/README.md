# RAUC Hawkbit C++ Client

A C++ application that integrates RAUC (Robust Auto-Update Controller) with Eclipse Hawkbit for over-the-air updates.

## Features

- Polls Hawkbit server for available updates
- Downloads update bundles from Hawkbit
- Installs updates using RAUC via DBus
- Sends feedback to Hawkbit about update status
- Comprehensive DLT logging
- Systemd service integration

## Dependencies

- DLT (Diagnostic Log and Trace)
- DBus (for RAUC communication)
- libcurl (for HTTP communication with Hawkbit)
- json-c (for JSON parsing)

## Configuration

The application uses the following configuration (currently hardcoded, should be made configurable):

- Hawkbit server URL: `https://hawkbit.example.com`
- Tenant: `DEFAULT`
- Controller ID: `nuc-device-001`
- Poll interval: 30 seconds

## Building

The application is built using CMake and integrated into the Yocto build system via the `meta-apps` layer.

## Service

The application runs as a systemd service named `rauc-hawkbit-cpp.service` and starts automatically after boot.

## Logging

The application uses DLT for logging with the following contexts:
- `HAWK`: Hawkbit client operations
- `RAUC`: RAUC client operations

## Usage

The service starts automatically after boot. To manually start/stop:

```bash
systemctl start rauc-hawkbit-cpp
systemctl stop rauc-hawkbit-cpp
systemctl status rauc-hawkbit-cpp
```

## Testing

The project includes comprehensive Google Test (gtest) framework integration with the following test categories:

### Test Structure
- **Configuration Tests**: Validate all configuration constants and settings
- **ServerAgent Tests**: Test Hawkbit server communication and JSON parsing
- **ServiceAgent Tests**: Test RAUC D-Bus communication and bundle installation
- **Integration Tests**: Test complete update flow and error handling

### Running Tests

```bash
./test.sh
```

This will:
1. Source Yocto SDK environment
2. Build the project and tests with cross-compilation
3. Run all tests

For detailed testing information, see [tests/TESTING.md](tests/TESTING.md).

## Development

The application consists of:
- `main.cpp`: Main application loop and integration
- `server_agent.h/cpp`: Hawkbit server communication
- `service_agent.h/cpp`: RAUC D-Bus communication
- `config.h`: Configuration constants

### Build Scripts
- `build.sh`: Cross-compilation build with Yocto SDK
- `test.sh`: Local test execution
- `deploy.sh`: Application deployment to target device
