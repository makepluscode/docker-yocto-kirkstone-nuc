cmake_minimum_required(VERSION 3.16)
project(UpdaterGUI VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Quick
    QuickControls2
    Network
)

# Qt6 policy settings (only for Qt 6.5+)
if(Qt6_VERSION VERSION_GREATER_EQUAL "6.5.0")
    qt_policy(SET QTP0001 NEW)
endif()

# Source files (minimal implementation)
set(SOURCES
    src/main.cpp
)

# QML files
set(QML_FILES
    qml/main.qml
    qml/style/Style.qml
    qml/style/CommonStrings.qml
    qml/components/HeaderBar.qml
)

# Create executable
qt_add_executable(UpdaterGUI ${SOURCES})

# Set Qt6 properties
set_target_properties(UpdaterGUI PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.updater.gui
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Add QML module (only if Qt version supports it)
if(Qt6_VERSION VERSION_GREATER_EQUAL "6.2.0")
    qt_add_qml_module(UpdaterGUI
        URI Updater
        VERSION 1.0
        QML_FILES ${QML_FILES}
    )
endif()

# Link Qt6 libraries
target_link_libraries(UpdaterGUI PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
)

# Compiler definitions
target_compile_definitions(UpdaterGUI PRIVATE
    QT_QML_DEBUG
    APP_VERSION="${PROJECT_VERSION}"
)

# Include directories
target_include_directories(UpdaterGUI PRIVATE
    src
)

# Install rules
install(TARGETS UpdaterGUI
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# Configuration summary
message(STATUS "=== Updater GUI Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "==================================")