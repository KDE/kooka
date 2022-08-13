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
#  Destination plugin for the scan gallery				#
#									#
#########################################################################

set(destinationgallery_SRCS
  destinationgallery.cpp
  ${kookadestination_LOG_SRCS}
)

kcoreaddons_add_plugin(destinationgallery SOURCES ${destinationgallery_SRCS} INSTALL_NAMESPACE "kooka_destination")

kcoreaddons_desktop_to_json(destinationgallery kookadestination-gallery.desktop)
target_link_libraries(destinationgallery Qt::Core Qt::Gui)
target_link_libraries(destinationgallery KF5::I18n KF5::WidgetsAddons)
target_link_libraries(destinationgallery kookadestination kookascan)
