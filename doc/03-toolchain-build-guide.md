# Yocto Toolchain Build Guide

Docker-based Yocto toolchain build environment for Intel NUC using Kirkstone (Yocto 4.0 LTS) release with **Qt5 cross-compilation support**.

## Prerequisites

- Docker installed
- Minimum 50GB disk space
- Internet connection
- Yocto build environment already set up (see [00-build-guide.md](./00-build-guide.md))

## Build Process

### Step 1: Enter Docker Container (Manual Mode)

```bash
# From project root directory
./entrypoint.sh

# Yocto environment is automatically configured inside햣 the container
# You can now build the toolchain
```

This step:
- Enters Docker container with build environment setup
- Configures Yocto layers and local.conf automatically
- Provides manual control for toolchain building

**Expected output:**
```
### Shell environment set up for builds. ###

🛠 Updating bblayers.conf...
✅ local.conf replaced with custom template
🔧 Manual mode: Build environment setup complete
   You can now run manual commands like:
   - bitbake meta-toolchain-qt5
   - bitbake core-image-minimal -c populate_sdk
```

### Step 2: Build Toolchain

Execute the following commands inside the container:

```bash
# Qt5 toolchain build (recommended)
bitbake meta-toolchain-qt5

# Or basic toolchain build
bitbake core-image-minimal -c populate_sdk
```

**Build time:** Approximately 30-60 minutes (depending on system performance)

**Expected output:**
```
NOTE: Executing Tasks
NOTE: Tasks Summary: Attempted 1234 tasks of which 1230 didn't need to be rerun and 4 succeeded.
```

### Step 3: Verify Build Completion

```bash
# Check SDK files
ls -la tmp-glibc/deploy/sdk/

# Expected output:
# poky-glibc-x86_64-core-image-minimal-corei7-64-toolchain-4.0.4.sh
# poky-glibc-x86_64-meta-toolchain-qt5-corei7-64-toolchain-4.0.4.sh
```

### Step 4: Exit Docker Container

```bash
# Exit container
exit
```

### Step 5: Install SDK

Install SDK on the host system:

```bash
# From project root directory
cd kirkstone

# Find SDK installation files
find . -name "*toolchain*.sh" -type f

# Install Qt5 toolchain (recommended)
./tmp-glibc/deploy/sdk/poky-glibc-x86_64-meta-toolchain-qt5-corei7-64-toolchain-4.0.4.sh

# Or install basic toolchain
./tmp-glibc/deploy/sdk/poky-glibc-x86_64-core-image-minimal-corei7-64-toolchain-4.0.4.sh
```

This step:
- Installs the cross-compilation toolchain on host system
- Sets up Qt5 development environment
- Configures system paths for cross-compilation

**Installation options:**
- Default installation path: `/opt/poky/4.0.4/`
- Custom path selection available

**Expected output:**
```
Extracting SDK...
Installing SDK...
SDK has been successfully installed.
```

### Step 6: Setup Toolchain Environment

Configure environment after SDK installation:

```bash
# Setup Qt5 toolchain environment
source /opt/poky/4.0.4/environment-setup-corei7-64-poky-linux

# Or if installed to custom path
source /path/to/installed/sdk/environment-setup-corei7-64-poky-linux
```

This step:
- Configures cross-compilation environment variables
- Sets up Qt5 development paths
- Enables cross-compilation tools

**Verify environment variables:**
```bash
# Check compiler
echo $CC
# Output: x86_64-poky-linux-gcc

# Check Qt5 path
echo $QTDIR
# Output: /opt/poky/4.0.4/sysroots/corei7-64-poky-linux/usr
```

**Expected output:**
```
### Shell environment set up for builds. ###
You can now run commands like:
  x86_64-poky-linux-gcc --version
  x86_64-poky-linux-g++ --version
```

## Cross-Compilation Usage

### Dashboard Project Build

Once toolchain is configured, you can cross-compile the dashboard project:

