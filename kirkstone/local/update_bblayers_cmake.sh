#!/bin/bash

# Script to update bblayers.conf to use meta-rauc-cmake instead of meta-rauc
# This should be run by entrypoint.sh or similar

BBLAYERS_CONF="../build/conf/bblayers.conf"

echo "=== Updating bblayers.conf to use meta-rauc-cmake ==="

if [ -f "$BBLAYERS_CONF" ]; then
    # Backup original
    cp "$BBLAYERS_CONF" "$BBLAYERS_CONF.backup"
    
    # Replace meta-rauc with meta-rauc-cmake
    sed -i 's|/kirkstone/meta-rauc |/kirkstone/meta-rauc-cmake |g' "$BBLAYERS_CONF"
    
    echo "Updated bblayers.conf:"
    echo "- Replaced meta-rauc with meta-rauc-cmake"
    echo "- Backup saved as bblayers.conf.backup"
    
    # Show the change
    echo ""
    echo "Current BBLAYERS:"
    grep -A10 "BBLAYERS" "$BBLAYERS_CONF" | grep meta-rauc
else
    echo "bblayers.conf not found at $BBLAYERS_CONF"
    echo "This script should be run after the build environment is set up"
fi