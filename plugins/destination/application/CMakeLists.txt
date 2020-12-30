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

set(kookadestinationapplication_SRCS
  destinationapplication.cpp
)
add_library(kookadestinationapplication MODULE ${kookadestinationapplication_SRCS})
kcoreaddons_desktop_to_json(kookadestinationapplication kookadestination-application.desktop)

target_link_libraries(kookadestinationapplication Qt5::Core Qt5::Widgets)
target_link_libraries(kookadestinationapplication KF5::I18n KF5::WidgetsAddons KF5::Service)
target_link_libraries(kookadestinationapplication kookadestination kookacore kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookadestinationapplication DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookadestination-application.desktop DESTINATION ${SERVICES_INSTALL_DIR})
