# Override grub-efi to use our custom grub.cfg instead of auto-generated one
# This ensures our RAUC A/B boot configuration is properly applied

# Ensure our custom grub configuration is used
do_install:append() {
    # Remove any auto-generated grub.cfg
    rm -f ${D}${EFI_FILES_PATH}/grub.cfg
    
    # Install our custom grub.cfg from rauc-qemu-grubconf
    if [ -f "${STAGING_DIR_TARGET}/boot/EFI/BOOT/grub.cfg" ]; then
        install -m 644 ${STAGING_DIR_TARGET}/boot/EFI/BOOT/grub.cfg ${D}${EFI_FILES_PATH}/grub.cfg
    fi
} 