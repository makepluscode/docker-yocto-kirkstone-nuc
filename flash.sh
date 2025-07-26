#!/bin/bash

set -e

# Prompt for image type
echo "Available image types:"
echo "  1. nuc-image (default)"
echo "  2. nuc-image-qt5"
read -p "Select image type (1 or 2, default: 1): " IMAGE_TYPE

case "$IMAGE_TYPE" in
  "2"|"nuc-image-qt5")
    IMAGE_NAME="nuc-image-qt5"
    ;;
  ""|"1"|"nuc-image")
    IMAGE_NAME="nuc-image"
    ;;
  *)
    echo "Invalid selection. Using default: nuc-image"
    IMAGE_NAME="nuc-image"
    ;;
esac

echo "Selected image: $IMAGE_NAME"

# Default image path
DEFAULT_WIC="./kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/${IMAGE_NAME}-intel-corei7-64.wic"
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

echo "\nAvailable USB devices:"

# Get USB devices and store in arrays
USB_DEVICES=()
USB_INFO=()
while IFS= read -r line; do
  if [[ "$line" =~ ^([a-z]+[0-9]*)[[:space:]]+([^[:space:]]+)[[:space:]]+(.+)[[:space:]]+usb[[:space:]]+disk$ ]]; then
    device_name="${BASH_REMATCH[1]}"
    device_size="${BASH_REMATCH[2]}"
    device_model="${BASH_REMATCH[3]}"
    USB_DEVICES+=("$device_name")
    USB_INFO+=("$device_size $device_model")
  fi
done < <(lsblk -d -o NAME,SIZE,MODEL,TRAN,TYPE | grep -E 'usb.*disk')

# Check if any USB devices found
if [ ${#USB_DEVICES[@]} -eq 0 ]; then
  echo "❌ Error: No USB devices found."
  exit 2
fi

# Display USB devices with numbers
for i in "${!USB_DEVICES[@]}"; do
  echo "  $((i+1)). /dev/${USB_DEVICES[$i]} - ${USB_INFO[$i]}"
done

# Get user selection
while true; do
  read -p "Select USB device (1-${#USB_DEVICES[@]}): " SELECTION
  if [[ "$SELECTION" =~ ^[0-9]+$ ]] && [ "$SELECTION" -ge 1 ] && [ "$SELECTION" -le "${#USB_DEVICES[@]}" ]; then
    DEV_NAME="${USB_DEVICES[$((SELECTION-1))]}"
    DEV_PATH="/dev/$DEV_NAME"
    break
  else
    echo "❌ Invalid selection. Please enter a number between 1 and ${#USB_DEVICES[@]}."
  fi
done

echo "Selected device: $DEV_PATH"

# Check if device exists
if [ ! -b "$DEV_PATH" ]; then
  echo "❌ Error: Device $DEV_PATH does not exist."
  exit 2
fi

# Confirm with user
echo "\n⚠️  WARNING: All data on $DEV_PATH will be erased!"
echo "Image: $WIC_IMAGE -> $DEV_PATH"
read -p "Type 'Y' or 'y' to continue: " CONFIRM
if [ "$CONFIRM" != "Y" ] && [ "$CONFIRM" != "y" ]; then
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