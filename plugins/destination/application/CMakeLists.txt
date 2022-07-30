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
#  Additional dependencies for this plugin				#
#									#
#########################################################################

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Service)

#########################################################################
#									#
#  Destination plugin to open the scan with an application		#
#									#
#########################################################################

set(destinationapplication_SRCS
  destinationapplication.cpp
  ${kookadestination_LOG_SRCS}
)

kcoreaddons_add_plugin(destinationapplication SOURCES ${destinationapplication_SRCS} INSTALL_NAMESPACE "kooka_destination")

kcoreaddons_desktop_to_json(destinationapplication kookadestination-application.desktop)
target_link_libraries(destinationapplication Qt5::Core Qt5::Widgets)
target_link_libraries(destinationapplication KF5::I18n KF5::WidgetsAddons KF5::Service)
target_link_libraries(destinationapplication kookadestination kookacore kookascan)
