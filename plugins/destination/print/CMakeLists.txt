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
#  Destination plugin for printing the scanned image			#
#									#
#########################################################################

set(kookadestinationprint_SRCS
  destinationprint.cpp
  ${kookadestination_LOG_SRCS}
)

add_library(kookadestinationprint MODULE ${kookadestinationprint_SRCS})
kcoreaddons_desktop_to_json(kookadestinationprint kookadestination-print.desktop)
target_link_libraries(kookadestinationprint Qt5::Gui)
target_link_libraries(kookadestinationprint kookadestination kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookadestinationprint DESTINATION ${KDE_INSTALL_PLUGINDIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookadestination-print.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
