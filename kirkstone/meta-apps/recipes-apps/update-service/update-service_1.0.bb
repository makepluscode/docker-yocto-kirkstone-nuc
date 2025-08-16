SUMMARY = "Update Service - RAUC D-Bus Broker"
DESCRIPTION = "A D-Bus broker service that provides abstraction between update-agent and RAUC"
LICENSE = "CLOSED"

PN = "update-service"

DEPENDS = "dlt-daemon dbus cmake-native pkgconfig-native rauc"
RDEPENDS:${PN} = "rauc"

SRC_URI = ""

S = "${EXTERNALSRC}"

inherit cmake externalsrc systemd

SYSTEMD_SERVICE:${PN} = "update-service.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

EXTRA_OECMAKE = ""

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${EXTERNALSRC}/services/update-service.service ${D}${systemd_system_unitdir}/
    
    install -d ${D}${sysconfdir}/dbus-1/system.d
    install -m 0644 ${EXTERNALSRC}/dbus-service/org.freedesktop.UpdateService.conf ${D}${sysconfdir}/dbus-1/system.d/
    
    install -d ${D}${datadir}/dbus-1/system-services
    install -m 0644 ${EXTERNALSRC}/dbus-service/org.freedesktop.UpdateService.service ${D}${datadir}/dbus-1/system-services/
}

FILES:${PN} += " \
    ${systemd_system_unitdir}/update-service.service \
    /usr/local/bin/update-service \
    ${sysconfdir}/dbus-1/system.d/org.freedesktop.UpdateService.conf \
    ${datadir}/dbus-1/system-services/org.freedesktop.UpdateService.service \
"