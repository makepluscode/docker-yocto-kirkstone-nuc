# Testing Guide for Update Agent

This document describes the testing framework and procedures for the update-agent project.

## Overview

The project uses Google Test (gtest) framework for unit testing and integration testing. Tests are organized into separate files for different components:

- `test_config.cpp` - Configuration constants and settings
- `test_server_agent.cpp` - ServerAgent class functionality
- `test_service_agent.cpp` - ServiceAgent class functionality  
- `test_integration.cpp` - Integration tests and complete update flow

## Test Structure

### Unit Tests

#### Configuration Tests (`test_config.cpp`)
- Server configuration validation
- Timing configuration validation
- File path configuration validation
- Logging configuration validation
- Network configuration validation

#### ServerAgent Tests (`test_server_agent.cpp`)
- Constructor and initialization
- UpdateInfo structure validation
- JSON response parsing (valid/invalid/empty)
- Download functionality (with mock scenarios)
- Feedback sending (with mock scenarios)
- Error handling

#### ServiceAgent Tests (`test_service_agent.cpp`)
- Constructor and initialization
- D-Bus connection handling
- Bundle installation (with mock scenarios)
- Status and information retrieval
- Callback functionality
- Error handling

### Integration Tests (`test_integration.cpp`)
- Complete update flow simulation
- Error handling flow
- Configuration integration
- Callback integration
- Resource management
- Thread safety (basic)

## Running Tests

### Simple Test Execution

```bash
./test.sh
```

This script will:
1. Source Yocto SDK environment
2. Configure CMake with cross-compilation toolchain
3. Build the project and tests
4. Run all tests using CTest

**Prerequisites:**
- Yocto SDK installed at `/usr/local/oecore-x86_64/`
- Same environment as `build.sh`

### Manual Testing

You can also run tests manually:

```bash
# Source Yocto SDK environment
unset LD_LIBRARY_PATH
source /usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux

# Build tests
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="/usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake" \
         -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run tests
ctest --output-on-failure --verbose

# Or run test binary directly
./tests/update-agent-tests
```

## Test Categories

### Network-Dependent Tests
Tests that require network connectivity (marked as expected to fail in test environment):
- `ServerAgentTest.PollForUpdates`
- `ServerAgentTest.DownloadBundle*`
- `ServerAgentTest.SendFeedback*`
- `ServerAgentTest.SendProgressFeedback`
- `ServerAgentTest.SendStartedFeedback`
- `ServerAgentTest.SendFinishedFeedback`

### D-Bus-Dependent Tests
Tests that require D-Bus service (marked as expected to fail in test environment):
- `ServiceAgentTest.ConnectWithoutService`
- `ServiceAgentTest.InstallBundle*`
- `ServiceAgentTest.GetStatus*`
- `ServiceAgentTest.MarkGood*`
- `ServiceAgentTest.MarkBad*`

### Mock Tests
Tests that work with mock data and don't require external services:
- All configuration tests
- JSON parsing tests
- Structure validation tests
- Error handling tests

## Test Environment

### Local Development
- Uses host system libraries
- Can run tests immediately after build
- Requires development packages installed

## Continuous Integration

For CI/CD integration, use:

```bash
./test.sh
```

The test script returns appropriate exit codes for CI systems.

## Test Coverage

Current test coverage includes:

- ✅ Configuration validation
- ✅ JSON parsing and validation
- ✅ Error handling scenarios
- ✅ Constructor and initialization
- ✅ Callback functionality
- ✅ Resource management
- ✅ Integration flow simulation

Areas that could benefit from additional testing:
- Network timeout scenarios
- D-Bus signal handling
- File system operations
- Concurrent operations
- Memory leak detection

## Debugging Tests

### Verbose Output
```bash
ctest --output-on-failure --verbose
```

### Individual Test Execution
```bash
./build/tests/update-agent-tests --gtest_filter="ConfigTest.*"
./build/tests/update-agent-tests --gtest_filter="ServerAgentTest.*"
./build/tests/update-agent-tests --gtest_filter="ServiceAgentTest.*"
./build/tests/update-agent-tests --gtest_filter="IntegrationTest.*"
```

### Test Output to File
```bash
./build/tests/update-agent-tests --gtest_output=xml:test_results.xml
```

## Adding New Tests

1. Create test file in `tests/` directory
2. Add test file to `tests/CMakeLists.txt`
3. Follow existing test patterns
4. Use appropriate test categories (unit/integration)
5. Handle network/D-Bus dependencies appropriately
6. Update this documentation

## Troubleshooting

### Common Issues

1. **Missing packages**: Install required development packages
2. **Cross-compilation issues**: Ensure Yocto SDK is properly sourced
3. **Network tests failing**: Expected in test environment
4. **D-Bus tests failing**: Expected without D-Bus service
5. **Permission issues**: Ensure test scripts are executable

### Debug Commands

```bash
# Check package availability
pkg-config --exists automotive-dlt
pkg-config --exists dbus-1
pkg-config --exists libcurl
pkg-config --exists json-c

# Check test binary
file build/tests/update-agent-tests
ldd build/tests/update-agent-tests
```