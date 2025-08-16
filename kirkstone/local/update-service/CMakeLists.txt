cmake_minimum_required(VERSION 3.10)
project(update-service)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DLT REQUIRED automotive-dlt)
pkg_check_modules(DBUS REQUIRED dbus-1)

add_executable(update-service
    src/main.cpp
    src/update_service.cpp
)

# Optional test client
add_executable(test-client
    test/test_client.cpp
)

target_include_directories(update-service PRIVATE
    ${DLT_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
)

target_link_libraries(update-service
    ${DLT_LIBRARIES}
    ${DBUS_LIBRARIES}
)

target_compile_options(update-service PRIVATE
    ${DLT_CFLAGS_OTHER}
    ${DBUS_CFLAGS_OTHER}
)

target_include_directories(test-client PRIVATE
    ${DBUS_INCLUDE_DIRS}
)

target_link_libraries(test-client
    ${DBUS_LIBRARIES}
)

target_compile_options(test-client PRIVATE
    ${DBUS_CFLAGS_OTHER}
)

install(TARGETS update-service
    DESTINATION /usr/local/bin
)

install(FILES services/update-service.service
    DESTINATION /etc/systemd/system
)

install(FILES dbus-service/org.freedesktop.UpdateService.conf
    DESTINATION /etc/dbus-1/system.d
)

install(FILES dbus-service/org.freedesktop.UpdateService.service
    DESTINATION /usr/share/dbus-1/system-services
)