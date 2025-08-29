cmake_minimum_required(VERSION 3.16)
project(update-library VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages (RAUC dependencies)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(DLT dlt)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${GIO_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIRS})

if(DLT_FOUND)
    include_directories(${DLT_INCLUDE_DIRS})
    add_definitions(-DWITH_DLT)
endif()

# RAUC ported source files
set(RAUC_SOURCES
    src/rauc/bundle.c
    src/rauc/install.c
    src/rauc/slot.c
    src/rauc/manifest.c
    src/rauc/checksum.c
    src/rauc/utils.c
    src/rauc/context.c
    src/rauc/config_file.c
    src/rauc/stubs.c
    src/rauc/signature.c
)

# C++ wrapper sources (commented out for now due to build issues)
# set(CPP_SOURCES
#     src/update_client.cpp
#     src/rauc_engine.cpp
# )

# Headers
set(HEADERS
    include/update_client.h
    include/rauc_engine.h
    include/update_types.h
    include/rauc/bundle.h
    include/rauc/install.h
    include/rauc/slot.h
    include/rauc/manifest.h
    include/rauc/checksum.h
    include/rauc/utils.h
    include/rauc/context.h
    include/rauc/config_file.h
    include/rauc/signature.h
)

# Create static library
add_library(update-library STATIC ${RAUC_SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(update-library ${GLIB_LIBRARIES})
target_link_libraries(update-library ${GIO_LIBRARIES})
target_link_libraries(update-library ${OPENSSL_LIBRARIES})
if(DLT_FOUND)
    target_link_libraries(update-library ${DLT_LIBRARIES})
endif()

# Compiler flags
target_compile_options(update-library PRIVATE ${GLIB_CFLAGS_OTHER})
target_compile_options(update-library PRIVATE ${GIO_CFLAGS_OTHER})
target_compile_options(update-library PRIVATE ${OPENSSL_CFLAGS_OTHER})
if(DLT_FOUND)
    target_compile_options(update-library PRIVATE ${DLT_CFLAGS_OTHER})
endif()

# Install targets
install(TARGETS update-library
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install(DIRECTORY include/
    DESTINATION include/update-library
    FILES_MATCHING PATTERN "*.h")

# CLI test application for bundle installation
add_executable(update-test-app update-test-app.c)
target_link_libraries(update-test-app update-library)
