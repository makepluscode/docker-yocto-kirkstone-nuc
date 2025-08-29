cmake_minimum_required(VERSION 3.10)

project(update-agent-tests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages for tests
find_package(PkgConfig REQUIRED)
pkg_check_modules(DLT REQUIRED automotive-dlt)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON REQUIRED json-c)

# Test executables
add_executable(update-agent-tests
    main.cpp
    test_server_agent.cpp
    test_service_agent.cpp
    test_config.cpp
    test_integration.cpp
    test_server_agent_mocked.cpp
    test_service_agent_mocked.cpp
    mocks/mockable_server_agent.cpp
    mocks/mockable_service_agent.cpp
    ../src/server_agent.cpp
    ../src/service_agent.cpp
)

target_include_directories(update-agent-tests PRIVATE
    ../src
    .
    ${DLT_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${JSON_INCLUDE_DIRS}
)

target_link_libraries(update-agent-tests
    GTest::gtest_main
    GTest::gmock
    ${DLT_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
)

target_compile_options(update-agent-tests PRIVATE
    ${DLT_CFLAGS_OTHER}
    ${DBUS_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
    ${JSON_CFLAGS_OTHER}
)

# Add test to CTest
add_test(NAME update-agent-tests COMMAND update-agent-tests)

# Set test properties
set_tests_properties(update-agent-tests PROPERTIES
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
