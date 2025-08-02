DESCRIPTION = "Intel NUC image with Qt5 (QtQuick, QML) support"

# Enable Qt platform plugins for hardware acceleration and font support
PACKAGECONFIG:pn-qtbase = "eglfs linuxfb kms gbm freetype fontconfig accessibility dbus udev evdev widgets tools libs release"

IMAGE_FEATURES += "splash ssh-server-openssh"

IMAGE_INSTALL = "\
    packagegroup-core-boot \
    packagegroup-core-full-cmdline \
    ${CORE_IMAGE_EXTRA_INSTALL} \
    intel-nuc-init \
    rauc \
    qtbase \
    qtbase-plugins \
    qtbase-tools \
    qtdeclarative \
    qtdeclarative-plugins \
    qtquickcontrols \
    qtquickcontrols2 \
    qtgraphicaleffects \
    dashboard \
    libdrm \
    libgbm \
    mesa \
    mesa-megadriver \
    kernel-modules \
    fontconfig \
    fontconfig-utils \
    ttf-dejavu-common \
    ttf-dejavu-sans \
    ttf-dejavu-serif \
    "

inherit core-image

IMAGE_FSTYPES = "ext4 wic"
DEPLOYABLE_IMAGE_TYPES = "ext4 wic"
IMAGE_TYPEDEP_wic = "ext4"

WKS_FILE = "image-installer.wks.in"
INITRD_IMAGE_LIVE = "core-image-minimal-initramfs"
do_image_wic[depends] += "${INITRD_IMAGE_LIVE}:do_image_complete"
do_rootfs[depends] += "virtual/kernel:do_deploy"

IMAGE_BOOT_FILES:append = "\
    ${KERNEL_IMAGETYPE} \
    microcode.cpio \
    ${IMGDEPLOYDIR}/${IMAGE_BASENAME}-${MACHINE}.ext4;rootfs.img \
    ${@bb.utils.contains('EFI_PROVIDER', 'grub-efi', 'grub-efi-bootx64.efi;EFI/BOOT/bootx64.efi', '', d)} \
    ${@bb.utils.contains('EFI_PROVIDER', 'grub-efi', 'grub.cfg;EFI/BOOT/grub.cfg', '', d)} \
"

do_image_wic[depends] += "${PN}:do_image_ext4" 

# Add post-install script for RAUC configuration
SRC_URI += "file://rauc-post-install.sh"
do_install:append() {
    install -d ${D}/etc/rauc
    install -m 755 ${WORKDIR}/rauc-post-install.sh ${D}/etc/rauc/
} 