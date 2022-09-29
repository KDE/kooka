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
#  OCR plugin for Tesseract						#
#									#
#########################################################################

set(tesseract_SRCS
  ocrtesseractengine.cpp
  ocrtesseractdialog.cpp
  ${kookaocr_LOG_SRCS}
)

kcoreaddons_add_plugin(tesseract SOURCES ${tesseract_SRCS} INSTALL_NAMESPACE "kooka_ocr")

kcoreaddons_desktop_to_json(tesseract kookaocr-tesseract.desktop)

target_link_libraries(tesseract Qt::Core Qt::Gui)
target_link_libraries(tesseract KF5::I18n KF5::WidgetsAddons)
target_link_libraries(tesseract kookaocr kookascan)

install(FILES tesseract.png DESTINATION ${PICS_INSTALL_DIR})
