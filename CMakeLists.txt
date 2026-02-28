cmake_minimum_required(VERSION 3.16)

project(DKT_VPN VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt auto-tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 first, fall back to Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

message(STATUS "Using Qt ${Qt${QT_VERSION_MAJOR}_VERSION}")

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/vpnmanager.cpp
)

set(HEADERS
    src/mainwindow.h
    src/vpnmanager.h
    src/vpnserver.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(DKT_VPN
        MANUAL_FINALIZATION
        ${SOURCES}
        ${HEADERS}
    )
else()
    if(ANDROID)
        add_library(DKT_VPN SHARED ${SOURCES} ${HEADERS})
    else()
        add_executable(DKT_VPN ${SOURCES} ${HEADERS})
    endif()
endif()

target_link_libraries(DKT_VPN PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(DKT_VPN PRIVATE src)

# macOS bundle settings
set_target_properties(DKT_VPN PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.dkt.vpn
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Install rules
include(GNUInstallDirs)
install(TARGETS DKT_VPN
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY configs/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dkt-vpn/configs
    FILES_MATCHING PATTERN "*.conf.template"
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    qt_finalize_executable(DKT_VPN)
endif()
