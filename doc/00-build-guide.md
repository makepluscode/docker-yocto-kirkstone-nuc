# Intel NUC Yocto Build Guide

Docker-based Yocto build environment for Intel NUC using Kirkstone (Yocto 4.0 LTS) release with **automatic RAUC (Robust Auto-Update Controller) integration**.

## Prerequisites

- Docker installed
- Minimum 50GB disk space
- Internet connection

## Build Process

### Step 1: Download Yocto Layers (First Time Only)
```bash
./download.sh
```

### Step 2: Clean Environment (Recommended)
```bash
./clean.sh
rm -rf ./kirkstone/build/conf
```

This step:
- Stops and removes existing Docker containers
- Removes Docker images to ensure fresh environment
- Removes build configuration to prevent conflicts

### Step 3: Choose Build Mode

#### Option A: Automatic Build (Recommended)
```bash
./build.sh
# or explicitly
./build.sh auto
```
**Default behavior**: Automatically builds the complete NUC image with RAUC integration and exits.

#### Option B: Manual Build Mode
```bash
./build.sh manual
```
**Manual mode**: Enters the Docker container with build environment setup for manual operations.

#### Option C: Direct Docker Control
```bash
./run-docker.sh auto     # Automatic build and exit
./run-docker.sh manual   # Manual mode
```

### Step 4: Automatic Build Environment Setup
When using automatic mode, `entrypoint.sh` runs automatically and performs:

#### 4.1 Layer Configuration
- **Automatic bblayers.conf setup** with all required layers:
  - `meta-openembedded/meta-oe`
  - `meta-openembedded/meta-python`
  - `meta-openembedded/meta-networking`
  - `meta-intel`
  - `meta-nuc` (NUC-specific configurations)
  - `meta-rauc` (RAUC auto-update system)
  - `meta-qt5`
  - `meta-apps`

#### 4.2 RAUC Integration
- **Automatic RAUC configuration** including:
  - RAUC feature enabled in `DISTRO_FEATURES`
  - RAUC package installation in images
  - GRUB configuration for A/B boot slots
  - RAUC bundle version and description setup
  - System configuration for dual-slot updates

#### 4.3 Build Configuration
- **Automatic local.conf setup** with NUC-optimized settings:
  - Intel Core i7-64 machine configuration
  - Qt5 with OpenGL/KMS/GBM support
  - SystemD with networkd/resolved
  - User account setup (root/nuc users)
  - RAUC-compatible boot configuration

Expected output (Auto mode):
```
### Shell environment set up for builds. ###

🛠 Updating bblayers.conf...
✅ local.conf replaced with custom template
🧹 Cleaning sstate for dashboard and rauc ...
🚀 Building nuc-image-qt5 ...
```

Expected output (Manual mode):
```
### Shell environment set up for builds. ###

🛠 Updating bblayers.conf...
✅ local.conf replaced with custom template
🔧 Manual mode: Build environment setup complete
   You can now run manual commands like:
   - bitbake nuc-image-qt5
   - bitbake nuc-bundle
   - bitbake -c menuconfig virtual/kernel
```

### Step 5: Build Execution

#### Automatic Mode
The entrypoint script automatically:
1. **Cleans sstate cache** for dashboard and RAUC components
2. **Builds nuc-image-qt5** with full RAUC integration
3. **Exits container** upon successful completion

#### Manual Mode
After environment setup, you can run commands manually:
```bash
# Build the complete image
bitbake nuc-image-qt5

# Build RAUC bundle
bitbake nuc-bundle

# Configure kernel
bitbake -c menuconfig virtual/kernel

# Clean specific components
bitbake -c cleansstate rauc
```

## Build Scripts

### Primary Scripts
- **`./build.sh`** - Main build script (defaults to auto mode)
- **`./run-docker.sh`** - Direct Docker container control
- **`./download.sh`** - Download Yocto layers
- **`./clean.sh`** - Clean Docker environment

### Script Usage
```bash
# Automatic build (default)
./build.sh

# Explicit automatic build
./build.sh auto

# Manual mode
./build.sh manual

# Direct Docker control
./run-docker.sh auto     # Automatic build
./run-docker.sh manual   # Manual mode

# Help
./run-docker.sh          # Shows usage
```

## Build Targets

### Primary Target
- `nuc-image-qt5` - Custom NUC image with Qt5 and RAUC support

### RAUC Components
- **RAUC System**: Robust auto-update controller
- **A/B Boot Slots**: Dual partition system for safe updates
- **GRUB Integration**: Bootloader configured for RAUC slots
- **Bundle Support**: RAUC update bundle creation capability

