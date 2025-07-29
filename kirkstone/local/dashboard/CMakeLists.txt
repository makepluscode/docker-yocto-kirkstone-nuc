cmake_minimum_required(VERSION 3.16)

project(dashboard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt5 AUTOMOC for Q_OBJECT macro processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Quick Network DBus)

set(QML_FILES
    qml/DashboardMain.qml
    qml/DashboardCard.qml
    qml/InfoRow.qml
    qml/RaucCard.qml
    qml/DashboardCard00.qml
    qml/DashboardCard01.qml
    qml/DashboardCard02.qml
    qml/DashboardCard03.qml
    qml/DashboardCard10.qml
    qml/DashboardCard11.qml
    qml/DashboardCard12.qml
    qml/DashboardCard13.qml
    qml/DashboardCard20.qml
    qml/DashboardCard21.qml
    qml/DashboardCard22.qml
    qml/DashboardCard23.qml
)

set(CPP_FILES
    src/main.cpp
    src/system_info.cpp
    src/system_info.h
    src/rauc_manager.cpp
    src/rauc_manager.h
)

qt5_add_resources(QT_RESOURCES resources/dashboard_resources.qrc)

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

# Add DLT (required)
target_include_directories(dashboard PRIVATE /usr/local/oecore-x86_64/sysroots/corei7-64-oe-linux/usr/include)
target_link_libraries(dashboard dlt)
target_compile_definitions(dashboard PRIVATE HAVE_DLT=1)

# Install target
install(TARGETS dashboard
    RUNTIME DESTINATION bin
) 