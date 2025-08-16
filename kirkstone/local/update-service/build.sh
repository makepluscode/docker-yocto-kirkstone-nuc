#!/usr/bin/env bash
# Build update-service with cross-compilation toolchain
# Usage: ./build.sh

set -e

echo "🔨 Building update-service..."

# Check if toolchain is sourced
if [[ -z "$OECORE_SDK_VERSION" ]]; then
    echo "❌ Yocto SDK toolchain not sourced."
    echo "Please run:"
    echo "  unset LD_LIBRARY_PATH"
    echo "  source /usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux"
    exit 1
fi

echo "✅ Using Yocto SDK: $OECORE_SDK_VERSION"
echo "✅ Target: $OECORE_TARGET_ARCH"

# Create build directory
mkdir -p build
cd build

echo "🔧 Configuring with CMake..."
cmake .. -DCMAKE_TOOLCHAIN_FILE="/usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake"

echo "🔨 Building..."
make -j$(nproc)

echo "✅ Build completed successfully"
echo "📦 Binaries:"
ls -la update-service test-client 2>/dev/null || true

echo ""
echo "Next steps:"
echo "  ./deploy.sh          - Deploy to target device"
echo "  ./test-client        - Test D-Bus broker (on target)"