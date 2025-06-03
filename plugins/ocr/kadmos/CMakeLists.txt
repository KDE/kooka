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


#########################################################################
#									#
#  Options
#									#
#########################################################################

option(KADMOS_DIR "Path to Kadmos include and library files, default '/usr/local'" "/usr/local")

#########################################################################
#									#
#  Locating required Kadmos support					#
#									#
#########################################################################

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

#########################################################################
#									#
#  Definitions								#
#									#
#########################################################################

if (HAVE_KADMOS)
  add_definitions(-DHAVE_KADMOS)

  if (KADMOS_INCLUDE_DIR)
    include_directories(${KADMOS_INCLUDE_DIR})
  endif (KADMOS_INCLUDE_DIR)
  if (KADMOS_LIB_DIR)
    link_directories(${KADMOS_LIB_DIR})
  endif (KADMOS_LIB_DIR)
endif (HAVE_KADMOS)

#########################################################################
#									#
#  OCR plugin for Kadmos						#
#									#
#########################################################################

set(kadmos_SRCS
  ocrkadmosengine.cpp
  ocrkadmosdialog.cpp
  kadmosocr.cpp
  ${kookaocr_LOG_SRCS}
)

kcoreaddons_add_plugin(kadmos SOURCES ${kadmos_SRCS} INSTALL_NAMESPACE "kooka_ocr")

target_link_libraries(kadmos Qt6::Core Qt6::Gui)
target_link_libraries(kadmos KF6::I18n KF6::WidgetsAddons)
target_link_libraries(kadmos kookaocr kookascan)
target_link_libraries(kadmos librep)
