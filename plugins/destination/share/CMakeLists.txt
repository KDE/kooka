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
#  Destination plugin for sharing the scanned image via Purpose		#
#									#
#########################################################################

set(kookadestinationshare_SRCS
  destinationshare.cpp
  ${kookadestination_LOG_SRCS}
)

add_library(kookadestinationshare MODULE ${kookadestinationshare_SRCS})
kcoreaddons_desktop_to_json(kookadestinationshare kookadestination-share.desktop)
target_link_libraries(kookadestinationshare Qt5::Gui)
target_link_libraries(kookadestinationshare KF5::Purpose KF5::PurposeWidgets)
target_link_libraries(kookadestinationshare kookadestination kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookadestinationshare DESTINATION ${KDE_INSTALL_PLUGINDIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookadestination-share.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
