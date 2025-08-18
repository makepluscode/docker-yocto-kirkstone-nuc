#!/bin/bash

# Test CMake-built RAUC locally
# This validates the binary works before deployment

RAUC_BINARY="./build/rauc"

echo "=== Testing CMake-built RAUC locally ==="

# Check if binary exists
if [ ! -f "$RAUC_BINARY" ]; then
    echo "Error: RAUC binary not found at $RAUC_BINARY"
    echo "Please run 'make' first to build the binary"
    exit 1
fi

echo "Binary info:"
file "$RAUC_BINARY"
echo ""

echo "Binary size:"
ls -lh "$RAUC_BINARY"
echo ""

echo "Testing --version:"
"$RAUC_BINARY" --version 2>&1
echo ""

echo "Testing --help (first 15 lines):"
"$RAUC_BINARY" --help 2>&1 | head -15
echo ""

echo "Testing invalid command (should show error):"
"$RAUC_BINARY" --invalid-option 2>&1 | head -3
echo ""

echo "Binary dependencies:"
ldd "$RAUC_BINARY" 2>/dev/null | head -10
echo ""

echo "=== Local Test Complete ==="
echo ""
echo "To deploy to target NUC:"
echo "1. Ensure target NUC is accessible (ssh root@TARGET_IP)"
echo "2. Copy binary: scp $RAUC_BINARY root@TARGET_IP:/tmp/rauc_cmake"
echo "3. Test on target: ssh root@TARGET_IP '/tmp/rauc_cmake --version'"
echo "4. Test basic function: ssh root@TARGET_IP '/tmp/rauc_cmake status'"
