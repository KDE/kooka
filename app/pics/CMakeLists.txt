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


########### install files ###############

set(kooka_PICS
  lockzoom.png
  newfromselect.png
  ocr.png
  ocr-select.png
  ocr-file.png
  photocopy.png
  preview.png
  scan.png
  scanadd.png
  scanselect.png
  thumbviewtile.png
  rotate-acw.png
  rotate-cw.png
  rotate-180.png
  autoselect.png
)

install(FILES ${kooka_PICS} DESTINATION ${PICS_INSTALL_DIR})
