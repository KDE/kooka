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
#  OCR plugin for OCRAD							#
#									#
#########################################################################

set(ocrocrad_SRCS
  ocrocradengine.cpp
  ocrocraddialog.cpp
  ${kookaocr_LOG_SRCS}
)

kcoreaddons_add_plugin(ocrocrad SOURCES ${ocrocrad_SRCS} INSTALL_NAMESPACE "kooka_ocr")

target_link_libraries(ocrocrad Qt${Q}::Core Qt${Q}::Gui)
target_link_libraries(ocrocrad KF${Q}::I18n KF${Q}::WidgetsAddons)
target_link_libraries(ocrocrad kookaocr kookascan)

install(FILES ocrad.png DESTINATION ${PICS_INSTALL_DIR})
