# Common class for application recipes
# Provides common functionality for all applications

inherit systemd

# Default systemd service configuration
SYSTEMD_AUTO_ENABLE ?= "enable"

# Common dependencies for GUI applications
DEPENDS += "qtbase qtdeclarative qtquickcontrols2"

# Common runtime dependencies
RDEPENDS:${PN} += "qtbase qtdeclarative qtquickcontrols2 qtgraphicaleffects"

# Common install function for desktop applications
do_install:append() {
    # Install desktop file if it exists
    if [ -f ${WORKDIR}/${PN}.desktop ]; then
        install -d ${D}${datadir}/applications
        install -m 0644 ${WORKDIR}/${PN}.desktop ${D}${datadir}/applications/
    fi
    
    # Install systemd service if it exists
    if [ -f ${WORKDIR}/${PN}.service ]; then
        install -d ${D}${systemd_system_unitdir}
        install -m 0644 ${WORKDIR}/${PN}.service ${D}${systemd_system_unitdir}/
    fi
}

# Common files to include
FILES:${PN} += " \
    ${datadir}/applications/*.desktop \
    ${systemd_system_unitdir}/*.service \
" 