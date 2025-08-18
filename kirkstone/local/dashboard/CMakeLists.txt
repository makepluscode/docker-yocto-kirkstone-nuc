cmake_minimum_required(VERSION 3.16)

project(dashboard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt5 AUTOMOC for Q_OBJECT macro processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Quick Network DBus Widgets)

set(QML_FILES
    qml/DashboardMain.qml
    qml/DashboardCardBase.qml
    qml/CardInfoRow.qml
    qml/InfoRow.qml
    qml/RaucCard.qml
    qml/Card01.qml
    qml/Card02.qml
    qml/Card03.qml
    qml/Card04.qml
    qml/Card05.qml
    qml/Card06.qml
    qml/Card07.qml
    qml/Card08.qml
    qml/Card09.qml
    qml/Card10.qml
    qml/Card11.qml
    qml/Card12.qml
    qml/Card13.qml
    qml/Card14.qml
    qml/Card15.qml
    qml/Card16.qml
    qml/Card17.qml
    qml/Card18.qml
    qml/Card19.qml
    qml/Card20.qml
    qml/Card21.qml
    qml/Card22.qml
    qml/Card23.qml
    qml/Card24.qml
    qml/Card25.qml
    qml/Card26.qml
    qml/Card27.qml
    qml/Card28.qml
    qml/Card29.qml
    qml/Card30.qml
    qml/UpdatePopup.qml
)

set(CPP_FILES
    src/main.cpp
    src/system_info.cpp
    src/system_info.h
    src/rauc_manager.cpp
    src/rauc_manager.h
    src/rauc_system_manager.cpp
    src/rauc_system_manager.h
    src/grub_manager.cpp
    src/grub_manager.h
    src/update_agent_manager.cpp
    src/update_agent_manager.h
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
    Qt5::Widgets
)

# Add DLT (required)
target_include_directories(dashboard PRIVATE /usr/local/oecore-x86_64/sysroots/corei7-64-oe-linux/usr/include)
target_link_libraries(dashboard dlt)
target_compile_definitions(dashboard PRIVATE HAVE_DLT=1)

# Install target
install(TARGETS dashboard
    RUNTIME DESTINATION bin
)
