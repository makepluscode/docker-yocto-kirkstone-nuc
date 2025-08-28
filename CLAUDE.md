# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 언어 및 응답 스타일 설정 (Korean Response Configuration)

### 언어 규칙
- **필수**: 사용자가 영어로 질문해도 항상 한국어로만 응답해야 합니다
- **예외 없음**: 코드 주석, 변수명, 함수명 등 기술적 내용도 한국어로 설명
- **일관성**: 모든 세션에서 동일하게 적용

### 응답 상세도
- **계획 설명**: 작업 전 반드시 계획과 과정을 상세히 설명
- **단계별 설명**: 각 작업 단계를 명확하게 구분하여 설명
- **이유 포함**: 왜 그런 방법을 사용하는지 근거와 이유를 함께 설명
- **과정 투명화**: 도구 사용, 파일 수정, 명령 실행 등 모든 과정을 설명

### 기술 설명 방식
- **초보자 친화적**: 기술적 개념을 이해하기 쉽게 설명
- **예시 포함**: 구체적인 예시와 함께 설명
- **단계적 접근**: 복잡한 내용을 단계별로 나누어 설명
- **맥락 제공**: 현재 작업이 전체 프로젝트에서 갖는 의미 설명

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

## Updater Server

The `tools/updater` directory contains a FastAPI-based OTA update server implementation.

### Updater Architecture
- **FastAPI Server**: RESTful API following updater protocol for OTA updates
- **Python Backend**: `updater_server/` - Poll, feedback, and bundle download endpoints
- **Qt6 GUI**: `gui/` - Qt6/QML updater management interface
- **RAUC Integration**: `updater-rauc/` - RAUC client libraries and utilities
- **Bundle Management**: Automatic discovery of .raucb files for deployment

### Updater Commands
```bash
# Install and run updater server
cd tools/updater
./build.sh                    # Build backend and GUI
./server.sh                   # Start HTTP server (port 8080)

# With HTTPS/TLS support
export UPDATER_ENABLE_HTTPS=true
./server.sh                   # Start HTTPS server (port 8443)

# Generate SSL certificates
./generate_certs.sh

# Build specific components
./build.sh backend            # Build Python backend only
./build.sh gui               # Build Qt6 GUI only
./build.sh certs             # Generate certificates only

# Test the server
python test_client.py        # Test client verification
```

### Updater Protocol Endpoints
- **Poll**: `GET /{tenant}/controller/v1/{controller_id}` - Check for updates
- **Feedback**: `POST /{tenant}/controller/v1/{controller_id}/deploymentBase/{execution_id}/feedback` - Send status
- **Download**: `GET /download/{filename}` - Download bundle files
- **Admin API**: `GET/POST/DELETE /admin/deployments` - Deployment management

### Bundle Deployment with Updater Server
```bash
# Place bundles in bundle directory
cp *.raucb tools/updater/bundle/

# Start server (auto-discovers bundles)
cd tools/updater
./server.sh

# Server URLs
# HTTP:  http://localhost:8080
# HTTPS: https://localhost:8443 (with UPDATER_ENABLE_HTTPS=true)
# Admin: http://localhost:8080/admin/deployments
```

### Environment Configuration
- `UPDATER_ENABLE_HTTPS`: Enable HTTPS/TLS mode
- `UPDATER_PORT`: HTTP port (default: 8080)
- `UPDATER_HTTPS_PORT`: HTTPS port (default: 8443)
- `UPDATER_BUNDLE_DIR`: Bundle directory (default: bundle)
- `UPDATER_LOG_LEVEL`: Logging level (default: INFO)
- `UPDATER_EXTERNAL_HOST`: External host for client URLs (default: auto-detect)
- `UPDATER_LOG_FILE`: Log file path (optional)
- `UPDATER_ENABLE_AUTH`: Enable API authentication (default: false)

### Refactored Architecture

The updater server has been refactored for improved maintainability with:

- **Layered Architecture**: API, Service, and Storage layers with clear separation
- **Configuration Management**: Structured config with validation and type safety
- **Service-Oriented Design**: DeploymentService, BundleService, FeedbackService
- **Error Handling**: Custom exception hierarchy with specific error types
- **Logging Infrastructure**: Structured logging with context and standardized formats
- **Input Validation**: Comprehensive validation utilities for all inputs

### Updater Application Usage

```bash
# Run integrated GUI application (recommended)
cd tools/updater
./updater.py                     # Integrated GUI with auto server management

# Run server only (headless)
./server.sh                      # HTTP server
export UPDATER_ENABLE_HTTPS=true && ./server.sh  # HTTPS server

# Run refactored server (improved architecture)
uv run python -m updater_server.main_refactored

# Install dependencies if needed
./scripts/install_deps.sh        # Install PyQt6 and other dependencies
```

### Integrated Features
- **Auto Server Management**: GUI automatically starts/stops backend server
- **Real-time Monitoring**: Live deployment status and server health
- **File Organization**: Cleaner directory structure with organized subdirectories
- **Deployment Control**: Enable/disable deployments with GUI interface
- **Activity Logging**: Real-time server output and application events

See `tools/updater/REFACTORING_GUIDE.md` for detailed architecture documentation.

## RAUC Bundler Tool

The `tools/bundler` directory contains a CMake-based RAUC bundle creation tool for standalone bundle generation.

### Bundler Architecture
- **C Implementation**: `src/bundler.c` - Core bundler implementation 
- **CMake Build**: Standard CMake configuration with install targets
- **Scripts**: Connection and manual bundle creation utilities
- **Documentation**: Comprehensive RAUC bundle creation guide

### Bundler Commands
```bash
# Build the bundler tool
cd tools/bundler
mkdir build && cd build
cmake ..
make

# Create RAUC bundle
./bundler <rootfs.ext4> <output.raucb>

# Using manual script
./scripts/rauc-bundle-manual.sh <rootfs.ext4>

# Connect to target for deployment
./scripts/connect.sh root@192.168.1.100

# Development build with debugging
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Bundle Creation Workflow
```bash
# Standalone bundle creation (outside Yocto)
cd tools/bundler

# Build the tool
mkdir build && cd build
cmake .. && make

# Create bundle from filesystem image
./bundler /path/to/rootfs.ext4 output.raucb

# Deploy to target device
scp output.raucb root@192.168.1.100:/data/
ssh root@192.168.1.100 "rauc install /data/output.raucb"
```

### Certificate Configuration
The bundler uses fixed CA certificates from the meta-nuc layer:
- **CA Certificate**: `kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/ca.cert.pem`
- **Development Certificate**: `kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/development-1.cert.pem`
- **Private Key**: `kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/development-1.key.pem`

### Integration with Build System
The bundler tool complements the Yocto build system by providing:
- **Standalone Operation**: Create bundles without full Yocto build
- **Development Speed**: Faster iteration during bundle testing
- **CI/CD Integration**: Lightweight tool for automated pipelines
- **Manual Control**: Fine-grained control over bundle creation process