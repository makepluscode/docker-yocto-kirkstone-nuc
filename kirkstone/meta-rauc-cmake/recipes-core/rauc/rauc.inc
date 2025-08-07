DESCRIPTION = "RAUC update controller for host and target (CMake build)"
HOMEPAGE = "https://github.com/rauc/rauc"
LICENSE = "LGPL-2.1-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=4fbd65380cdd255951079008b364516c"
DEPENDS = "openssl glib-2.0 dbus util-linux curl"

inherit cmake pkgconfig gettext

# CMake configuration options
EXTRA_OECMAKE += "\
        -DENABLE_SERVICE=ON \
        -DENABLE_CREATE=ON \
        -DENABLE_NETWORK=ON \
        -DENABLE_GPT=ON \
        -DENABLE_JSON=OFF \
        -DENABLE_STREAMING=OFF \
        -DBUILD_TESTS=OFF \
        "

PACKAGECONFIG[nocreate]  = "-DENABLE_CREATE=OFF,-DENABLE_CREATE=ON,"
PACKAGECONFIG[service] = "-DENABLE_SERVICE=ON,-DENABLE_SERVICE=OFF,dbus,${PN}-service"
PACKAGECONFIG[streaming] = "-DENABLE_STREAMING=ON,-DENABLE_STREAMING=OFF,libnl"
PACKAGECONFIG[network] = "-DENABLE_NETWORK=ON,-DENABLE_NETWORK=OFF,curl"
PACKAGECONFIG[json]    = "-DENABLE_JSON=ON,-DENABLE_JSON=OFF,json-glib"
PACKAGECONFIG[gpt]     = "-DENABLE_GPT=ON,-DENABLE_GPT=OFF,util-linux"

FILES:${PN}-dev += "\
  ${datadir}/dbus-1/interfaces/de.pengutronix.rauc.Installer.xml \
  "

# Make sure we use the cross-compilation environment
do_configure:prepend() {
    export PKG_CONFIG_PATH="${STAGING_DIR_TARGET}/usr/lib/pkgconfig:${STAGING_DIR_TARGET}/usr/share/pkgconfig"
    export PKG_CONFIG_SYSROOT_DIR="${STAGING_DIR_TARGET}"
}