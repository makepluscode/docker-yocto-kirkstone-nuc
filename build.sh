#!/bin/bash

# Build script for Intel NUC Yocto project
# Defaults to auto mode, but allows manual mode

SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Check if manual mode is requested
if [ "$1" = "manual" ]; then
    echo "🔧 Manual build mode requested"
    exec "$SCRIPT_DIR/run-docker.sh" manual
elif [ "$1" = "bundle" ]; then
    echo "🔄 Running clean.sh ..."
    if "$SCRIPT_DIR/clean.sh"; then
        echo "✅ clean.sh completed"
    else
        echo "❌ clean.sh failed. Aborting."
        exit 1
    fi
    echo "📦 Bundle build mode"
    exec "$SCRIPT_DIR/run-docker.sh" bundle
elif [ "$1" = "auto" ] || [ $# -eq 0 ]; then
    echo "🔄 Running clean.sh ..."
    if "$SCRIPT_DIR/clean.sh"; then
        echo "✅ clean.sh completed"
    else
        echo "❌ clean.sh failed. Aborting."
        exit 1
    fi
    echo "🚀 Auto build mode (default)"
    exec "$SCRIPT_DIR/run-docker.sh" auto
else
    echo "Usage: $0 [auto|manual|bundle]"
    echo "  (no args) - Auto build mode: clean, configure, and build nuc-image-qt5"
    echo "  auto      - Auto build mode: clean, configure, and build nuc-image-qt5"
    echo "  manual    - Manual mode: enter container with build environment setup"
    echo "  bundle    - Bundle build mode: clean, configure, and build nuc-image-qt5-bundle"
    echo ""
    echo "Examples:"
    echo "  $0         # Run automatic build and exit"
    echo "  $0 auto    # Run automatic build and exit"
    echo "  $0 manual  # Enter container for manual operations"
    echo "  $0 bundle  # Build RAUC bundle for nuc-image-qt5"
    exit 1
fi 