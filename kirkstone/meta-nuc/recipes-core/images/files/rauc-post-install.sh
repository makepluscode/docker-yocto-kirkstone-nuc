#!/bin/bash
# RAUC Post-Install Script
# This script configures RAUC A/B booting after system installation

set -e

echo "Configuring RAUC A/B booting..."

# Wait for system to be ready
sleep 5

# Create RAUC grub.cfg
cat > /grubenv/EFI/BOOT/grub.cfg << 'EOF'
default=0
timeout=3

set ORDER="A B"
set A_OK=0
set B_OK=0
set A_TRY=0
set B_TRY=0
load_env --file=(hd0,1)/grubenv

# fallback defaults if variables not set (first boot)
if [ -z "$ORDER" ]; then
    set ORDER="A B"
fi
if [ -z "$A_OK" ]; then
    set A_OK=1
fi
if [ -z "$B_OK" ]; then
    set B_OK=0
fi
if [ -z "$A_TRY" ]; then
    set A_TRY=0
fi
if [ -z "$B_TRY" ]; then
    set B_TRY=0
fi

# select bootable slot
for SLOT in $ORDER; do
    if [ "$SLOT" == "A" ]; then
        INDEX=0
        OK=$A_OK
        TRY=$A_TRY
        A_TRY=1
    fi
    if [ "$SLOT" == "B" ]; then
        INDEX=1
        OK=$B_OK
        TRY=$B_TRY
        B_TRY=1
    fi
    if [ "$OK" -eq 1 -a "$TRY" -eq 0 ]; then
        default=$INDEX
        break
    fi
done

# reset booted flags
if [ "$default" -eq 0 ]; then
    if [ "$A_OK" -eq 1 -a "$A_TRY" -eq 1 ]; then
        A_TRY=0
    fi
    if [ "$B_OK" -eq 1 -a "$B_TRY" -eq 1 ]; then
        B_TRY=0
    fi
fi

save_env --file=(hd0,1)/grubenv A_TRY A_OK B_TRY B_OK ORDER

# For more logs add "loglevel=7" to CMDLINE
CMDLINE="quiet"
ROOT_BLOCK_DEVICE_NAME="sda"

menuentry "Slot A (OK=$A_OK TRY=$A_TRY)" {
    linux (hd0,2)/boot/bzImage root=/dev/sda2 $CMDLINE rauc.slot=A
}

menuentry "Slot B (OK=$B_OK TRY=$B_TRY)" {
    linux (hd0,3)/boot/bzImage root=/dev/sda3 $CMDLINE rauc.slot=B
}
EOF

# Initialize grubenv
grub-editenv /grubenv/grubenv set ORDER="A B"
grub-editenv /grubenv/grubenv set A_OK=1
grub-editenv /grubenv/grubenv set B_OK=0
grub-editenv /grubenv/grubenv set A_TRY=0
grub-editenv /grubenv/grubenv set B_TRY=0

echo "RAUC A/B booting configured successfully!"
echo "System will reboot in 10 seconds..."
sleep 10
reboot 