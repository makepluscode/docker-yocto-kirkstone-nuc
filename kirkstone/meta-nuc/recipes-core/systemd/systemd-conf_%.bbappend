FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://20-wired.network"
SRC_URI += "file://system.conf"

FILES:${PN} += "${sysconfdir}/systemd/network/20-wired.network"
FILES:${PN} += "${sysconfdir}/systemd/system.conf"

do_install:append() {
    # Install network configuration
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/20-wired.network ${D}${sysconfdir}/systemd/network/
    
    # Install system configuration for faster shutdown
    install -m 0644 ${WORKDIR}/system.conf ${D}${sysconfdir}/systemd/
} 