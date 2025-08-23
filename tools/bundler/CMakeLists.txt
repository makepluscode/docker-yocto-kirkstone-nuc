cmake_minimum_required(VERSION 3.10)
project(rauc-bundler VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG")

# Source files
set(SOURCES
    src/bundler.c
)

# Create executable
add_executable(bundler ${SOURCES})

# Install target
install(TARGETS bundler
    RUNTIME DESTINATION bin
)

# Install scripts
install(DIRECTORY scripts/
    DESTINATION bin
    FILES_MATCHING PATTERN "*.sh"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

# Install documentation
install(DIRECTORY docs/
    DESTINATION share/doc/rauc-bundler
    FILES_MATCHING PATTERN "*.md"
)

# Create package
include(CPack)
set(CPACK_PACKAGE_NAME "rauc-bundler")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RAUC Bundle Creation Tool")
set(CPACK_PACKAGE_VENDOR "Example Org")
set(CPACK_PACKAGE_CONTACT "admin@example.org")

# Enable testing
enable_testing()
add_test(NAME bundler_test COMMAND bundler --help) 