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

set(kookaocrgocr_SRCS
  ocrgocrengine.cpp
  ocrgocrdialog.cpp
  ${kookaocr_LOG_SRCS}
)

add_library(kookaocrgocr MODULE ${kookaocrgocr_SRCS})
kcoreaddons_desktop_to_json(kookaocrgocr kookaocr-gocr.desktop)

target_link_libraries(kookaocrgocr Qt5::Core Qt5::Gui)
target_link_libraries(kookaocrgocr KF5::I18n KF5::WidgetsAddons)
target_link_libraries(kookaocrgocr kookaocr kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookaocrgocr DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookaocr-gocr.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES gocr.png DESTINATION ${PICS_INSTALL_DIR})
