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
#  OCR plugin for GOCR							#
#									#
#########################################################################

set(ocrgocr_SRCS
  ocrgocrengine.cpp
  ocrgocrdialog.cpp
  ${kookaocr_LOG_SRCS}
)

kcoreaddons_add_plugin(ocrgocr SOURCES ${ocrgocr_SRCS} INSTALL_NAMESPACE "kooka_ocr")

kcoreaddons_desktop_to_json(ocrgocr kookaocr-gocr.desktop)

target_link_libraries(ocrgocr Qt::Core Qt::Gui)
target_link_libraries(ocrgocr KF5::I18n KF5::WidgetsAddons)
target_link_libraries(ocrgocr kookaocr kookascan)

install(FILES gocr.png DESTINATION ${PICS_INSTALL_DIR})
