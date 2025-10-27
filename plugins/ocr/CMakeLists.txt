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
#  Options								#
#									#
#########################################################################

option(WITH_KADMOS "Enable the Kadmos OCR engine" false)

#########################################################################
#									#
#  Additional dependencies for OCR					#
#									#
#########################################################################

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS I18n ConfigWidgets TextWidgets IconThemes Sonnet WidgetsAddons)

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
set_target_properties(kookaocr PROPERTIES VERSION ${VERSION} SOVERSION 6)
target_link_libraries(kookaocr Qt6::Core Qt6::Widgets)
target_link_libraries(kookaocr KF6::I18n KF6::WidgetsAddons KF6::ConfigWidgets KF6::SonnetCore KF6::SonnetUi)
target_link_libraries(kookaocr kookascan kookacore)
target_link_libraries(kookaocr dialogutil)

if (INSTALL_BINARIES)
  install(TARGETS kookaocr ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
endif (INSTALL_BINARIES)

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
