SUMMARY = "TinyXML2 is a simple, small, efficient, C++ XML parser"
DESCRIPTION = "TinyXML2 is a simple, small, efficient, C++ XML parser that can be easily integrated into other programs."
HOMEPAGE = "https://github.com/leethomason/tinyxml2"
SECTION = "libs"
LICENSE = "Zlib"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=135624eef03e1f1101b9ba9ac9b5fffd"

SRC_URI = "git://github.com/leethomason/tinyxml2.git;branch=master;protocol=https"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake

DEPENDS = "cmake-native"

EXTRA_OECMAKE = "-DBUILD_SHARED_LIBS=ON \
                 -DBUILD_STATIC_LIBS=OFF \
                 -DCMAKE_BUILD_TYPE=Release \
                 -DCMAKE_POSITION_INDEPENDENT_CODE=ON"

BBCLASSEXTEND = "native nativesdk"

FILES:${PN} = "${libdir}/libtinyxml2.so.*"
FILES:${PN}-dev = "${includedir} ${libdir}/libtinyxml2.so ${libdir}/cmake ${libdir}/pkgconfig"
FILES:${PN}-staticdev = "${libdir}/libtinyxml2.a"