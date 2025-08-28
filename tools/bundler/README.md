# RAUC Bundler Tool

A CMake-based RAUC bundle creation tool for Intel NUC with real Yocto-built ext4 images and fixed certificates for production deployment.

## Project Structure

```
tools/bundler/
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # This file
├── create-nuc-bundle.sh        # 🆕 Complete NUC bundle creation script
├── create-ext4-and-bundle.sh   # Legacy script (deprecated)
├── src/                        # Source code
│   └── bundler.c              # Main bundler implementation
├── create-bundle.sh            # 🆕 Manual bundle creation with fixed certs  
├── deploy-bundle.sh            # 🆕 Bundle deployment to NUC target
├── scripts/                    # Utility scripts
│   └── connect.sh             # SSH connection script
├── docs/                       # Documentation
│   ├── README.md              # Original README
│   └── rauc-bundle-guide.md    # RAUC bundle guide
├── test/                       # Test files and data
└── build/                      # Build output directory
    ├── bundler                # Compiled bundler executable
    ├── bundle-temp/           # Temporary bundle creation directory
    └── output/                # Generated RAUC bundles
```

## Quick Start - NUC Bundle Creation

### 🚀 One-Command Bundle Creation (Recommended)

```bash
# From project root (/home/makepluscode/docker-yocto-kirkstone-nuc)
./tools/bundler/create-nuc-bundle.sh
```

This script automatically:
1. ✅ Uses real Yocto-built ext4 image (`nuc-image-qt5-intel-corei7-64.ext4`)
2. ✅ Applies NUC fixed certificates for compatibility
3. ✅ Creates production-ready RAUC bundle with timestamp
4. ✅ Verifies bundle integrity with NUC CA
5. ✅ Provides deployment commands for target

## Manual Workflow

### Prerequisites

- CMake 3.10 or higher
- GCC compiler
- RAUC tools
- Yocto-built ext4 image (from `./build.sh`)

### Step 1: Build Yocto Image (If Not Exists)

```bash
# From project root
./build.sh

# Expected output location:
# kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-intel-corei7-64.ext4
```

### Step 2: Build Bundler Tool

```bash
cd tools/bundler
mkdir build && cd build
cmake ..
make
```

### Step 3: Create Bundle with Real ext4 and Fixed Certificates

```bash
# Method A: Using create-nuc-bundle.sh (Recommended)
cd /home/makepluscode/docker-yocto-kirkstone-nuc
./tools/bundler/create-nuc-bundle.sh

# Method B: Using compiled bundler directly
cd tools/bundler/build
./bundler \
    -c /home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/development-1.cert.pem \
    -k /home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/development-1.key.pem \
    -v bundle-directory output-bundle.raucb

# Method C: Using manual script (auto-detects Yocto image)
./create-bundle.sh

# Method D: Using manual script with specific image
./create-bundle.sh \
    kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-intel-corei7-64.ext4
```

### Step 4: Deploy to NUC Target

```bash
# Method A: Using deploy script (Recommended)
./deploy-bundle.sh -i -s                     # Deploy, install, and check status
./deploy-bundle.sh -i 192.168.1.150          # Deploy to specific IP and install
./deploy-bundle.sh bundle.raucb               # Deploy specific bundle

# Method B: Manual deployment
scp nuc-image-qt5-bundle-intel-corei7-64-*.raucb root@192.168.1.100:/tmp/
ssh root@192.168.1.100 'rauc install /tmp/nuc-image-qt5-bundle-intel-corei7-64-*.raucb'
ssh root@192.168.1.100 'rauc status'
```

## Bundle Creation Options

### Automatic ext4 Image Detection

The `create-bundle.sh` script automatically searches for Yocto-built ext4 images in the following order:

1. `kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-intel-corei7-64.ext4`
2. `kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-intel-corei7-64.rootfs.ext4`
3. `kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-intel-corei7-64.ext4`
4. `kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-intel-corei7-64.rootfs.ext4`
5. Any `*.ext4` file in the deploy directory

### Usage Examples

```bash
# Auto-detect Yocto-built ext4 image (recommended)
./create-bundle.sh

# With specific ext4 image
./create-bundle.sh /path/to/custom-image.ext4

# Complete workflow with auto-detection
./create-nuc-bundle.sh
```

