#!/bin/bash

# Build script for creating Ubuntu/Debian package

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

warn() {
    echo -e "${YELLOW}[WARNING]${NC} $*"
}

error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

# Check if we're on a Debian/Ubuntu system
if ! command -v dpkg-buildpackage >/dev/null 2>&1; then
    error "dpkg-buildpackage not found. This script requires a Debian/Ubuntu system."
    error "Install with: sudo apt-get install build-essential devscripts debhelper"
    exit 1
fi

# Check if required tools are installed
missing_tools=()
for tool in dh_python3 python3-setuptools; do
    if ! dpkg -l | grep -q "^ii.*${tool}"; then
        missing_tools+=("python3-${tool}")
    fi
done

if [ ${#missing_tools[@]} -gt 0 ]; then
    error "Missing required packages: ${missing_tools[*]}"
    error "Install with: sudo apt-get install ${missing_tools[*]}"
    exit 1
fi

log "Building ARCRO Debian package..."

# Clean previous builds
log "Cleaning previous builds..."
rm -rf build/ dist/ *.egg-info/ debian/arcro/

# Check if we have the required Python dependencies for building
log "Checking Python build environment..."
if ! python3 -c "import setuptools" >/dev/null 2>&1; then
    error "Python setuptools not available"
    exit 1
fi

# Make debian/rules executable
chmod +x debian/rules

# Build source package first (optional, for completeness)
log "Building source package..."
if dpkg-buildpackage -S -us -uc; then
    success "Source package built successfully"
else
    warn "Source package build failed, continuing with binary build"
fi

# Build binary package
log "Building binary package..."
if dpkg-buildpackage -b -us -uc; then
    success "Binary package built successfully"
else
    error "Binary package build failed"
    exit 1
fi

# Show built packages
log "Built packages:"
ls -la ../*.deb 2>/dev/null || warn "No .deb files found in parent directory"

log "Package contents:"
if [ -f "../rauc-updater_0.1.0-1_all.deb" ]; then
    dpkg-deb -c "../rauc-updater_0.1.0-1_all.deb"
else
    warn "Package file not found for inspection"
fi

success "Debian package build completed!"
log "To install: sudo dpkg -i ../rauc-updater_*.deb"
log "To install dependencies if missing: sudo apt-get install -f"