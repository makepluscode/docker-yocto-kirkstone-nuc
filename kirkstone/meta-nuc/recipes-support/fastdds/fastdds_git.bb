SUMMARY = "eProsima Fast DDS (formerly Fast RTPS)"
DESCRIPTION = "eProsima Fast DDS (formerly Fast RTPS) is a C++ implementation of the DDS (Data Distribution Service) standard of the OMG (Object Management Group). eProsima Fast DDS implements the RTPS (Real Time Publish Subscribe) protocol, which provides publisher-subscriber communications over unreliable transports such as UDP, as defined and maintained by the Object Management Group (OMG) consortium."
HOMEPAGE = "https://github.com/eProsima/Fast-DDS"
SECTION = "libs"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRC_URI = "git://github.com/eProsima/Fast-DDS.git;branch=master;protocol=https"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake

DEPENDS = "cmake-native fast-cdr foonathan-memory openssl libxml2 tinyxml2"

# Fix GNU_HASH QA issue by ensuring proper LDFLAGS
EXTRA_OECMAKE = "-DBUILD_SHARED_LIBS=ON \
                 -DCMAKE_BUILD_TYPE=Release \
                 -DCOMPILE_EXAMPLES=OFF \
                 -DCOMPILE_TOOLS=ON \
                 -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON \
                 -DTHIRDPARTY=ON \
                 -DSECURITY=ON \
                 -DSHM_TRANSPORT_DEFAULT=ON \
                 -DFORCE_CXX=ON \
                 -DSM_RUN_RESULT=0 \
                 -DSM_RUN_RESULT__TRYRUN_OUTPUT=PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP \
                 -DCMAKE_EXE_LINKER_FLAGS='${LDFLAGS} -Wl,--hash-style=gnu -lpthread' \
                 -DCMAKE_SHARED_LINKER_FLAGS='${LDFLAGS} -Wl,--hash-style=gnu -lpthread' \
                 -DCMAKE_MODULE_LINKER_FLAGS='${LDFLAGS} -Wl,--hash-style=gnu -lpthread'"

# Ensure proper compiler and linker flags
TARGET_CC_ARCH += " -ltinyxml2 -lpthread"
LDFLAGS += " -Wl,--hash-style=gnu"

# Disable QA check for GNU_HASH as we're explicitly setting it
INSANE_SKIP:${PN} += "ldflags"

# Ensure proper package naming and dependencies
PROVIDES += "fastdds libfastdds"
RPROVIDES:${PN} += "fastdds libfastdds"

RDEPENDS:${PN} = "fast-cdr foonathan-memory openssl libxml2 tinyxml2"

BBCLASSEXTEND = "native nativesdk"

# Main package - libraries only
FILES:${PN} = "${libdir}/libfastrtps.so.* \
               ${libdir}/libfastdds.so.*"

# Development package
FILES:${PN}-dev = "${includedir} \
                   ${libdir}/libfastrtps.so \
                   ${libdir}/libfastdds.so \
                   ${libdir}/cmake \
                   ${libdir}/pkgconfig \
                   ${datadir}/fastdds/cmake \
                   ${datadir}/fastdds/*.xsd \
                   ${datadir}/fastdds/LICENSE"

# Static development package
FILES:${PN}-staticdev = "${libdir}/libfastrtps.a \
                         ${libdir}/libfastdds.a"

# Tools package - executables and shared data
FILES:${PN}-tools = "${bindir}/fastddsgen \
                     ${bindir}/fast-discovery-server \
                     ${bindir}/fast-discovery-server-1.0.1 \
                     ${bindir}/ros-discovery \
                     ${bindir}/fastdds \
                     ${datadir}/fastdds \
                     ${datadir}/fastdds/cmake \
                     ${datadir}/fastdds/*.xsd \
                     ${datadir}/fastdds/LICENSE"

# Python tools package - all Python tools
FILES:${PN}-python = "/usr/tools/fastdds \
                      /usr/tools/fastdds/* \
                      /usr/tools/fastdds/discovery \
                      /usr/tools/fastdds/discovery/* \
                      /usr/tools/fastdds/shm \
                      /usr/tools/fastdds/shm/* \
                      /usr/tools/fastdds/xml_ci \
                      /usr/tools/fastdds/xml_ci/*"

# Add new packages
PACKAGES += "${PN}-tools ${PN}-python"

# Ensure proper package naming for tools and python packages
PROVIDES:${PN}-tools += "fastdds-tools"
RPROVIDES:${PN}-tools += "fastdds-tools"
PROVIDES:${PN}-python += "fastdds-python"
RPROVIDES:${PN}-python += "fastdds-python"

# Dependencies
RDEPENDS:${PN}-tools += "python3 ${PN}"
RDEPENDS:${PN}-python += "python3 ${PN}"

# Ensure all installed files are packaged
FILES:${PN}-tools += "${datadir}/fastdds"
FILES:${PN}-python += "/usr/tools"