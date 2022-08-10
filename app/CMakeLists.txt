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

project(kooka5)

########### dependencies ###############

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS PrintSupport)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n Config TextWidgets KIO IconThemes)

############### Packages used by KDE ###############

include(CheckIncludeFile)
include(CheckFunctionExists)

############### Find the stuff we need ###############

check_function_exists(strerror HAVE_STRERROR)
if (HAVE_STRERROR)
  check_include_file(errno.h HAVE_ERRNO_H)		# pointless if no strerror()
endif (HAVE_STRERROR)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)

########### setup ###############

if (HAVE_ERRNO_H)
  add_definitions(-DHAVE_ERRNO_H)
endif (HAVE_ERRNO_H)
if (HAVE_STRERROR)
  add_definitions(-DHAVE_STRERROR)
endif (HAVE_STRERROR)
if (HAVE_SYS_STAT_H)
  add_definitions(-DHAVE_SYS_STAT_H)
endif (HAVE_SYS_STAT_H)
if (HAVE_SYS_TYPES_H)
  add_definitions(-DHAVE_SYS_TYPES_H)
endif (HAVE_SYS_TYPES_H)

include_directories(..)

########### logging ###############

ecm_qt_declare_logging_category(kooka_LOG_SRCS
  HEADER "kooka_logging.h"
  IDENTIFIER "KOOKA_LOG"
  CATEGORY_NAME "kooka"
  EXPORT kookalogging
  DESCRIPTION "application (Kooka)")

########### library shared with plugins ###############

set(kookacore_SRCS
  pluginmanager.cpp
  abstractplugin.cpp
  scangallery.cpp
  imgsaver.cpp			# -> imagesaver
  formatdialog.cpp
  galleryroot.cpp
  kookaprint.cpp
  imgprintdialog.cpp
  ${kooka_LOG_SRCS}
)
kconfig_add_kcfg_files(kookacore_SRCS kookasettings.kcfgc)

add_library(kookacore SHARED ${kookacore_SRCS})
generate_export_header(kookacore BASE_NAME kookacore)
set_target_properties(kookacore PROPERTIES SOVERSION "2.0.0")
target_link_libraries(kookacore Qt5::Core Qt5::Gui Qt5::PrintSupport)
target_link_libraries(kookacore KF5::I18n KF5::KIOFileWidgets KF5::KIOWidgets KF5::IconThemes KF5::ConfigGui)
target_link_libraries(kookacore kookascan)
target_link_libraries(kookacore libfiletree)

if (INSTALL_BINARIES)
  install(TARGETS kookacore ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
endif (INSTALL_BINARIES)

########### executable ###############

# TODO: rename some of these files to match class name, some others are obsolete
set(kooka_SRCS
  main.cpp
  kooka.cpp			# -> kookamainwindow
  kookaview.cpp
  statusbarmanager.cpp
  kookapref.cpp			# -> kookaprefs
  prefspages.cpp
  kookascanparams.cpp
  imagetransform.cpp
  galleryhistory.cpp
  kookagallery.cpp
  thumbview.cpp
  kookaprint.cpp
  imgprintdialog.cpp
  scanpresetsdialog.cpp
  newscanpresetdialog.cpp
  ocrresedit.cpp
  ${kooka_LOG_SRCS}
)

add_executable(kooka ${kooka_SRCS})
target_link_libraries(kooka
  Qt5::PrintSupport
  dialogutil
  kookacore
  kookadestination
  kookaocr
  KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets
  KF5::XmlGui
  KF5::TextWidgets
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
add_dependencies(kooka vcsversion)

########### install files ###############

if (INSTALL_BINARIES)
  install(TARGETS kooka ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
endif (INSTALL_BINARIES)
install(FILES kookasettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

########### subdirectories ###############

add_subdirectory(pics)
add_subdirectory(desktop)
