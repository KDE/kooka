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
#  Destination plugin for generating multi-page output			#
#									#
#########################################################################

set(destinationmultipage_SRCS
  destinationmultipage.cpp
  multipageoptionsdialog.cpp
  abstractmultipagewriter.cpp
  multipagepdfwriter.cpp
  ${kookadestination_LOG_SRCS}
)

find_program(TIFFCP_COMMAND "tiffcp")
if (TIFFCP_COMMAND)
  add_definitions("-DHAVE_TIFF")
  set(destinationmultipage_SRCS ${destinationmultipage_SRCS} multipagetiffwriter.cpp)
  message(STATUS "Found tiffcp(1) command: ${TIFFCP_COMMAND}")
else ()
  message(WARNING "tiffcp(1) command not found, multipage TIFF output not available")
endif ()

kcoreaddons_add_plugin(destinationmultipage SOURCES ${destinationmultipage_SRCS} INSTALL_NAMESPACE "kooka_destination")

target_link_libraries(destinationmultipage Qt${Q}::Gui)
target_link_libraries(destinationmultipage kookadestination kookascan)
