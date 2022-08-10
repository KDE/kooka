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

set(kookaocrtesseract_SRCS
  ocrtesseractengine.cpp
  ocrtesseractdialog.cpp
  ${kookaocr_LOG_SRCS}
)
add_library(kookaocrtesseract MODULE ${kookaocrtesseract_SRCS})
kcoreaddons_desktop_to_json(kookaocrtesseract kookaocr-tesseract.desktop)

target_link_libraries(kookaocrtesseract Qt5::Core Qt5::Gui)
target_link_libraries(kookaocrtesseract KF5::I18n KF5::WidgetsAddons)
target_link_libraries(kookaocrtesseract kookaocr kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookaocrtesseract DESTINATION ${KDE_INSTALL_PLUGINDIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookaocr-tesseract.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES tesseract.png DESTINATION ${PICS_INSTALL_DIR})
