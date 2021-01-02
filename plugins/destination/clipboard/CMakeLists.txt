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
#  Destination plugin for copying the scanned image to the clipboard	#
#									#
#########################################################################

set(kookadestinationclipboard_SRCS
  destinationclipboard.cpp
  ${kookadestination_LOG_SRCS}
)

add_library(kookadestinationclipboard MODULE ${kookadestinationclipboard_SRCS})
kcoreaddons_desktop_to_json(kookadestinationclipboard kookadestination-clipboard.desktop)
target_link_libraries(kookadestinationclipboard Qt5::Gui)
target_link_libraries(kookadestinationclipboard kookadestination kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookadestinationclipboard DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookadestination-clipboard.desktop DESTINATION ${SERVICES_INSTALL_DIR})
