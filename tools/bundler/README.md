# RAUC Bundler Tool

A CMake-based RAUC bundle creation tool for Yocto-based embedded systems.

## Project Structure

```
tools/bundler/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── src/                    # Source code
│   └── bundler.c          # Main bundler implementation
├── scripts/                # Utility scripts
│   ├── connect.sh         # SSH connection script
│   └── rauc-bundle-manual.sh # Manual bundle creation
├── docs/                   # Documentation
│   ├── README.md          # Original README
│   └── rauc-bundle-guide.md # RAUC bundle guide
└── test/                   # Test files and data
```

## Building

### Prerequisites

- CMake 3.10 or higher
- GCC compiler
- RAUC tools

### Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Install (optional)
sudo make install
```

### Development Build

```bash
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## Usage

### Basic Bundle Creation

```bash
# Using the compiled bundler
./build/bundler <rootfs.ext4> <output.raucb>

# Using the manual script
./scripts/rauc-bundle-manual.sh <rootfs.ext4>
```

### Connecting to Target

```bash
./scripts/connect.sh root@192.168.1.100
```

## Testing

```bash
# Run tests
cd build
make test

# Manual testing
./bundler --help
```

## Configuration

The bundler uses fixed CA certificates from the meta-nuc layer:
- CA Certificate: `kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/ca.cert.pem`
- Development Certificate: `kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/development-1.cert.pem`
- Private Key: `kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/development-1.key.pem`

## License

This project is part of the Yocto-based NUC system. 