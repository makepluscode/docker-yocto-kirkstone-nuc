cmake_minimum_required(VERSION 3.10)
project(rauc-hawkbit-cpp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DLT REQUIRED automotive-dlt)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON REQUIRED json-c)

add_executable(rauc-hawkbit-cpp
    src/main.cpp
    src/hawkbit_client.cpp
    src/rauc_client.cpp
)

target_include_directories(rauc-hawkbit-cpp PRIVATE
    ${DLT_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${JSON_INCLUDE_DIRS}
    src
)

target_link_libraries(rauc-hawkbit-cpp
    ${DLT_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
)

target_compile_options(rauc-hawkbit-cpp PRIVATE
    ${DLT_CFLAGS_OTHER}
    ${DBUS_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
    ${JSON_CFLAGS_OTHER}
)

install(TARGETS rauc-hawkbit-cpp
    DESTINATION /usr/local/bin
)

install(FILES services/rauc-hawkbit-cpp.service
    DESTINATION /etc/systemd/system
) 