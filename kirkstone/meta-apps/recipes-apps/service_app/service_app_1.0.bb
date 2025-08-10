SUMMARY = "Service Application with DLT logging"
DESCRIPTION = "A C++ service application that prints hello messages and logs heartbeat via DLT"
LICENSE = "CLOSED"

PN = "service-app"

DEPENDS = "dlt-daemon cmake-native pkgconfig-native"

SRC_URI = ""

S = "${EXTERNALSRC}"

inherit cmake systemd externalsrc

SYSTEMD_SERVICE:${PN} = "service_app.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

EXTRA_OECMAKE = ""

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${EXTERNALSRC}/services/service_app.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += " \
    ${systemd_system_unitdir}/service_app.service \
    /usr/local/bin/service_app \
"