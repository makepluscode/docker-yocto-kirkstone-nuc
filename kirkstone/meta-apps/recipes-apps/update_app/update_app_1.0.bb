SUMMARY = "Update Application with DLT logging"
DESCRIPTION = "A C++ application for performing RAUC updates with DLT logging"
LICENSE = "CLOSED"

PN = "update-app"

DEPENDS = "dlt-daemon cmake-native pkgconfig-native rauc"
RDEPENDS:${PN} = "rauc"

SRC_URI = ""

S = "${EXTERNALSRC}"

inherit cmake externalsrc systemd

SYSTEMD_SERVICE:${PN} = "update_app.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

EXTRA_OECMAKE = ""

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${EXTERNALSRC}/services/update_app.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += " \
    ${systemd_system_unitdir}/update_app.service \
    /usr/local/bin/update_app \
"