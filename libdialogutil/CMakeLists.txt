##########################################################################
##									##
##  This CMake file is part of Kooka, a KDE scanning/OCR application.	##
##									##
##  This file may be distributed and/or modified under the terms of	##
##  the GNU General Public License version 2, as published by the	##
##  Free Software Foundation and appearing in the file COPYING		##
##  included in the packaging of this file.				##
##									##
##  Author:  Jonathan Marten <jjm AT keelhaul DOT me DOT uk>		##
##									##
##########################################################################

project(libdialogutil)

########### dependencies ###############

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS I18n Config KIO WidgetsAddons)

########### libdialogutil ###############

add_definitions(-DTRANSLATION_DOMAIN="libkookascan")

set(dialogutil_SRCS
  dialogbase.cpp
  dialogstatesaver.cpp
  dialogstatewatcher.cpp
  recentsaver.cpp
  imagefilter.cpp
  clickabletooltip.cpp
)
ecm_qt_declare_logging_category(dialogutil_SRCS
  HEADER "libdialogutil_logging.h"
  IDENTIFIER "LIBDIALOGUTIL_LOG"
  CATEGORY_NAME "libdialogutil"
  EXPORT kookalogging
  DESCRIPTION "libdialogutil (Kooka)")

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
  Qt6::Core Qt6::Widgets
  KF6::I18n
  KF6::ConfigCore
  KF6::WidgetsAddons
  KF6::KIOCore KF6::KIOFileWidgets
)

set_target_properties(dialogutil PROPERTIES VERSION ${VERSION} SOVERSION 6)

########### package config ###############

set(DU "DialogUtil")
set(CONFIGDIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/${DU}")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/${DU}Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${DU}Config.cmake"
  INSTALL_DESTINATION ${CONFIGDIR})

########### installation ###############

install(TARGETS dialogutil ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${dialogutil_HDRS} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/${DU})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${DU}Config.cmake" DESTINATION ${CONFIGDIR} COMPONENT Devel)
