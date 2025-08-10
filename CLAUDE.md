# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Docker-based Yocto build environment for Intel NUC using the Kirkstone (Yocto 4.0 LTS) release. The project creates custom Linux images with Qt5 support, RAUC A/B boot system, and industrial-grade features for Intel NUC hardware.

## Key Architecture

### Layer Structure
- **meta-nuc**: Custom layer for Intel NUC specific configurations, images, and recipes
- **meta-apps**: Application layer containing dashboard and other custom applications
- **meta-rauc**: RAUC (Robust Auto-Update Client) integration for A/B boot functionality
- **meta-qt5**: Qt5 framework support for GUI applications
- **meta-intel**: Intel BSP layer for hardware support
- **meta-openembedded**: Additional OE layers (oe, python, networking)

### Target Images
- `nuc-image`: Base image for Intel NUC
- `nuc-image-qt5`: Qt5-enabled image with GUI support
- `nuc-bundle`: RAUC update bundle for A/B boot updates

### Boot System
The project uses RAUC with GRUB for A/B boot functionality on Intel NUC hardware with SATA SSD storage.

## Common Development Commands

### Initial Setup
```bash
./download.sh          # Download Yocto layers (first time only)
```

### Building Images
```bash
# Quick auto build (recommended)
./build.sh              # Auto mode: clean, configure, build nuc-image-qt5
./build.sh auto         # Same as above

# Manual development mode
./build.sh manual       # Enter container for manual operations
```

### Inside Container (Manual Mode)
```bash
# Build specific images
bitbake nuc-image-qt5   # Build Qt5 image
bitbake nuc-image       # Build base image
bitbake nuc-bundle      # Build RAUC update bundle

# Development tasks
bitbake -c menuconfig virtual/kernel  # Configure kernel
bitbake -c cleansstate dashboard      # Clean dashboard recipe
```

### Flashing and Deployment
```bash
./flash.sh              # Flash built image to USB device
./deploy_dashboard.sh   # Deploy dashboard application
```

### Container Management
```bash
./docker.sh manual  # Enter existing container
./clean.sh              # Remove container and clean build configs
```

## Key Configuration Files

### Build Configuration
- `kirkstone/meta-nuc/conf/local.conf.sample`: Main build configuration template
- `kirkstone/build/conf/local.conf`: Active build configuration (auto-generated from template)
- `kirkstone/build/conf/bblayers.conf`: Layer configuration (auto-generated)

### RAUC Configuration
- `kirkstone/meta-nuc/recipes-core/rauc/files/intel-corei7-64/system.conf`: RAUC system configuration

### Machine Settings
- **MACHINE**: `intel-corei7-64`
- **Target Storage**: SATA SSD (`sda`)
- **Boot Method**: GRUB EFI with A/B partitions

## Development Workflow

1. **Layer Development**: Custom recipes go in `meta-nuc` or `meta-apps`
2. **External Source**: Dashboard app source in `kirkstone/local/dashboard` (mounted from host)
3. **Build Process**: Uses Docker container with Yocto environment
4. **Image Output**: Built images in `kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/`

## Important Notes

- **Build Directory**: `kirkstone/build` is excluded from git but persists build state
- **Downloads Cache**: `kirkstone/downloads` and `kirkstone/sstate-cache` for faster rebuilds
- **Container Persistence**: Docker container `yocto-nuc` maintains state between runs
- **Qt Configuration**: Uses EGL/KMS/GBM for hardware-accelerated graphics
- **User Account**: Default user `nuc` with password `nuc123`

## Docker Environment

The build environment runs in a Ubuntu-based Docker container with all Yocto dependencies pre-installed. The container mounts the `kirkstone` directory for persistent storage of builds, downloads, and source code.

## Dashboard Application Development

### Dashboard Structure
The dashboard application (`kirkstone/local/dashboard`) is a Qt5/QML system monitoring application with the following key components:
- **C++ Backend**: `src/` - System info collection, RAUC manager, GRUB manager  
- **QML Frontend**: `qml/` - Dashboard cards and UI components
- **Resources**: Qt resource files, systemd services, configuration

### Dashboard Build Commands
```bash
# Build dashboard only (from dashboard directory)
cd kirkstone/local/dashboard
./build.sh

# Build dashboard only (from project root)
./build.sh dashboard

# Manual dashboard build
cd kirkstone/local/dashboard
unset LD_LIBRARY_PATH
source /usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="/usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake"
make -j$(nproc)
```

### Dashboard Deployment
```bash
cd kirkstone/local/dashboard

# Deploy to default target (192.168.1.100)
./deploy.sh

# Deploy to specific target
./deploy.sh root@192.168.1.50

# Manual network setup (if needed)
./connect.sh
```

### Dashboard Architecture
- **SystemInfo**: Real-time CPU, memory, network, storage monitoring
- **RaucManager**: RAUC A/B boot management via D-Bus
- **GrubManager**: GRUB configuration and boot slot management
- **Qt5 EGLFS**: Hardware-accelerated graphics pipeline

## Bundle Deployment Workflow

### RAUC Bundle Creation and Deployment
```bash
# Build RAUC update bundle
./build.sh bundle

# Deploy bundle to target
./send_bundle.sh                    # Default: 192.168.1.100
./send_bundle.sh 192.168.1.150     # Custom target
./send_bundle.sh root@192.168.1.100 --skip-network

# Install on target device
sudo rauc install /data/nuc-image-qt5-bundle-intel-corei7-64.raucb
```

## Network Configuration

### Development Network Setup
- **Host Interface**: `enp42s0`
- **Host IP**: `192.168.1.101`
- **Target IP**: `192.168.1.100` (default)
- **Network**: `255.255.255.0`

### Connection Scripts
```bash
./connect.sh        # Setup network interface and SSH keys
./dlt-receive.sh    # Connect to DLT logging on target
```