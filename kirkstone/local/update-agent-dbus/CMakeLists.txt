cmake_minimum_required(VERSION 3.10)
project(update-agent)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DLT REQUIRED automotive-dlt)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON REQUIRED json-c)

add_executable(update-agent
    src/main.cpp
    src/server_agent.cpp
    src/service_agent.cpp
)

target_include_directories(update-agent PRIVATE
    ${DLT_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${JSON_INCLUDE_DIRS}
    src
)

target_link_libraries(update-agent
    ${DLT_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
)


# Set the correct dynamic linker for target device
if(CMAKE_CROSSCOMPILING)
    set_target_properties(update-agent PROPERTIES
        LINK_FLAGS "-Wl,--dynamic-linker=/lib/ld-linux-x86-64.so.2"
    )
endif()

target_compile_options(update-agent PRIVATE
    ${DLT_CFLAGS_OTHER}
    ${DBUS_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
    ${JSON_CFLAGS_OTHER}
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

# Add test subdirectory
add_subdirectory(tests)
