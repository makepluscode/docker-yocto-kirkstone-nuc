# Qt5 Application class
# Inherits from app-common and adds Qt5 specific functionality

inherit app-common cmake_qt5

# Qt5 specific dependencies
DEPENDS += "qtbase qtdeclarative qtquickcontrols2"

# Qt5 specific runtime dependencies
RDEPENDS:${PN} += "qtbase qtdeclarative qtquickcontrols2 qtgraphicaleffects"

# Qt5 specific build configuration
EXTRA_OECMAKE += "-DCMAKE_BUILD_TYPE=Release"

# Qt5 specific install function
do_install:append() {
    # Install Qt5 specific configuration files
    if [ -f ${WORKDIR}/qt5_config.json ]; then
        install -d ${D}${sysconfdir}/qt5
        install -m 0644 ${WORKDIR}/qt5_config.json ${D}${sysconfdir}/qt5/
    fi
}

# Qt5 specific files
FILES:${PN} += " \
    ${sysconfdir}/qt5/*.json \
" 