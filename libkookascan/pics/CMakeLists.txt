##########################################################################
##									##
##  This CMake file is part of libkscan, a KDE scanning library.	##
##									##
##  This file may be distributed and/or modified under the terms of	##
##  the GNU General Public License version 2, as published by the	##
##  Free Software Foundation and appearing in the file COPYING		##
##  included in the packaging of this file.				##
##									##
##  Author:  Jonathan Marten <jjm AT keelhaul DOT me DOT uk>		##
##									##
##########################################################################


########### install files ###############

set(libkookascan_PICS
  palette-color.png
  palette-gray.png
  palette-halftone.png
  palette-lineart.png
)

install(FILES ${libkookascan_PICS} DESTINATION ${KDE_INSTALL_DATADIR}/libkookascan/pics)
