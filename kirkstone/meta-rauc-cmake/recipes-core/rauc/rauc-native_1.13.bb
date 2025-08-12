require rauc-1.13.inc

inherit cmake pkgconfig gettext native deploy

# Native build dependencies - only what's essential for bundle creation
DEPENDS = "openssl-native glib-2.0-native util-linux-native curl-native"

# Override rauc.inc settings that might cause issues
DESCRIPTION = "RAUC update controller (native tools)"
LICENSE = "LGPL-2.1-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=4fbd65380cdd255951079008b364516c"

# Minimal PACKAGECONFIG for native - only bundle creation features
PACKAGECONFIG = "network gpt"
PACKAGECONFIG[network] = "-DENABLE_NETWORK=ON,-DENABLE_NETWORK=OFF,curl-native"
PACKAGECONFIG[gpt] = "-DENABLE_GPT=ON,-DENABLE_GPT=OFF,util-linux-native"

# CMake options - minimal feature set, no service components
EXTRA_OECMAKE = "\
        -DENABLE_SERVICE=OFF \
        -DENABLE_CREATE=ON \
        -DENABLE_JSON=OFF \
        -DENABLE_STREAMING=OFF \
        -DBUILD_TESTS=OFF \
        -DENABLE_NETWORK=ON \
        -DENABLE_GPT=ON \
        "

# Compiler flags to handle deprecated declarations
CFLAGS:append = " -Wno-error=deprecated-declarations"

# Single-threaded build for stability
PARALLEL_MAKE = "-j1"

# Fix CMakeLists.txt to only build the library components we need
do_configure:prepend() {
    # Remove service.c from build
    sed -i '/src\/service\.c/d' ${S}/CMakeLists.txt
    
    # Remove the rauc executable target and its linking - we only need the library for bundle creation
    sed -i '/add_executable.*rauc.*src\/main\.c/,/^$/d' ${S}/CMakeLists.txt
    sed -i '/target_link_libraries.*rauc.*rauc_lib/d' ${S}/CMakeLists.txt
    sed -i '/target_include_directories.*rauc.*PRIVATE/,/^)/d' ${S}/CMakeLists.txt
    
    # Add a simple bundle creation tool instead
    cat >> ${S}/CMakeLists.txt << 'EOF'

# Create minimal bundle creation executable
add_executable(rauc-bundle-creator src/bundle.c src/signature.c)
target_link_libraries(rauc-bundle-creator rauc_lib)
target_include_directories(rauc-bundle-creator PRIVATE
    ${GLIB2_INCLUDE_DIRS}
    ${GIO2_INCLUDE_DIRS}
    ${GIO_UNIX_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)
install(TARGETS rauc-bundle-creator DESTINATION bin)
EOF
}

do_deploy[sstate-outputdirs] = "${DEPLOY_DIR_TOOLS}"
do_deploy() {
    install -d ${DEPLOY_DIR_TOOLS}
    if [ -f ${B}/rauc-bundle-creator ]; then
        install -m 0755 ${B}/rauc-bundle-creator ${DEPLOY_DIR_TOOLS}/rauc-${PV}
        ln -sf rauc-${PV} ${DEPLOY_DIR_TOOLS}/rauc
    elif [ -f ${B}/rauc ]; then
        install -m 0755 ${B}/rauc ${DEPLOY_DIR_TOOLS}/rauc-${PV}
        ln -sf rauc-${PV} ${DEPLOY_DIR_TOOLS}/rauc
    else
        bbwarn "No rauc executable found, checking if library build succeeded"
        ls -la ${B}/ || true
    fi
}

addtask deploy before do_package after do_install