SUMMARY = "OTA Update Agent"
DESCRIPTION = "A C++ application that integrates RAUC with Eclipse Hawkbit for over-the-air updates"
LICENSE = "CLOSED"

PN = "update-agent"

DEPENDS = "dlt-daemon cmake-native pkgconfig-native dbus curl json-c rauc"
RDEPENDS:${PN} = "rauc dbus curl json-c"

SRC_URI = ""

S = "${EXTERNALSRC}"

inherit cmake systemd externalsrc

SYSTEMD_SERVICE:${PN} = "update-agent.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

EXTRA_OECMAKE = ""

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${EXTERNALSRC}/services/update-agent.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += " \
    ${systemd_system_unitdir}/update-agent.service \
    /usr/local/bin/update-agent \
" 