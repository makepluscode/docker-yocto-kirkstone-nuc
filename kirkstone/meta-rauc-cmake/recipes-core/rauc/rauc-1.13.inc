# Use local CMake source instead of downloading
FILESEXTRAPATHS:prepend := "${THISDIR}:"
SRC_URI = "file://rauc-cmake-1.13"
S = "${WORKDIR}/rauc-cmake-1.13"

UPSTREAM_CHECK_URI = "https://github.com/${BPN}/${BPN}/releases"

# Since we're using local source, no checksum needed
INSANE_SKIP:${PN} += "already-stripped"