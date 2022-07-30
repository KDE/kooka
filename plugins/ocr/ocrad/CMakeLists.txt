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
#  OCR plugin for OCRAD							#
#									#
#########################################################################

set(ocrad_SRCS
  ocrocradengine.cpp
  ocrocraddialog.cpp
  ${kookaocr_LOG_SRCS}
)

kcoreaddons_add_plugin(ocrad SOURCES ${ocrad_SRCS} INSTALL_NAMESPACE "kooka_ocr")

kcoreaddons_desktop_to_json(ocrad kookaocr-ocrad.desktop)

target_link_libraries(ocrad Qt5::Core Qt5::Gui)
target_link_libraries(ocrad KF5::I18n KF5::WidgetsAddons)
target_link_libraries(ocrad kookaocr kookascan)

install(FILES ocrad.png DESTINATION ${PICS_INSTALL_DIR})
