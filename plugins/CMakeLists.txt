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
#  Common definitions for plugins					#
#									#
#########################################################################

add_definitions("-DPLUGIN_EXPORT=__attribute__((visibility(\"default\")))")

#########################################################################
#									#
#  Subdirectories							#
#									#
#########################################################################

add_subdirectory(ocr)
add_subdirectory(destination)
