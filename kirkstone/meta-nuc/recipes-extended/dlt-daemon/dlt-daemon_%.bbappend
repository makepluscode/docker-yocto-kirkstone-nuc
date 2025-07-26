FILESEXTRAPATHS:prepend := "${THISDIR}:"

SRC_URI += "file://dlt.conf \
            file://dlt-system.conf"

do_install:append() {
    install -d ${D}${sysconfdir}
    install -m 0644 ${WORKDIR}/dlt.conf ${D}${sysconfdir}/dlt.conf
    install -m 0644 ${WORKDIR}/dlt-system.conf ${D}${sysconfdir}/dlt-system.conf
} 