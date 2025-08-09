cmake_minimum_required(VERSION 3.16)
project(rauc VERSION 1.13 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

# Configuration
option(ENABLE_SERVICE "Enable/Disable background service" ON)
option(ENABLE_CREATE "Enable/Disable bundle creation and modification commands" ON)
option(ENABLE_NETWORK "Enable/Disable network update mode" ON)
option(ENABLE_STREAMING "Enable/Disable streaming update mode" ON)
option(ENABLE_JSON "Enable/Disable JSON support" ON)
option(ENABLE_GPT "Enable/Disable GPT support" ON)
option(BUILD_TESTS "Enable/Disable test suite" OFF)

set(STREAMING_USER "nobody" CACHE STRING "Unprivileged user for the streaming subprocess")

# Find dependencies
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 REQUIRED glib-2.0>=2.56)
pkg_check_modules(GIO2 REQUIRED gio-2.0>=2.56)
pkg_check_modules(GIO_UNIX REQUIRED gio-unix-2.0>=2.56)
pkg_check_modules(OPENSSL REQUIRED openssl>=1.1.1)

if(ENABLE_SERVICE)
    pkg_check_modules(DBUS dbus-1)
    if(NOT DBUS_FOUND)
        message(FATAL_ERROR "D-Bus required for service support")
    endif()
endif()

if(ENABLE_JSON)
    pkg_check_modules(JSON_GLIB json-glib-1.0)
    if(NOT JSON_GLIB_FOUND)
        set(ENABLE_JSON OFF)
        message(WARNING "JSON-GLib not found, disabling JSON support")
    endif()
endif()

if(ENABLE_GPT)
    pkg_check_modules(FDISK fdisk>=2.29)
    if(NOT FDISK_FOUND)
        set(ENABLE_GPT OFF)
        message(WARNING "libfdisk not found, disabling GPT support")
    endif()
endif()

if(ENABLE_NETWORK)
    pkg_check_modules(CURL libcurl>=7.32.0)
    if(NOT CURL_FOUND)
        set(ENABLE_NETWORK OFF)
        message(WARNING "libcurl not found, disabling network support")
    endif()
endif()

if(ENABLE_STREAMING)
    if(NOT ENABLE_NETWORK)
        message(FATAL_ERROR "Streaming support requires network support")
    endif()
    pkg_check_modules(LIBNL_GENL libnl-genl-3.0>=3.1)
    find_package(Threads REQUIRED)
    if(NOT LIBNL_GENL_FOUND)
        set(ENABLE_STREAMING OFF)
        message(WARNING "libnl-genl-3.0 not found, disabling streaming support")
    endif()
endif()

# Check for headers
include(CheckIncludeFile)
check_include_file("linux/mmc/ioctl.h" HAVE_LINUX_MMC_IOCTL_H)
if(HAVE_LINUX_MMC_IOCTL_H)
    set(ENABLE_EMMC_BOOT_SUPPORT ON)
endif()

if(ENABLE_STREAMING)
    check_include_file("linux/nbd-netlink.h" HAVE_NBD_NETLINK_H)
    if(NOT HAVE_NBD_NETLINK_H)
        message(FATAL_ERROR "Streaming support requested but linux/nbd-netlink.h not found")
    endif()
endif()

# Installation directories
include(GNUInstallDirs)
set(RAUC_BINDIR ${CMAKE_INSTALL_FULL_BINDIR})
set(RAUC_LIBEXECDIR ${CMAKE_INSTALL_FULL_LIBEXECDIR})

# Add target library directories for cross-compilation
if(CMAKE_CROSSCOMPILING)
    link_directories("/usr/local/oecore-x86_64/sysroots/corei7-64-oe-linux/usr/lib")
    link_directories("/usr/local/oecore-x86_64/sysroots/corei7-64-oe-linux/lib")
endif()

# Configure config.h
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

# Configure version.h
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Make config.h available to all source files
add_compile_definitions(HAVE_CONFIG_H)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${CMAKE_CURRENT_BINARY_DIR}/config.h")

