cmake_minimum_required(VERSION 3.10)
project(service_app)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DLT REQUIRED automotive-dlt)

add_executable(service_app
    src/main.cpp
)

target_include_directories(service_app PRIVATE
    ${DLT_INCLUDE_DIRS}
)

target_link_libraries(service_app
    ${DLT_LIBRARIES}
)

target_compile_options(service_app PRIVATE
    ${DLT_CFLAGS_OTHER}
)

install(TARGETS service_app
    DESTINATION /usr/local/bin
)

install(FILES services/service_app.service
    DESTINATION /etc/systemd/system
)