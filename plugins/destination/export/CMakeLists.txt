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
#  Destination plugin for exporting the scanned image via Kipi		#
#									#
#########################################################################

set(kookadestinationexport_SRCS
  destinationexport.cpp
  exportkipiinterface.cpp
  exportinfoshared.cpp
  exportcollectionshared.cpp
  exportcollectionselector.cpp
  ${kookadestination_LOG_SRCS}
)

add_library(kookadestinationexport MODULE ${kookadestinationexport_SRCS})
kcoreaddons_desktop_to_json(kookadestinationexport kookadestination-export.desktop)
target_link_libraries(kookadestinationexport Qt5::Gui)
target_link_libraries(kookadestinationexport KF5::Kipi)
target_link_libraries(kookadestinationexport kookadestination kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookadestinationexport DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookadestination-export.desktop DESTINATION ${SERVICES_INSTALL_DIR})
