#!/bin/bash

# Build script for Intel NUC Yocto project
# Defaults to auto mode, but allows manual mode

SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Check if manual mode is requested
if [ "$1" = "manual" ]; then
    echo "🔧 Manual build mode requested"
    exec "$SCRIPT_DIR/docker.sh" manual
elif [ "$1" = "bundle" ]; then
    echo "🔄 Running clean.sh ..."
    if "$SCRIPT_DIR/clean.sh"; then
        echo "✅ clean.sh completed"
    else
        echo "❌ clean.sh failed. Aborting."
        exit 1
    fi
    echo "📦 Bundle build mode"
    if ! "$SCRIPT_DIR/docker.sh" bundle; then
        echo "❌ Bundle build failed. Aborting."
        exit 1
    fi

    echo "📦 Copying bundle..."
    DEST_DIR=./tools/updater/bundle/
    if [ ! -d "$DEST_DIR" ]; then
        echo "⚠️  $DEST_DIR directory not found, skipping copy"
    else
        bundle_path=$(find kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64 -name "nuc-image-qt5-bundle-intel-corei7-64-*.raucb" | sort -r | head -n 1)
        if [ -z "$bundle_path" ]; then
            echo "❌ Bundle file not found."
            exit 1
        fi

        if ! cp "$bundle_path" "$DEST_DIR"; then
            echo "❌ Failed to copy bundle."
            exit 1
        fi
        echo "✅ Bundle copied to $DEST_DIR"
    fi
elif [ "$1" = "auto" ] || [ $# -eq 0 ]; then
    echo "🔄 Running clean.sh ..."
    if "$SCRIPT_DIR/clean.sh"; then
        echo "✅ clean.sh completed"
    else
        echo "❌ clean.sh failed. Aborting."
        exit 1
    fi
    echo "🚀 Auto build mode (default)"
    exec "$SCRIPT_DIR/docker.sh" auto
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