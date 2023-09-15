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

ecm_install_icons(ICONS 48-apps-kooka.png DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
install(FILES org.kde.kooka.desktop DESTINATION ${KDE_INSTALL_APPDIR})
