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

set(kookadestinationgallery_SRCS
  destinationgallery.cpp
  ${kookadestination_LOG_SRCS}
)

add_library(kookadestinationgallery MODULE ${kookadestinationgallery_SRCS})
kcoreaddons_desktop_to_json(kookadestinationgallery kookadestination-gallery.desktop)
target_link_libraries(kookadestinationgallery Qt5::Core Qt5::Gui)
target_link_libraries(kookadestinationgallery KF5::I18n KF5::WidgetsAddons)
target_link_libraries(kookadestinationgallery kookadestination kookascan)

if (INSTALL_BINARIES)
  install(TARGETS kookadestinationgallery DESTINATION ${KDE_INSTALL_PLUGINDIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookadestination-gallery.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
