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
#  Destination plugin for the scan gallery				#
#									#
#########################################################################

set(destinationgallery_SRCS
  destinationgallery.cpp
  ${kookadestination_LOG_SRCS}
)

kcoreaddons_add_plugin(destinationgallery SOURCES ${destinationgallery_SRCS} INSTALL_NAMESPACE "kooka_destination")

target_link_libraries(destinationgallery Qt6::Core Qt6::Gui)
target_link_libraries(destinationgallery KF6::I18n KF6::WidgetsAddons)
target_link_libraries(destinationgallery kookadestination kookascan)
