DESCRIPTION = "Dashboard Application with Qt5/QML"
HOMEPAGE = "https://github.com/makepluscode"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://main.cpp \
    file://systeminfo.h \
    file://systeminfo.cpp \
    file://raucmanager.h \
    file://raucmanager.cpp \
    file://main.qml \
    file://DashboardCard.qml \
    file://InfoRow.qml \
    file://RaucCard.qml \
    file://CMakeLists.txt \
    file://qml.qrc \
    file://dashboard.service \
    file://dashboard-eglfs.service \
    file://eglfs_kms_config.json \
"

S = "${WORKDIR}"

DEPENDS = "qtbase qtdeclarative qtquickcontrols2"
RDEPENDS:${PN} = "qtbase qtdeclarative qtquickcontrols2 qtgraphicaleffects"

inherit cmake_qt5 systemd

# SystemD service - use LinuxFB as default (more stable)
SYSTEMD_SERVICE:${PN} = "dashboard-eglfs.service"
SYSTEMD_AUTO_ENABLE = "enable"

#do_install append

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/dashboard.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/dashboard-eglfs.service ${D}${systemd_system_unitdir}/

    install -d ${D}${sysconfdir}/qt5
    install -m 0644 ${WORKDIR}/eglfs_kms_config.json ${D}${sysconfdir}/qt5/

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