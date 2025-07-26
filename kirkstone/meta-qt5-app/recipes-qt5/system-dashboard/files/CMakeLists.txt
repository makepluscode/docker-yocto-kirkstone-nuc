cmake_minimum_required(VERSION 3.16)

project(system-dashboard VERSION 1.0.0 LANGUAGES CXX)

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
)

set(CPP_FILES
    main.cpp
    systeminfo.cpp
    systeminfo.h
)

qt5_add_resources(QT_RESOURCES qml.qrc)

add_executable(system-dashboard
    ${CPP_FILES}
    ${QT_RESOURCES}
)

target_link_libraries(system-dashboard
    Qt5::Core
    Qt5::Quick
    Qt5::Network
)

# Install target
install(TARGETS system-dashboard
    RUNTIME DESTINATION bin
) 