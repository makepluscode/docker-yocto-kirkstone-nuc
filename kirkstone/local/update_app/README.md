# Update App

Simple C++ sample application that prints "hello, update" every 2 seconds with DLT logging support.

## Features

- Prints "hello, update" message every 2 seconds to stdout  
- DLT logging with sample messages
- Auto-starts on boot via systemd
- Built with C++14 and CMake

## Build with SDK

### Prerequisites

Setup the Yocto SDK toolchain (see doc/03-toolchain-build-guide.md):

```bash
# Source the SDK environment
source /opt/poky/4.0.4/environment-setup-corei7-64-poky-linux
```

### Build

```bash
# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/poky/4.0.4/sysroots/x86_64-pokysdk-linux/usr/share/cmake/OEToolchainConfig.cmake

# Build
make -j$(nproc)
```

## Deploy

```bash
# Deploy to target (default: 192.168.1.100)
./deploy.sh

# Deploy to custom target
./deploy.sh root@192.168.1.100
```

## Monitor Logs

```bash
# Monitor DLT logs
./dlt-receive.sh

# Monitor DLT logs from custom target
./dlt-receive.sh 192.168.1.100
```