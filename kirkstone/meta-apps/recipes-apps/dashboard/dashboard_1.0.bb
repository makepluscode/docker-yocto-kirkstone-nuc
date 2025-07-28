DESCRIPTION = "Dashboard Application with Qt5/QML"
HOMEPAGE = "https://github.com/makepluscode"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = ""

S = "${EXTERNALSRC}"

DEPENDS = "qtbase qtdeclarative qtquickcontrols2 dlt-daemon"
RDEPENDS:${PN} = "qtbase qtdeclarative qtquickcontrols2 qtgraphicaleffects dlt-daemon"

inherit qt5-app externalsrc

# SystemD service - use LinuxFB as default (more stable)
SYSTEMD_SERVICE:${PN} = "dashboard-eglfs.service"

#do_install append

do_install:append() {
    # Install systemd services
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${EXTERNALSRC}/services/dashboard.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${EXTERNALSRC}/services/dashboard-eglfs.service ${D}${systemd_system_unitdir}/

    # Install Qt5 configuration
    install -d ${D}${sysconfdir}/qt5
    install -m 0644 ${EXTERNALSRC}/config/qt5_eglfs_config.json ${D}${sysconfdir}/qt5/qt5_config.json

    # Create desktop file
    install -d ${D}${datadir}/applications
    cat > ${D}${datadir}/applications/dashboard.desktop << EOF
[Desktop Entry]
Name=Dashboard
Comment=Real-time system monitoring dashboard
Exec=/usr/bin/dashboard
Icon=utilities-system-monitor
Type=Application
Categories=System;Monitor;
EOF
}

FILES:${PN} += " \
    ${systemd_system_unitdir}/dashboard.service \
    ${systemd_system_unitdir}/dashboard-eglfs.service \
    ${sysconfdir}/qt5/eglfs_kms_config.json \
    ${datadir}/applications/dashboard.desktop \
" 