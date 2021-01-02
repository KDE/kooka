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

#########################################################################
#									#
#  Options								#
#									#
#########################################################################

option(WITH_KADMOS "Enable the Kadmos OCR engine" false)

#########################################################################
#									#
#  Additional dependencies for OCR					#
#									#
#########################################################################

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n ConfigWidgets TextWidgets IconThemes Sonnet WidgetsAddons)

#########################################################################
#									#
#  OCR plugin base library						#
#									#
#########################################################################

ecm_qt_declare_logging_category(kookaocr_LOG_SRCS
  HEADER "ocr_logging.h"
  IDENTIFIER "OCR_LOG"
  CATEGORY_NAME "ocr"
  EXPORT kookalogging
  DESCRIPTION "ocr plugins (Kooka)")

set(kookaocr_SRCS
  abstractocrengine.cpp
  abstractocrdialogue.cpp
  executablepathdialogue.cpp
  ${kookaocr_LOG_SRCS}
)

add_library(kookaocr SHARED ${kookaocr_SRCS})
set_target_properties(kookaocr PROPERTIES SOVERSION "1.0.0")
target_link_libraries(kookaocr Qt5::Core Qt5::Widgets)
target_link_libraries(kookaocr KF5::I18n KF5::WidgetsAddons KF5::ConfigWidgets KF5::SonnetCore KF5::SonnetUi)
target_link_libraries(kookaocr kookascan kookacore)
target_link_libraries(kookaocr dialogutil)

if (INSTALL_BINARIES)
  install(TARGETS kookaocr ${INSTALL_TARGETS_DEFAULT_ARGS})
endif (INSTALL_BINARIES)
install(FILES kookaocrplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

#########################################################################
#									#
#  Subdirectories							#
#									#
#########################################################################

add_subdirectory(gocr)
add_subdirectory(ocrad)
add_subdirectory(tesseract)
if(WITH_KADMOS)
  add_subdirectory(kadmos)
endif(WITH_KADMOS)