## Directory Structure

```
docker-yocto-kirkstone-nuc/
├── kirkstone/              # Yocto layers (host-container shared)
│   ├── poky/
│   ├── meta-openembedded/
│   ├── meta-intel/
│   ├── meta-nuc/           # NUC-specific configurations
│   │   ├── conf/
│   │   │   ├── local.conf.sample    # RAUC-enabled config template
│   │   │   └── layer.conf
│   │   ├── recipes-core/
│   │   │   ├── rauc/               # RAUC configurations
│   │   │   ├── bundles/            # RAUC bundle recipes
│   │   │   └── images/             # NUC image recipes
│   │   └── create-example-keys.sh  # RAUC key generation
│   ├── meta-rauc/          # RAUC layer
│   └── build/              # Build output directory
├── docker/                 # Docker configuration files
│   ├── Dockerfile          # Docker image definition
│   └── entrypoint.sh       # Automatic build setup script (with debugging)
├── build.sh               # Main build script (auto/manual modes)
├── run-docker.sh          # Docker container execution script
├── download.sh            # Yocto layer download script
├── clean.sh              # Docker environment cleanup script
└── flash.sh              # USB flashing script
```

## Command Sequence

### Complete Automatic Build Sequence:
```bash
# On host system
./download.sh              # Download layers (first time)
./build.sh                 # Automatic build and exit
# or
./build.sh auto            # Explicit automatic build

# Or using direct Docker control
./run-docker.sh auto       # Automatic build and exit
```

### Manual Development Sequence:
```bash
# On host system
./download.sh              # Download layers (first time)
./build.sh manual          # Enter container for manual operations

# Inside Docker container (manual mode)
source poky/oe-init-build-env build  # Setup build environment
bitbake nuc-image-qt5      # Build image manually
bitbake nuc-bundle         # Build RAUC bundle
# ... other manual commands
```

## Environment Details

### Docker Container Environment
- User: `yocto`
- Home: `/home/yocto`
- Build directory: `/home/yocto/kirkstone/build`
- Shared volume: Host `kirkstone/` ↔ Container `/home/yocto/kirkstone`

### RAUC Configuration
- **Dual-slot boot system**: A/B partitions for safe updates
- **GRUB integration**: Bootloader configured for RAUC slots
- **Bundle support**: RAUC update bundle creation
- **System integration**: RAUC service and configuration files

### Build Configuration
- Build system automatically configures when `entrypoint.sh` runs
- Uses existing `local.conf` if present (with RAUC settings)
- BitBake environment variables set automatically
- Working directory changes to `~/kirkstone/build`

## Output Files

Build artifacts location:
```
kirkstone/build/tmp/deploy/images/
```

Generated files:
- `.wic` - Flashable disk image with RAUC support
- `.rootfs.tar.bz2` - Root filesystem archive
- `.manifest` - Package list
- RAUC bundles (if configured)

## RAUC Features

### Automatic Integration
- **Pre-configured**: RAUC is automatically included in all builds
- **Dual-slot boot**: A/B partition system for safe updates
- **GRUB integration**: Bootloader configured for RAUC slots
- **Bundle support**: Ready for RAUC update bundle creation

### Manual RAUC Operations
To create RAUC keys (development):
```bash
cd kirkstone/meta-nuc
./create-example-keys.sh
```

To build RAUC bundle:
```bash
bitbake nuc-bundle
```

## Build Modes Summary

| Mode | Command | Behavior |
|------|---------|----------|
| **Auto (default)** | `./build.sh` | Automatic build and exit |
| **Auto (explicit)** | `./build.sh auto` | Explicit automatic build |
| **Manual** | `./build.sh manual` | Enter container for manual operations |
| **Direct Auto** | `./run-docker.sh auto` | Direct automatic build |
| **Direct Manual** | `./run-docker.sh manual` | Direct manual mode |

## Troubleshooting

### Manual Mode Issues
If manual mode still runs automatic build:
1. Check that `run-docker.sh` has been updated with the latest changes
2. Ensure containers are properly cleaned with `./clean.sh`
3. Verify that `entrypoint.sh` debugging logs appear when expected

### Debugging
The `entrypoint.sh` script includes debugging information:
- Shows arguments passed to the script
- Displays manual mode detection logic
- Indicates which execution path is taken

### Container Management
- **Clean environment**: `./clean.sh` removes all containers and images
- **Manual container control**: Use `./run-docker.sh` for direct Docker operations
- **Container inspection**: Use `docker ps -a` to check container status
