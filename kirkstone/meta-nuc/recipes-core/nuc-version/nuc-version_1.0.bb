SUMMARY = "Creates /etc/version with KST timestamp for NUC bundles"
DESCRIPTION = "This recipe creates /etc/version file with Korea Standard Time (KST) timestamp"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# This package has no source files
inherit allarch

# Version timestamp variable that can be shared with bundle recipes
NUC_VERSION_TIMESTAMP ?= ""

do_configure[noexec] = "1"
do_compile[noexec] = "1"

python do_install:prepend() {
    import time
    import os
    
    # Generate timestamp in KST (Korea Standard Time) if not already set
    if not d.getVar('NUC_VERSION_TIMESTAMP'):
        # Get current time in KST (UTC+9)
        os.environ['TZ'] = 'Asia/Seoul'
        time.tzset()
        kst_time = time.strftime("%Y%m%d%H%M%S", time.localtime())
        
        # Set the version timestamp
        d.setVar('NUC_VERSION_TIMESTAMP', kst_time)
        bb.note(f"Generated NUC version timestamp: {kst_time}")
}

python do_install() {
    import os
    
    # Create /etc directory
    etc_dir = os.path.join(d.getVar('D'), 'etc')
    bb.utils.mkdirhier(etc_dir)
    
    # Get the timestamp from the variable
    timestamp = d.getVar('NUC_VERSION_TIMESTAMP')
    
    if not timestamp:
        bb.error("NUC_VERSION_TIMESTAMP not set")
        return
    
    # Write timestamp to /etc/sw-version (avoid conflict with Yocto's reproducible build)
    version_file = os.path.join(etc_dir, 'sw-version')
    with open(version_file, 'w') as f:
        f.write(timestamp + '\n')
        
    bb.note(f"Created /etc/sw-version with KST timestamp: {timestamp}")
}

FILES:${PN} = "/etc/sw-version"

# This package should be installed in all images that need version tracking
RDEPENDS:${PN} = ""