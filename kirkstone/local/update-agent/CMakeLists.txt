cmake_minimum_required(VERSION 3.10)
project(update-agent)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DLT REQUIRED automotive-dlt)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON REQUIRED json-c)

# Find required packages for RAUC functionality
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(OPENSSL REQUIRED openssl)

# Enable DLT support
add_definitions(-DWITH_DLT)

# Legacy RAUC source files
set(LEGACY_SOURCES
    src/legacy/bundle.c
    src/legacy/install.c
    src/legacy/slot.c
    src/legacy/manifest.c
    src/legacy/checksum.c
    src/legacy/utils.c
    src/legacy/context.c
    src/legacy/config_file.c
    src/legacy/stubs.c
    src/legacy/signature.c
    src/legacy/bootchooser.c
)

# C++ wrapper sources
set(CPP_SOURCES
    src/update_client.cpp
    src/legacy_engine.cpp
)

add_executable(update-agent
    src/main.cpp
    src/server_agent.cpp
    src/package_installer.cpp
    ${LEGACY_SOURCES}
    ${CPP_SOURCES}
)

target_include_directories(update-agent PRIVATE
    ${DLT_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${JSON_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    src
    include
)

target_link_libraries(update-agent
    ${DLT_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)


# Set the correct dynamic linker for target device
if(CMAKE_CROSSCOMPILING)
    set_target_properties(update-agent PROPERTIES
        LINK_FLAGS "-Wl,--dynamic-linker=/lib/ld-linux-x86-64.so.2"
    )
endif()

target_compile_options(update-agent PRIVATE
    ${DLT_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
    ${JSON_CFLAGS_OTHER}
    ${GLIB_CFLAGS_OTHER}
    ${GIO_CFLAGS_OTHER}
    ${OPENSSL_CFLAGS_OTHER}
)

install(TARGETS update-agent
    DESTINATION /usr/local/bin
)

install(FILES services/update-agent.service
    DESTINATION /etc/systemd/system
)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add test subdirectory (disabled for now)
# add_subdirectory(tests)
