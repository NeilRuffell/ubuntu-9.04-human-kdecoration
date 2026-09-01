cmake_minimum_required(VERSION 3.20)

project(karmichuman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(QT_MAJOR_VERSION 6)

include(KDEInstallDirs6)
include(KDECMakeSettings)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)
find_package(KF6CoreAddons REQUIRED)
find_package(KDecoration3 REQUIRED)

add_library(karmichuman MODULE
    karmichuman.cpp
    resources.qrc
)

target_link_libraries(karmichuman PRIVATE
    Qt6::Core
    Qt6::Gui
    KF6::CoreAddons
    KDecoration3::KDecoration
)

set_target_properties(karmichuman PROPERTIES
    OUTPUT_NAME org.kde.karmichuman
)

install(
    TARGETS karmichuman
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/org.kde.kdecoration3
)
