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

set(kookaocrocrad_SRCS
  ocrocradengine.cpp
  ocrocraddialog.cpp
  ${kookaocr_LOG_SRCS}
)

add_library(kookaocrocrad MODULE ${kookaocrocrad_SRCS})
kcoreaddons_desktop_to_json(kookaocrocrad kookaocr-ocrad.desktop)

target_link_libraries(kookaocrocrad Qt5::Core Qt5::Gui)
target_link_libraries(kookaocrocrad KF5::I18n KF5::WidgetsAddons)
target_link_libraries(kookaocrocrad kookaocr kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookaocrocrad DESTINATION ${KDE_INSTALL_PLUGINDIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookaocr-ocrad.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES ocrad.png DESTINATION ${PICS_INSTALL_DIR})