```bash
# Navigate to dashboard project directory
cd kirkstone/local/dashboard

# Create build directory
mkdir -p build
cd build

# CMake configuration (using toolchain)
cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/poky/4.0.4/sysroots/x86_64-pokysdk-linux/usr/share/cmake/OEToolchainConfig.cmake

# Build
make -j$(nproc)
```

**Expected output:**
```
-- The CXX compiler identification is GNU 11.5.0
-- Detecting CXX compiler ABI info
-- Check for working CXX compiler: /opt/poky/4.0.4/sysroots/x86_64-pokysdk-linux/usr/bin/x86_64-poky-linux/x86_64-poky-linux-g++ - works
-- Found Qt5: /opt/poky/4.0.4/sysroots/corei7-64-poky-linux/usr/lib/cmake/Qt5
-- Configuring done
-- Generating done
-- Build files have been written to: /path/to/dashboard/build
```

## Build Scripts

### Primary Scripts
- **`./entrypoint.sh`** - Enter Docker container with build environment setup
- **`./run-docker.sh`** - Direct Docker container control

### Script Usage
```bash
# Manual mode (recommended for toolchain build)
./entrypoint.sh

# Direct Docker control
./run-docker.sh manual   # Manual mode

# Help
./run-docker.sh          # Shows usage
```

## Build Targets

### Primary Targets
- `meta-toolchain-qt5` - Qt5 cross-compilation toolchain (recommended)
- `core-image-minimal -c populate_sdk` - Basic cross-compilation toolchain

### Toolchain Features
- **Qt5 Development**: Full Qt5/QML development environment
- **Cross-Compilation**: Target-specific compiler and libraries
- **OpenGL Support**: Hardware acceleration support
- **System Integration**: Compatible with target system libraries

## Directory Structure

```
kirkstone/
├── tmp-glibc/
│   └── deploy/
│       └── sdk/                    # SDK installation files
│           ├── poky-glibc-x86_64-meta-toolchain-qt5-corei7-64-toolchain-4.0.4.sh
│           └── poky-glibc-x86_64-core-image-minimal-corei7-64-toolchain-4.0.4.sh
├── local/
│   └── dashboard/                  # Dashboard project (cross-compiled)
│       ├── src/                    # C++ source files
│       ├── qml/                    # QML UI files
│       ├── resources/              # Qt resources
│       └── build/                  # Cross-compiled build output
└── build/                          # Yocto build directory
```

## Command Sequence

### Complete Toolchain Build Sequence:
```bash
# On host system
./entrypoint.sh                     # Enter container

# Inside Docker container
bitbake meta-toolchain-qt5         # Build Qt5 toolchain
exit                               # Exit container

# On host system
cd kirkstone
./tmp-glibc/deploy/sdk/poky-glibc-x86_64-meta-toolchain-qt5-corei7-64-toolchain-4.0.4.sh
source /opt/poky/4.0.4/environment-setup-corei7-64-poky-linux
```

### Manual Development Sequence:
```bash
# On host system
./entrypoint.sh                     # Enter container for manual operations

# Inside Docker container (manual mode)
source poky/oe-init-build-env build  # Setup build environment
bitbake meta-toolchain-qt5          # Build toolchain manually
exit                                # Exit container

# On host system
cd kirkstone
./tmp-glibc/deploy/sdk/poky-glibc-x86_64-meta-toolchain-qt5-corei7-64-toolchain-4.0.4.sh
source /opt/poky/4.0.4/environment-setup-corei7-64-poky-linux
```

### Cross-Compilation Sequence:
```bash
# Setup toolchain environment
source /opt/poky/4.0.4/environment-setup-corei7-64-poky-linux

# Build dashboard project
cd kirkstone/local/dashboard
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/poky/4.0.4/sysroots/x86_64-pokysdk-linux/usr/share/cmake/OEToolchainConfig.cmake
make -j$(nproc)
```

## Environment Details

### Docker Container Environment
- User: `yocto`
- Home: `/home/yocto`
- Build directory: `/home/yocto/kirkstone/build`
- Shared volume: Host `kirkstone/` ↔ Container `/home/yocto/kirkstone`

