
project(libdialogutil)

########### dependencies ###############

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n Config KIO WidgetsAddons)

########### libdialogutil ###############

add_definitions(-DTRANSLATION_DOMAIN="libkookascan")

set(dialogutil_SRCS
  dialogbase.cpp
  dialogstatesaver.cpp
  dialogstatewatcher.cpp
  recentsaver.cpp
  imagefilter.cpp
)

set(dialogutil_HDRS
  dialogbase.h
  dialogstatesaver.h
  dialogstatewatcher.h
  recentsaver.h
  imagefilter.h
  ${CMAKE_CURRENT_BINARY_DIR}/libdialogutil_export.h
)

add_library(dialogutil SHARED ${dialogutil_SRCS})
generate_export_header(dialogutil BASE_NAME libdialogutil)
target_link_libraries(dialogutil
  Qt5::Core Qt5::Widgets
  KF5::I18n
  KF5::ConfigCore
  KF5::WidgetsAddons
  KF5::KIOCore KF5::KIOFileWidgets
)

set_target_properties(dialogutil PROPERTIES VERSION 1.0.0 SOVERSION 1)

########### package config ###############

set(DU "DialogUtil")
set(CONFIGDIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/${DU}")

ecm_configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/${DU}Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${DU}Config.cmake"
  INSTALL_DESTINATION ${CONFIGDIR})

########### installation ###############

install(TARGETS dialogutil ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${dialogutil_HDRS} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${DU}Config.cmake" DESTINATION ${CONFIGDIR} COMPONENT Devel)
