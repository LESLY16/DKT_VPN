cmake_minimum_required(VERSION 3.16)
project(DKT_VPN VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(dkt_vpn
    src/main.cpp
    src/MainWindow.cpp
    src/VPNManager.cpp
)

target_include_directories(dkt_vpn PRIVATE src)
target_link_libraries(dkt_vpn PRIVATE Qt6::Widgets)