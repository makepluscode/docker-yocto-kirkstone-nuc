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
./run-docker.sh manual  # Enter existing container
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