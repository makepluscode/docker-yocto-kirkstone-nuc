inherit bundle

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Ensure nuc-version runs first to establish timestamp
DEPENDS += "nuc-version"

# Use shared timestamp for bundle versioning to match /etc/version
# Set during do_configure to avoid basehash issues during parsing
RAUC_BUNDLE_BUILD ?= ""

python do_configure:prepend() {
    import time
    import os
    
    # Check if NUC_VERSION_TIMESTAMP is already set (by nuc-version recipe)
    shared_timestamp = d.getVar('NUC_VERSION_TIMESTAMP')
    
    if shared_timestamp:
        # Use the shared timestamp to ensure bundle version matches /etc/version
        bundle_timestamp = shared_timestamp
        bb.note(f"Using shared NUC version timestamp for bundle: {bundle_timestamp}")
    else:
        # Force read from nuc-version if timestamp not in metadata
        # This can happen if nuc-version was built in a different bitbake instance
        nuc_version_workdir = d.getVar('TMPDIR') + '/work/all-oe-linux/nuc-version/1.0-r0'
        timestamp_file = nuc_version_workdir + '/temp/nuc_version_timestamp'
        
        if os.path.exists(timestamp_file):
            with open(timestamp_file, 'r') as f:
                bundle_timestamp = f.read().strip()
            bb.note(f"Read NUC version timestamp from file: {bundle_timestamp}")
        else:
            # Generate new timestamp in KST (UTC+9) as fallback
            os.environ['TZ'] = 'Asia/Seoul'
            time.tzset()
            bundle_timestamp = time.strftime("%Y%m%d%H%M%S", time.localtime())
            bb.note(f"Generated new KST timestamp for bundle: {bundle_timestamp}")
        
        # Set the shared timestamp for consistency
        d.setVar('NUC_VERSION_TIMESTAMP', bundle_timestamp)
    
    # Set the bundle build timestamp
    d.setVar('RAUC_BUNDLE_BUILD', bundle_timestamp)
}

RAUC_BUNDLE_COMPATIBLE ?= "intel-i7-x64-nuc-rauc"

RAUC_BUNDLE_SLOTS = "rootfs"
RAUC_SLOT_rootfs[fstype] = "ext4"
RAUC_SLOT_rootfs = "nuc-image-qt5"