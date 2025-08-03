SUMMARY = "STL compatible C++ memory allocator library"
DESCRIPTION = "This library provides various memory allocators and debugging facilities for C++."
HOMEPAGE = "https://github.com/foonathan/memory"
SECTION = "libs"
LICENSE = "Zlib"
LIC_FILES_CHKSUM = "file://LICENSE;md5=858ac481b3e933be38e4d36cb0dfcee8"

SRC_URI = "git://github.com/foonathan/memory.git;branch=main;protocol=https"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake

DEPENDS = "cmake-native"

EXTRA_OECMAKE = "-DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF \
                 -DFOONATHAN_MEMORY_BUILD_TESTS=OFF \
                 -DFOONATHAN_MEMORY_BUILD_TOOLS=OFF \
                 -DFOONATHAN_MEMORY_CHECK_ALLOCATION_SIZE=ON \
                 -DBUILD_SHARED_LIBS=ON"

BBCLASSEXTEND = "native nativesdk"

# Ensure proper package naming and dependencies
PROVIDES += "foonathan-memory"
RPROVIDES:${PN} += "foonathan-memory"

FILES:${PN} = "${libdir}/libfoonathan_memory*.so.* \
               ${libdir}/libfoonathan_memory*.so"
FILES:${PN}-dev = "${includedir} \
                   ${libdir}/cmake \
                   ${libdir}/pkgconfig \
                   ${libdir}/foonathan_memory/cmake \
                   ${datadir}/foonathan_memory"
FILES:${PN}-staticdev = "${libdir}/libfoonathan_memory*.a"