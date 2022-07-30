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

set(destinationclipboard_SRCS
  destinationclipboard.cpp
  ${kookadestination_LOG_SRCS}
)

kcoreaddons_add_plugin(destinationclipboard SOURCES ${destinationclipboard_SRCS} INSTALL_NAMESPACE "kooka_destination")

kcoreaddons_desktop_to_json(destinationclipboard kookadestination-clipboard.desktop)
target_link_libraries(destinationclipboard Qt5::Gui)
target_link_libraries(destinationclipboard kookadestination kookascan)