### When ext4 Image Not Found

If no Yocto-built ext4 image is found, the script will display:

```
ERROR: No Yocto-built ext4 image found!

Expected locations:
  - kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-intel-corei7-64.ext4
  - kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-intel-corei7-64.rootfs.ext4

To build the ext4 image:
  cd /home/makepluscode/docker-yocto-kirkstone-nuc
  ./build.sh

Or specify a custom ext4 file:
  ./create-bundle.sh /path/to/custom-image.ext4
```

### Deployment Options

```bash
# Deploy script options
./deploy-bundle.sh [OPTIONS] [TARGET] [BUNDLE_FILE]

Options:
  -u, --user USER     SSH username (default: root)
  -d, --dir DIR       Remote directory (default: /tmp)
  -t, --timeout SEC   Connection timeout (default: 10)
  -i, --install       Install bundle after copying
  -s, --status        Check RAUC status after installation
  -v, --verbose       Enable verbose output
  -h, --help          Show help message

Examples:
  ./deploy-bundle.sh                        # Deploy to default target
  ./deploy-bundle.sh -i -s                  # Deploy, install, and check status
  ./deploy-bundle.sh 192.168.1.150          # Deploy to specific IP
  ./deploy-bundle.sh bundle.raucb            # Deploy specific bundle
```

### Bundler Command Line Options

```bash
./build/bundler [OPTIONS] <bundle-directory> <output.raucb>

Options:
  -c, --cert PATH    Path to certificate file (required for NUC)
  -k, --key PATH     Path to private key file (required for NUC)
  -v, --verbose      Enable verbose output
  -f, --force        Overwrite existing output file
  -h, --help         Show help message
```

## NUC Fixed Certificates

The bundler automatically uses NUC-compatible fixed certificates:

```bash
# Certificate locations (automatically detected)
kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/
├── ca.cert.pem              # CA certificate for verification
├── development-1.cert.pem   # Development certificate for signing
└── development-1.key.pem    # Private key for signing
```

These certificates are pre-installed on NUC targets for seamless deployment.

## Bundle Verification

```bash
# Verify bundle with NUC CA
rauc info --keyring kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/ca.cert.pem bundle.raucb

# Basic bundle info (no verification)
rauc info bundle.raucb
```

## Troubleshooting

### Common Issues

#### 1. No ext4 image found
```
ERROR: No Yocto-built ext4 image found
```
**Solution**: Run `./build.sh` from project root first

#### 2. Certificate not found
```
ERROR: NUC development certificate not found
```
**Solution**: Ensure you're in the correct project directory with meta-nuc layer

#### 3. Bundle verification failed
```
WARNING: Bundle verification failed (but bundle was created)
```
**Solution**: Bundle is still usable; verification requires exact keyring match

### Debug Mode

```bash
# Build in debug mode
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with verbose output
./bundler -v bundle-directory output.raucb
```

## Production Deployment Checklist

- ✅ Use real Yocto-built ext4 image (not test images)
- ✅ Use NUC fixed certificates (not test certificates)
- ✅ Verify bundle with NUC CA before deployment
- ✅ Test deployment on non-production NUC first
- ✅ Use timestamped bundle names for version tracking
- ✅ Keep deployment logs for troubleshooting

## Files Generated

### Successful Bundle Creation Output
```
nuc-image-qt5-bundle-intel-corei7-64-20250827203000.raucb
```

### Build Artifacts
```
tools/bundler/build/
├── bundler                    # Compiled executable
├── bundle-temp/              # Temporary files during creation
│   ├── manifest.raucm       # RAUC manifest
│   └── rootfs.ext4         # Copied ext4 image
└── output/                  # Final bundle output
```

## Integration with CI/CD

```bash
# Automated bundle creation
./tools/bundler/create-nuc-bundle.sh

# Exit code 0 = success, non-zero = failure
echo $?

# Bundle path: nuc-image-qt5-bundle-intel-corei7-64-$(date +%Y%m%d%H%M%S).raucb
```

This tool ensures NUC compatibility by using the exact same certificates and image format expected by the target hardware.