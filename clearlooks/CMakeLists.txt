cmake_minimum_required(VERSION 3.20)
project(ClearlooksDecoration VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
find_package(ECM REQUIRED NO_MODULE)
list(PREPEND CMAKE_MODULE_PATH "${ECM_MODULE_PATH}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui)
set(QT_MAJOR_VERSION 6)
include(KDEInstallDirs6)
find_package(KF6CoreAddons REQUIRED)
find_package(KDecoration3 REQUIRED)

add_library(clearlooks MODULE clearlooks.cpp)
target_compile_features(clearlooks PRIVATE cxx_std_17)
target_link_libraries(clearlooks PRIVATE Qt6::Gui KF6::CoreAddons KDecoration3::KDecoration)
set_target_properties(clearlooks PROPERTIES PREFIX "" OUTPUT_NAME "org.kde.clearlooks")

install(TARGETS clearlooks DESTINATION "${KDE_INSTALL_PLUGINDIR}/org.kde.kdecoration3")