# Compiler flags - append to existing flags instead of overriding
set(RAUC_C_FLAGS "-Wall -Wextra")
set(RAUC_C_FLAGS "${RAUC_C_FLAGS} -fdata-sections -ffunction-sections -fno-strict-aliasing")
set(RAUC_C_FLAGS "${RAUC_C_FLAGS} -DG_LOG_DOMAIN=\\\"rauc\\\"")
set(RAUC_C_FLAGS "${RAUC_C_FLAGS} -D_GNU_SOURCE")
set(RAUC_C_FLAGS "${RAUC_C_FLAGS} -DGLIB_VERSION_MAX_ALLOWED=G_ENCODE_VERSION\\(2,56\\)")
set(RAUC_C_FLAGS "${RAUC_C_FLAGS} -DGLIB_VERSION_MIN_REQUIRED=G_ENCODE_VERSION\\(2,56\\)")

# Append our flags to the existing CMAKE_C_FLAGS
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${RAUC_C_FLAGS}")

# Additional warning flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Walloca -Warray-bounds -Wcast-align")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat=2 -Wformat-security")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wold-style-definition -Wshadow")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes -Wundef")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-error=unused-parameter")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=deprecated-declarations")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# D-Bus code generation (if service enabled)
if(ENABLE_SERVICE AND DBUS_FOUND)
    find_program(GDBUS_CODEGEN gdbus-codegen)
    
    set(DBUS_INTERFACE_XML "${CMAKE_CURRENT_SOURCE_DIR}/src/de.pengutronix.rauc.Installer.xml")
    set(DBUS_GENERATED_H "${CMAKE_CURRENT_BINARY_DIR}/rauc-installer-generated.h")
    set(DBUS_GENERATED_C "${CMAKE_CURRENT_BINARY_DIR}/rauc-installer-generated.c")
    
    if(GDBUS_CODEGEN)
        add_custom_command(
            OUTPUT ${DBUS_GENERATED_H} ${DBUS_GENERATED_C}
            COMMAND ${GDBUS_CODEGEN}
                --interface-prefix de.pengutronix.rauc.
                --c-namespace R
                --generate-c-code rauc-installer-generated
                --c-generate-autocleanup all
                ${DBUS_INTERFACE_XML}
            DEPENDS ${DBUS_INTERFACE_XML}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating D-Bus interface code with autocleanup"
        )
        
        set(DBUS_SOURCES ${DBUS_GENERATED_H} ${DBUS_GENERATED_C})
    else()
        message(WARNING "gdbus-codegen not found, D-Bus service support may be limited")
    endif()
endif()

# Source files
set(RAUC_SOURCES
    src/artifacts.c
    src/bootchooser.c
    src/bundle.c
    src/checksum.c
    src/config_file.c
    src/context.c
    src/crypt.c
    src/dm.c
    src/emmc.c
    src/event_log.c
    src/hash_index.c
    src/install.c
    src/manifest.c
    src/mark.c
    src/mbr.c
    src/mount.c
    src/service.c
    src/shell.c
    src/signature.c
    src/slot.c
    src/stats.c
    src/status_file.c
    src/update_handler.c
    src/update_utils.c
    src/utils.c
    src/verity_hash.c
)

if(ENABLE_NETWORK AND CURL_FOUND)
    list(APPEND RAUC_SOURCES src/network.c)
endif()

if(ENABLE_STREAMING AND LIBNL_GENL_FOUND)
    list(APPEND RAUC_SOURCES src/nbd.c)
endif()

if(ENABLE_GPT AND FDISK_FOUND)
    list(APPEND RAUC_SOURCES src/gpt.c)
endif()

# Create static library
if(ENABLE_SERVICE AND DBUS_FOUND AND DBUS_SOURCES)
    add_library(rauc_lib STATIC ${RAUC_SOURCES} ${DBUS_SOURCES})
else()
    add_library(rauc_lib STATIC ${RAUC_SOURCES})
endif()

