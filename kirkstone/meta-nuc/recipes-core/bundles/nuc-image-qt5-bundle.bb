inherit bundle

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Use timestamp for bundle versioning
# Set during do_configure to avoid basehash issues during parsing
RAUC_BUNDLE_BUILD ?= ""

do_configure:prepend() {
    import time
    import os
    
    # Get current time in KST (UTC+9)
    os.environ['TZ'] = 'Asia/Seoul'
    time.tzset()
    kst_time = time.strftime("%Y%m%d%H%M%S", time.localtime())
    
    # Set the bundle build timestamp to KST
    d.setVar('RAUC_BUNDLE_BUILD', kst_time)
}

RAUC_BUNDLE_COMPATIBLE ?= "intel-i7-x64-nuc-rauc"

RAUC_BUNDLE_SLOTS = "rootfs"
RAUC_SLOT_rootfs[fstype] = "ext4"
RAUC_SLOT_rootfs = "nuc-image-qt5"