### Toolchain Configuration
- **Cross-compilation setup**: Target-specific compiler and libraries
- **Qt5 integration**: Full Qt5 development environment
- **System compatibility**: Compatible with target system libraries
- **OpenGL support**: Hardware acceleration capabilities

### Build Configuration
- Build system automatically configures when `entrypoint.sh` runs
- Uses existing `local.conf` if present (with toolchain settings)
- BitBake environment variables set automatically
- Working directory changes to `~/kirkstone/build`

### Toolchain Environment Variables
- **CC**: `x86_64-poky-linux-gcc` - C compiler
- **CXX**: `x86_64-poky-linux-g++` - C++ compiler
- **QTDIR**: `/opt/poky/4.0.4/sysroots/corei7-64-poky-linux/usr` - Qt5 installation
- **PKG_CONFIG_PATH**: Qt5 pkg-config path
- **PATH**: Updated with toolchain binaries

### SDK Installation Paths
- **Default**: `/opt/poky/4.0.4/`
- **Sysroot**: `/opt/poky/4.0.4/sysroots/corei7-64-poky-linux/`
- **Toolchain**: `/opt/poky/4.0.4/sysroots/x86_64-pokysdk-linux/`

## Output Files

Toolchain build artifacts location:
```
kirkstone/tmp-glibc/deploy/sdk/
```

Generated files:
- `*.sh` - SDK installation scripts
- `*.tar.gz` - SDK archives (if configured)

Cross-compiled application location:
```
kirkstone/local/dashboard/build/dashboard
```

### Build Failures

1. **Dependency issues:**
   ```bash
   # Clean sstate cache
   bitbake -c cleansstate meta-toolchain-qt5
   bitbake meta-toolchain-qt5
   ```

2. **Disk space issues:**
   ```bash
   # Check build directory size
   du -sh tmp-glibc/
   ```

3. **Memory issues:**
   ```bash
   # Reduce build jobs
   bitbake meta-toolchain-qt5 -j1
   ```

### SDK Installation Failures

1. **Permission issues:**
   ```bash
   # Check execution permissions
   chmod +x tmp-glibc/deploy/sdk/*.sh
   ```

2. **Path issues:**
   ```bash
   # Install with absolute path
   sudo ./tmp-glibc/deploy/sdk/*.sh -y -d /opt/poky/4.0.4
   ```

### Cross-Compilation Issues

1. **CMake toolchain not found:**
   ```bash
   # Verify toolchain file exists
   ls -la /opt/poky/4.0.4/sysroots/x86_64-pokysdk-linux/usr/share/cmake/OEToolchainConfig.cmake
   ```

2. **Qt5 not found:**
   ```bash
   # Check Qt5 installation
   ls -la /opt/poky/4.0.4/sysroots/corei7-64-poky-linux/usr/lib/libQt5*
   ```

## Build Modes Summary

| Mode | Command | Behavior |
|------|---------|----------|
| **Manual** | `./entrypoint.sh` | Enter container for manual toolchain build |
| **Direct** | `./run-docker.sh manual` | Direct manual mode |

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

## Useful Commands

```bash
# Check toolchain version
x86_64-poky-linux-gcc --version

# Check Qt5 libraries
ls /opt/poky/4.0.4/sysroots/corei7-64-poky-linux/usr/lib/libQt5*

# Check all environment variables
env | grep -E "(CC|CXX|QTDIR|PKG_CONFIG|PATH)"

# Verify cross-compilation
file dashboard  # Should show: ELF 64-bit LSB executable, x86-64
```

## Next Steps

After toolchain setup is complete, refer to the following guides:

- [04. Dashboard Development Guide](./04-dashboard-development-guide.md)
- [05. Debugging Guide](./05-debugging-guide.md)

## References

- [Yocto Project SDK Manual](https://docs.yoctoproject.org/dev-manual/sdk-manual.html)
- [Qt5 Cross-Compilation Guide](https://doc.qt.io/qt-5/linux.html)
- [CMake Toolchain Files](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html) 