# Link libraries for static lib
target_link_libraries(rauc_lib
    ${GLIB2_LIBRARIES}
    ${GIO2_LIBRARIES}
    ${GIO_UNIX_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

if(ENABLE_SERVICE AND DBUS_FOUND)
    target_link_libraries(rauc_lib ${DBUS_LIBRARIES})
endif()

if(ENABLE_JSON AND JSON_GLIB_FOUND)
    target_link_libraries(rauc_lib ${JSON_GLIB_LIBRARIES})
endif()

if(ENABLE_GPT AND FDISK_FOUND)
    target_link_libraries(rauc_lib ${FDISK_LIBRARIES})
endif()

if(ENABLE_NETWORK AND CURL_FOUND)
    target_link_libraries(rauc_lib ${CURL_LIBRARIES})
endif()

if(ENABLE_STREAMING AND LIBNL_GENL_FOUND)
    target_link_libraries(rauc_lib ${LIBNL_GENL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

# Include directories for the library
target_include_directories(rauc_lib PRIVATE
    ${GLIB2_INCLUDE_DIRS}
    ${GIO2_INCLUDE_DIRS}
    ${GIO_UNIX_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

if(ENABLE_SERVICE AND DBUS_FOUND)
    target_include_directories(rauc_lib PRIVATE ${DBUS_INCLUDE_DIRS})
endif()

if(ENABLE_JSON AND JSON_GLIB_FOUND)
    target_include_directories(rauc_lib PRIVATE ${JSON_GLIB_INCLUDE_DIRS})
endif()

if(ENABLE_GPT AND FDISK_FOUND)
    target_include_directories(rauc_lib PRIVATE ${FDISK_INCLUDE_DIRS})
endif()

if(ENABLE_NETWORK AND CURL_FOUND)
    target_include_directories(rauc_lib PRIVATE ${CURL_INCLUDE_DIRS})
endif()

if(ENABLE_STREAMING AND LIBNL_GENL_FOUND)
    target_include_directories(rauc_lib PRIVATE ${LIBNL_GENL_INCLUDE_DIRS})
endif()

# Create executable
add_executable(rauc src/main.c)
target_link_libraries(rauc rauc_lib)

# Include directories for the executable
target_include_directories(rauc PRIVATE
    ${GLIB2_INCLUDE_DIRS}
    ${GIO2_INCLUDE_DIRS}
    ${GIO_UNIX_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

if(ENABLE_SERVICE AND DBUS_FOUND)
    target_include_directories(rauc PRIVATE ${DBUS_INCLUDE_DIRS})
endif()

if(ENABLE_JSON AND JSON_GLIB_FOUND)
    target_include_directories(rauc PRIVATE ${JSON_GLIB_INCLUDE_DIRS})
endif()

if(ENABLE_GPT AND FDISK_FOUND)
    target_include_directories(rauc PRIVATE ${FDISK_INCLUDE_DIRS})
endif()

if(ENABLE_NETWORK AND CURL_FOUND)
    target_include_directories(rauc PRIVATE ${CURL_INCLUDE_DIRS})
endif()

if(ENABLE_STREAMING AND LIBNL_GENL_FOUND)
    target_include_directories(rauc PRIVATE ${LIBNL_GENL_INCLUDE_DIRS})
endif()

# Installation
install(TARGETS rauc DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES rauc.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

if(ENABLE_SERVICE)
    # Install D-Bus interface
    install(FILES src/de.pengutronix.rauc.Installer.xml 
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/interfaces)
    
    # Install D-Bus system configuration
    install(FILES data/de.pengutronix.rauc.conf
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/system.d)
    
    # Configure and install systemd service file
    set(bindir ${CMAKE_INSTALL_FULL_BINDIR})
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/data/rauc.service.in"
        "${CMAKE_CURRENT_BINARY_DIR}/rauc.service"
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rauc.service
            DESTINATION lib/systemd/system)
    
    # Configure and install D-Bus service file
    set(libexecdir ${CMAKE_INSTALL_FULL_LIBEXECDIR})
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/data/de.pengutronix.rauc.service.in"
        "${CMAKE_CURRENT_BINARY_DIR}/de.pengutronix.rauc.service"
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/de.pengutronix.rauc.service
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/system-services)
    
    # Configure and install service helper script
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/data/rauc-service.sh.in"
        "${CMAKE_CURRENT_BINARY_DIR}/rauc-service.sh"
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rauc-service.sh
            DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()