SUMMARY = "Grub configuration file to use with RAUC"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

include conf/image-uefi.conf

RPROVIDES:${PN} += "virtual-grub-bootconf"

SRC_URI += " \
    file://grub.cfg \
    "

S = "${WORKDIR}"

inherit deploy

do_install() {
    # Ensure target directories exist
    install -d ${D}/boot/EFI/BOOT
    install -d ${D}/grubenv/EFI/BOOT

    # Substitute BitBake variables into the template so that the final cfg
    # contains real values (otherwise the kernel cannot find rootfs and
    # panics).
    sed -e "s|@ROOT_BLOCK_DEVICE_NAME@|${ROOT_BLOCK_DEVICE_NAME}|g" \
        -e "s|@GRUB_RAUC_BOOT_CMD@|${GRUB_RAUC_BOOT_CMD}|g" \
        ${WORKDIR}/grub.cfg > ${D}/boot/EFI/BOOT/grub.cfg

    # Copy the same config to the grubenv partition path so both locations
    # stay in sync.
    install -m 644 ${D}/boot/EFI/BOOT/grub.cfg ${D}/grubenv/EFI/BOOT/grub.cfg
}

# Package the installed paths
FILES:${PN} += "/boot/EFI/BOOT/grub.cfg /grubenv/EFI/BOOT/grub.cfg"

do_deploy() {
    install -m 644 ${D}/boot/EFI/BOOT/grub.cfg ${DEPLOYDIR}
}

addtask deploy after do_install before do_build 