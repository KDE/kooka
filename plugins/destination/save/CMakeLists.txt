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
#  Destination plugin for saving the scanned image directly		#
#									#
#########################################################################

set(kookadestinationsave_SRCS
  destinationsave.cpp
  ${kookadestination_LOG_SRCS}
)

add_library(kookadestinationsave MODULE ${kookadestinationsave_SRCS})
kcoreaddons_desktop_to_json(kookadestinationsave kookadestination-save.desktop)
target_link_libraries(kookadestinationsave Qt5::Gui)
target_link_libraries(kookadestinationsave kookadestination kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookadestinationsave DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookadestination-save.desktop DESTINATION ${SERVICES_INSTALL_DIR})
