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
#  Destination plugin that simply discards the scanned image		#
#									#
#########################################################################

set(destinationnull_SRCS
  destinationnull.cpp
  ${kookadestination_LOG_SRCS}
)

kcoreaddons_add_plugin(destinationnull SOURCES ${destinationnull_SRCS} INSTALL_NAMESPACE "kooka_destination")

target_link_libraries(destinationnull Qt${Q}::Gui)
target_link_libraries(destinationnull kookadestination kookascan)
