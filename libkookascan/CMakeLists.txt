##########################################################################
##									##
##  This CMake file is part of libkscan, a KDE scanning library.	##
##									##
##  This file may be distributed and/or modified under the terms of	##
##  the GNU General Public License version 2, as published by the	##
##  Free Software Foundation and appearing in the file COPYING		##
##  included in the packaging of this file.				##
##									##
##  Author:  Jonathan Marten <jjm AT keelhaul DOT me DOT uk>		##
##									##
##########################################################################

project(kookascan)

########### dependencies ###############

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n Config IconThemes KIO)

# libpaper (at least in Gentoo Linux) provides neither a
# Find<package>.cmake or a pkg-config file.
find_library(LIBPAPER_LIB NAMES "libpaper" "libpaper.so")
find_file(LIBPAPER_HDR "paper.h")
if (LIBPAPER_LIB AND LIBPAPER_HDR)
  message(STATUS "libpaper library: ${LIBPAPER_LIB}")
  message(STATUS "libpaper header: ${LIBPAPER_HDR}")
  set(HAVE_LIBPAPER 1)
else (LIBPAPER_LIB AND LIBPAPER_HDR)
  message(STATUS "libpaper library not found, using built-in list of paper sizes")
endif (LIBPAPER_LIB AND LIBPAPER_HDR)

############### Support for multi-page TIFF images ###############

include(FindTIFF)
if (TIFF_FOUND)
  message(STATUS "TIFF includes: ${TIFF_INCLUDE_DIR}")
  message(STATUS "TIFF libraries: ${TIFF_LIBRARIES}")
  set(HAVE_TIFF true)
else (TIFF_FOUND)
  message(STATUS "TIFF library not found, no multi-page image support")
endif (TIFF_FOUND)

########### subdirectories ###############

add_subdirectory(pics)
add_subdirectory(desktop)

########### setup ###############

if (HAVE_SANE)
  add_definitions(-DHAVE_SANE)
  add_definitions(-DSANE_VERSION=\"${SANE_VERSION}\")
  include_directories(${SANE_INCLUDES})
endif (HAVE_SANE)
if (HAVE_LIBPAPER)
  add_definitions(-DHAVE_LIBPAPER)
  get_filename_component(LIBPAPER_INCLUDES ${LIBPAPER_HDR} PATH)
  include_directories(${LIBPAPER_INCLUDES})
endif (HAVE_LIBPAPER)
if (HAVE_TIFF)
  add_definitions(-DHAVE_TIFF)
  include_directories(${TIFF_INCLUDE_DIR})
endif (HAVE_TIFF)

########### I18N ###############

add_definitions(-DTRANSLATION_DOMAIN="libkookascan")

########### library ###############

# TODO: rename some of these to match class name, others are obsolete
set(kookascan_SRCS
  scanglobal.cpp
  imageformat.cpp
  scanimage.cpp
  kscandevice.cpp
  scandevices.cpp
  kscancontrols.cpp
  kgammatable.cpp
  kscanoption.cpp
  kscanoptset.cpp
  gammadialog.cpp
  gammawidget.cpp
  #scansourcedialog.cpp
  scanparams.cpp
  #massscandialog.cpp
  deviceselector.cpp
  adddevicedialog.cpp
  imagecanvas.cpp
  previewer.cpp
  imgscaledialog.cpp
  sizeindicator.cpp
  scansizeselector.cpp
  autoselectbar.cpp
  autoselectdialog.cpp
  scanicons.cpp
)
kconfig_add_kcfg_files(kookascan_SRCS scansettings.kcfgc)
ecm_qt_declare_logging_category(kookascan_SRCS
  HEADER "libkookascan_logging.h"
  IDENTIFIER "LIBKOOKASCAN_LOG"
  CATEGORY_NAME "libkookascan"
  EXPORT kookalogging
  DESCRIPTION "libkookascan (Kooka)")

add_library(kookascan SHARED ${kookascan_SRCS})
generate_export_header(kookascan BASE_NAME kookascan)
target_link_libraries(kookascan
  dialogutil
  KF5::I18n
  KF5::ConfigCore KF5::ConfigGui KF5::ConfigWidgets
  KF5::KIOCore KF5::KIOWidgets
  KF5::IconThemes
)

if (HAVE_SANE)
  target_link_libraries(kookascan ${SANE_LIBRARIES})
endif (HAVE_SANE)
if (HAVE_LIBPAPER)
  target_link_libraries(kookascan ${LIBPAPER_LIB})
endif (HAVE_LIBPAPER)
if (HAVE_TIFF)
  target_link_libraries(kookascan ${TIFF_LIBRARIES})
endif (HAVE_TIFF)

set_target_properties(kookascan PROPERTIES VERSION 2.0.0 SOVERSION 2)

########### installation ###############

if (INSTALL_BINARIES)
  install(TARGETS kookascan ${INSTALL_TARGETS_DEFAULT_ARGS})
endif (INSTALL_BINARIES)
install(FILES scansettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
