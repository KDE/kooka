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
#  Additional dependencies for scan destinations			#
#									#
#########################################################################

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n CoreAddons WidgetsAddons)

#########################################################################
#									#
#  Destination plugin base library					#
#									#
#########################################################################

set(kookadestination_SRCS
  abstractdestination.cpp
)
add_library(kookadestination SHARED ${kookadestination_SRCS})
set_target_properties(kookadestination PROPERTIES SOVERSION "1.0.0")
target_link_libraries(kookadestination Qt5::Core Qt5::Widgets)
target_link_libraries(kookadestination KF5::I18n KF5::WidgetsAddons)
target_link_libraries(kookadestination kookascan kookacore)

if (INSTALL_BINARIES)
  install(TARGETS kookadestination ${INSTALL_TARGETS_DEFAULT_ARGS})
endif (INSTALL_BINARIES)
install(FILES kookadestinationplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

#########################################################################
#									#
#  Subdirectories							#
#									#
#########################################################################

add_subdirectory(gallery)
