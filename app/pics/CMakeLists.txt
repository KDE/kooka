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

########### install files ###############

set(kooka_PICS
  lockzoom.png
  mirror-both.png
  mirror-horiz.png
  mirror-vert.png
  newfromselect.png
  ocr.png
  ocr-select.png
  gocr.png
  ocrad.png
  tesseract.png
  photocopy.png
  preview.png
  scan.png
  scanadd.png
  scanselect.png
  scaleorig.png
  scaletoheight.png
  scaletowidth.png
  thumbviewtile.png
  rotate-acw.png
  rotate-cw.png
  rotate-180.png
  autoselect.png
)

install(FILES ${kooka_PICS} DESTINATION ${DATA_INSTALL_DIR}/kooka/pics)
