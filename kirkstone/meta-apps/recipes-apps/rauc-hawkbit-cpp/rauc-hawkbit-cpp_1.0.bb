SUMMARY = "RAUC Hawkbit C++ Client"
DESCRIPTION = "A C++ application that integrates RAUC with Eclipse Hawkbit for over-the-air updates"
LICENSE = "CLOSED"

PN = "rauc-hawkbit-cpp"

DEPENDS = "dlt-daemon cmake-native pkgconfig-native dbus curl json-c rauc"
RDEPENDS:${PN} = "rauc dbus curl json-c"

SRC_URI = ""

S = "${EXTERNALSRC}"

inherit cmake systemd externalsrc

SYSTEMD_SERVICE:${PN} = "rauc-hawkbit-cpp.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

EXTRA_OECMAKE = ""

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${EXTERNALSRC}/services/rauc-hawkbit-cpp.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += " \
    ${systemd_system_unitdir}/rauc-hawkbit-cpp.service \
    /usr/local/bin/rauc-hawkbit-cpp \
" 