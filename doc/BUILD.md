# Intel NUC Yocto Build Guide

Docker-based Yocto build environment for Intel NUC using Kirkstone (Yocto 4.0 LTS) release.

## Prerequisites

- Docker installed
- Minimum 50GB disk space
- Internet connection

## Build Process

### Step 1: Navigate to Project Directory
```bash
cd /home/makepluscode/docker-yocto-kirkstone-nuc
```

### Step 2: Download Yocto Layers (First Time Only)
```bash
./download.sh
```

Downloads required layers to `kirkstone` directory:
- poky (Yocto core)
- meta-openembedded
- meta-intel
- meta-rauc

### Step 3: Clean Environment (Recommended)
```bash
./clean.sh
rm -rf ./kirkstone/build/conf
```

This step:
- Stops and removes existing Docker containers
- Removes Docker images to ensure fresh environment
- Removes build configuration to prevent conflicts

### Step 4: Run Docker Build Environment
```bash
./run-docker.sh
```

Script behavior:
- Builds Docker image if not exists
- Attaches to running container if exists
- Restarts stopped container if exists
- Creates new container if needed

### Step 5: Build Environment Setup (Automatic)
When entering Docker container, `entrypoint.sh` runs automatically:

Expected output:
```
### Shell environment set up for builds. ###

You can now run 'bitbake <target>'

Common targets are:
    core-image-minimal
    core-image-full-cmdline
    core-image-sato
    core-image-weston
    meta-toolchain
    meta-ide-support
```

Prompt changes to: `yocto@nuc:~/kirkstone/build$`

### Step 6: Build Qt5 Image for NUC
```bash
bitbake nuc-image-qt5
```

Build starts with:
```
Loading cache: 100% |######################################################
```

## Build Targets

### Primary Target
- `nuc-image-qt5` - Custom NUC image with Qt5 support

### Available Targets
- `core-image-minimal` - Minimal image
- `core-image-full-cmdline` - Full command line environment
- `core-image-sato` - Sato desktop environment
- `core-image-weston` - Weston compositor
- `meta-toolchain` - Cross-compilation toolchain
- `meta-ide-support` - IDE support packages

## Directory Structure

```
docker-yocto-kirkstone-nuc/
├── kirkstone/              # Yocto layers (host-container shared)
│   ├── poky/
│   ├── meta-openembedded/
│   ├── meta-intel/
│   ├── meta-rauc/
│   └── build/              # Build output directory
├── docker/                 # Docker configuration files
├── run-docker.sh          # Docker container execution script
├── download.sh            # Yocto layer download script
├── clean.sh              # Docker environment cleanup script
└── flash.sh              # USB flashing script
```

## Command Sequence

Complete build sequence:
```bash
# On host system
./download.sh              # Download layers (first time)
./run-docker.sh            # Enter Docker container

# Inside Docker container (automatic)
./entrypoint.sh            # Runs automatically, sets up environment

# Build command (manual)
bitbake nuc-image-qt5      # Build Qt5 image
```

## Environment Details

### Docker Container Environment
- User: `yocto`
- Home: `/home/yocto`
- Build directory: `/home/yocto/kirkstone/build`
- Shared volume: Host `kirkstone/` ↔ Container `/home/yocto/kirkstone`

### Build Configuration
- Build system automatically configures when `entrypoint.sh` runs
- Uses existing `local.conf` if present
- BitBake environment variables set automatically
- Working directory changes to `~/kirkstone/build`

## Output Files

Build artifacts location:
```
kirkstone/build/tmp/deploy/images/
```

Generated files:
- `.wic` - Flashable disk image
- `.rootfs.tar.bz2` - Root filesystem archive
- `.manifest` - Package list

## Troubleshooting

### Manual Environment Setup
If build environment not set up:
```bash
./entrypoint.sh
```

### Container Re-entry
```bash
./run-docker.sh
```

### Clean Environment Reset
```bash
./clean.sh
./run-docker.sh
```

## Build Process Flow

1. Host system: `./run-docker.sh`
2. Docker container starts
3. `entrypoint.sh` runs automatically
4. Environment configured
5. Prompt: `yocto@nuc:~/kirkstone/build$`
6. Execute: `bitbake nuc-image-qt5`
7. Build completes
8. Output in `kirkstone/build/tmp/deploy/images/` 