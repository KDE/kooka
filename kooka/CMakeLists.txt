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

############### Support for multi-page TIFF images ###############

include(FindTIFF)
if (TIFF_FOUND)
  message(STATUS "TIFF includes: ${TIFF_INCLUDE_DIR}")
  message(STATUS "TIFF libraries: ${TIFF_LIBRARIES}")
  set(HAVE_TIFF true)
else (TIFF_FOUND)
  message(STATUS "TIFF library not found, no multi-page image support")
endif (TIFF_FOUND)

############### Build Options ###############

option(WITH_KADMOS "Enable the Kadmos OCR engine" false)
option(KADMOS_DIR "Path to Kadmos include and library files, default '/usr/local'" "/usr/local")

############### Support for Kadmos OCR ###############

if (WITH_KADMOS)

  find_file(KADMOS_HEADER_FOUND kadmos.h PATHS ${KADMOS_DIR})
  if (KADMOS_HEADER_FOUND)
    string(REGEX REPLACE "/.*$" "" KADMOS_INCLUDE_DIR KADMOS_HEADER_FOUND)
    message(STATUS "Kadmos includes: ${KADMOS_INCLUDE_DIR}")
  else (KADMOS_HEADER_FOUND)
    message(SEND_ERROR "WITH_KADMOS requested, but Kadmos engine header file 'kadmos.h' not found in ${KADMOS_DIR} - set KADMOS_DIR to locate it")
  endif (KADMOS_HEADER_FOUND)

  find_file(KADMOS_LIB_FOUND librep.a PATHS ${KADMOS_DIR})
  if (KADMOS_LIB_FOUND)
    string(REGEX REPLACE "/.*$" "" KADMOS_LIB_DIR KADMOS_LIB_FOUND)
    message(STATUS "Kadmos libraries: ${KADMOS_LIB_DIR}")
  else (KADMOS_LIB_FOUND)
    message(SEND_ERROR "WITH_KADMOS requested, but Kadmos engine library 'librep.a' not found in ${KADMOS_DIR} - set KADMOS_DIR to locate it")
  endif (KADMOS_LIB_FOUND)

  set(HAVE_KADMOS true)
endif (WITH_KADMOS)

########### subdirectories ###############

add_subdirectory(pics)
add_subdirectory(desktop)

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

add_definitions(-DKDE4)
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=28000)
if (HAVE_KADMOS)
  add_definitions(-DHAVE_KADMOS)
endif (HAVE_KADMOS)
if (HAVE_TIFF)
  add_definitions(-DHAVE_TIFF)
endif (HAVE_TIFF)

include_directories(..)
include_directories(./ocr/)

#qt5_generate_dbus_interface(kooka.h org.kde.kooka.kooka.xml OPTIONS -a)

if (HAVE_KADMOS)
  if (KADMOS_INCLUDE_DIR)
    include_directories(${KADMOS_INCLUDE_DIR})
  endif (KADMOS_INCLUDE_DIR)
  if (KADMOS_LIB_DIR)
    link_directories(${KADMOS_LIB_DIR})
  endif (KADMOS_LIB_DIR)
endif (HAVE_KADMOS)

if (HAVE_TIFF)
  include_directories(${TIFF_INCLUDE_DIR})
endif (HAVE_TIFF)

########### i18n ###############

add_definitions(-DTRANSLATION_DOMAIN="kooka")

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
  imgsaver.cpp			# -> imagesaver
  formatdialog.cpp
  kookaimage.cpp
  imageformat.cpp
  imagetransform.cpp
  scangallery.cpp
  galleryhistory.cpp
  kookagallery.cpp
  thumbview.cpp
  kookaprint.cpp
  #QT5 imgprintdialog.cpp
  scanparamsdialog.cpp
  newscanparams.cpp		# -> newscaparamsdialog
  #QT5 photocopyprintdialogpage.cpp
)

set(kooka_ocr_SRCS
  ocr/ocrbasedialog.cpp
  ocr/ocrengine.cpp
  ocr/ocrgocrdialog.cpp
  ocr/ocrgocrengine.cpp
  #QT5 ocr/ocrkadmosdialog.cpp
  ocr/ocrkadmosengine.cpp
  ocr/ocrocraddialog.cpp
  ocr/ocrocradengine.cpp
  ocr/ocrresedit.cpp
)

if (WITH_KADMOS)
  set(kooka_SRCS ${kooka_SRCS} kadmosocr.cpp)
endif (WITH_KADMOS)

kde4_add_kcfg_files(kooka_SRCS kookasettings.kcfgc)

add_executable(kooka ${kooka_SRCS} ${kooka_ocr_SRCS})
target_link_libraries(kooka
  kookascan
  libfiletree
  KF5::KDELibs4Support
)

if (HAVE_KADMOS)
  target_link_libraries(kooka librep)
endif (HAVE_KADMOS)

if (HAVE_TIFF)
  target_link_libraries(kooka ${TIFF_LIBRARIES})
endif (HAVE_TIFF)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
add_dependencies(kooka vcsversion)

########### install files ###############

install(TARGETS kooka ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kookasettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
