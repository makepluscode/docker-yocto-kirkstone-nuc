SUMMARY = "CDR serialization implementation for eProsima Fast DDS"
DESCRIPTION = "eProsima Fast CDR is a C++ serialization library implementing the Common Data Representation (CDR) mechanism defined by the Object Management Group (OMG) consortium."
HOMEPAGE = "https://github.com/eProsima/Fast-CDR"
SECTION = "libs"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRC_URI = "git://github.com/eProsima/Fast-CDR.git;branch=master;protocol=https"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake

DEPENDS = "cmake-native"

EXTRA_OECMAKE = "-DBUILD_SHARED_LIBS=ON \
                 -DCMAKE_BUILD_TYPE=Release"

BBCLASSEXTEND = "native nativesdk"

FILES:${PN} = "${libdir}/libfastcdr.so.*"
FILES:${PN}-dev = "${includedir} \
                   ${libdir}/libfastcdr.so \
                   ${libdir}/cmake \
                   ${libdir}/pkgconfig \
                   ${datadir}/fastcdr"
FILES:${PN}-staticdev = "${libdir}/libfastcdr.a"