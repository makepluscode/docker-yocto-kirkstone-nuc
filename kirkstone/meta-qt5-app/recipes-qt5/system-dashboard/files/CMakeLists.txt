cmake_minimum_required(VERSION 3.16)

project(dashboard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt5 AUTOMOC for Q_OBJECT macro processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Quick Network)

set(QML_FILES
    main.qml
    DashboardCard.qml
    InfoRow.qml
    RaucCard.qml
)

set(CPP_FILES
    main.cpp
    systeminfo.cpp
    systeminfo.h
    raucmanager.cpp
    raucmanager.h
)

qt5_add_resources(QT_RESOURCES qml.qrc)

add_executable(dashboard
    ${CPP_FILES}
    ${QT_RESOURCES}
)

set_target_properties(dashboard PROPERTIES OUTPUT_NAME dashboard)

target_link_libraries(dashboard
    Qt5::Core
    Qt5::Quick
    Qt5::Network
    Qt5::DBus
)

# Install target
install(TARGETS dashboard
    RUNTIME DESTINATION bin
) 