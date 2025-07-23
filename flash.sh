#!/bin/bash

set -e

# Default image path
DEFAULT_WIC="./kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-intel-corei7-64.wic"
WIC_IMAGE=""

if [ -f "$DEFAULT_WIC" ]; then
  echo "Found default image: $DEFAULT_WIC"
  WIC_IMAGE="$DEFAULT_WIC"
else
  # Search for .wic file in current directory
  WIC_COUNT=$(ls *.wic 2>/dev/null | wc -l)
  if [ "$WIC_COUNT" -eq 1 ]; then
    WIC_IMAGE=$(ls *.wic)
  elif [ "$WIC_COUNT" -gt 1 ]; then
    echo "Multiple .wic files found:"
    ls *.wic
    read -p "Enter the .wic image filename to use: " WIC_IMAGE
  else
    read -p "Enter the path to the .wic image: " WIC_IMAGE
  fi
fi

if [ ! -f "$WIC_IMAGE" ]; then
  echo "❌ Error: .wic image file '$WIC_IMAGE' not found."
  exit 1
fi

echo "\nAvailable removable devices (excluding system disk):"
lsblk -d -o NAME,SIZE,MODEL,TRAN,TYPE | grep -E 'disk'

read -p "Enter the device name to flash (e.g. sdb): /dev/" DEV_NAME
DEV_PATH="/dev/$DEV_NAME"

# Check if device exists
if [ ! -b "$DEV_PATH" ]; then
  echo "❌ Error: Device $DEV_PATH does not exist."
  exit 2
fi

# Confirm with user
echo "\n⚠️  WARNING: All data on $DEV_PATH will be erased!"
echo "Image: $WIC_IMAGE -> $DEV_PATH"
read -p "Type 'YES' to continue: " CONFIRM
if [ "$CONFIRM" != "YES" ]; then
  echo "Aborted."
  exit 3
fi

# Unmount all partitions on the device
echo "Unmounting all partitions on $DEV_PATH..."
for part in $(lsblk -ln $DEV_PATH | awk 'NR>1 {print $1}'); do
  sudo umount "/dev/$part" 2>/dev/null || true
done

# Write image with dd
set -o pipefail
echo "\nFlashing $WIC_IMAGE to $DEV_PATH..."
sudo dd if="$WIC_IMAGE" of="$DEV_PATH" bs=4M status=progress conv=fsync || { echo "❌ Error: dd failed."; exit 4; }
echo "\n✅ Flash complete!" 