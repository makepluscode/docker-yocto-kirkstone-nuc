#!/bin/bash

# Installation script for RAUC Updater

set -euo pipefail

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

# Detect OS
if [ -f /etc/os-release ]; then
    source /etc/os-release
    OS=$ID
    VERSION=$VERSION_ID
else
    error "Cannot detect operating system"
    exit 1
fi

log "Detected OS: $OS $VERSION"

# Check if we're on Ubuntu/Debian
if [[ "$OS" != "ubuntu" && "$OS" != "debian" ]]; then
    error "This installer currently supports Ubuntu and Debian only"
    exit 1
fi

# Install method selection
echo
echo "Choose installation method:"
echo "1) Install from .deb package (recommended)"
echo "2) Install with pip/uv (development)"
echo "3) Build and install from source"
echo
read -p "Enter your choice (1-3): " choice

case $choice in
    1)
        # Install from .deb package
        log "Installing from .deb package..."
        
        # Check if package exists
        if [ ! -f "../rauc-updater_0.1.0-1_all.deb" ]; then
            log "Package not found, building it first..."
            ./build-deb.sh
        fi
        
        # Install dependencies
        log "Installing dependencies..."
        sudo apt-get update
        sudo apt-get install -y python3-paramiko python3-click python3-rich python3-pydantic
        
        # Ask about GUI dependencies
        read -p "Install GUI dependencies (PyQt6)? [y/N]: " install_gui
        if [[ "$install_gui" =~ ^[Yy]$ ]]; then
            sudo apt-get install -y python3-pyqt6 python3-pyqt6.qtquick
        fi
        
        # Install package
        log "Installing RAUC Updater package..."
        sudo dpkg -i ../rauc-updater_*.deb || {
            warn "Package installation failed, trying to fix dependencies..."
            sudo apt-get install -f -y
            sudo dpkg -i ../rauc-updater_*.deb
        }
        ;;
        
    2)
        # Install with pip/uv
        log "Installing with pip/uv..."
        
        # Check if uv is available
        if command -v uv >/dev/null 2>&1; then
            log "Using uv for installation..."
            uv tool install --editable .
            if [[ "${install_gui:-n}" =~ ^[Yy]$ ]]; then
                uv tool install --editable ".[gui]"
            fi
        else
            log "Using pip for installation..."
            pip3 install --user -e .
            read -p "Install GUI dependencies (PyQt6)? [y/N]: " install_gui
            if [[ "$install_gui" =~ ^[Yy]$ ]]; then
                pip3 install --user ".[gui]"
            fi
        fi
        ;;
        
    3)
        # Build and install from source
        log "Building and installing from source..."
        
        # Install build dependencies
        sudo apt-get update
        sudo apt-get install -y python3-setuptools python3-pip python3-dev build-essential
        
        # Build and install
        python3 setup.py build
        sudo python3 setup.py install
        
        # Install runtime dependencies
        sudo apt-get install -y python3-paramiko python3-click python3-rich python3-pydantic
        
        read -p "Install GUI dependencies (PyQt6)? [y/N]: " install_gui
        if [[ "$install_gui" =~ ^[Yy]$ ]]; then
            sudo apt-get install -y python3-pyqt6 python3-pyqt6.qtquick
        fi
        ;;
        
    *)
        error "Invalid choice"
        exit 1
        ;;
esac

# Test installation
log "Testing installation..."
if command -v rauc-updater >/dev/null 2>&1; then
    success "CLI tool installed successfully"
    rauc-updater --version
else
    error "CLI tool installation failed"
fi

if command -v rauc-updater-gui >/dev/null 2>&1; then
    success "GUI tool installed successfully"
    log "You can run 'rauc-updater-gui' to start the graphical interface"
else
    warn "GUI tool not available (may need PyQt6 dependencies)"
fi

success "Installation completed!"
log ""
log "Usage:"
log "  CLI: rauc-updater --help"
log "  GUI: rauc-updater-gui"
log ""
log "For more information, see the documentation in docs/"