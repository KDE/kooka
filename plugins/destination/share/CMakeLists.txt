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


#########################################################################
#									#
#  Destination plugin for sharing the scanned image via Purpose		#
#									#
#########################################################################

set(destinationshare_SRCS
  destinationshare.cpp
  ${kookadestination_LOG_SRCS}
)

kcoreaddons_add_plugin(destinationshare SOURCES ${destinationshare_SRCS} INSTALL_NAMESPACE "kooka_destination")

target_link_libraries(destinationshare Qt6::Gui)
target_link_libraries(destinationshare KF6::Purpose KF6::PurposeWidgets)
target_link_libraries(destinationshare kookadestination kookascan)
