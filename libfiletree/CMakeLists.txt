##########################################################################
##									##
##  This CMake file is part of libfiletree. originally part of KDE3.	##
##									##
##  This file may be distributed and/or modified under the terms of	##
##  the GNU General Public License version 2, as published by the	##
##  Free Software Foundation and appearing in the file COPYING		##
##  included in the packaging of this file.				##
##									##
##  Author:  Jonathan Marten <jjm AT keelhaul DOT me DOT uk>		##
##									##
##########################################################################

project(libfiletree)

########### dependencies ###############

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n KIO)

########### libfiletree ###############

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=250)

set(libfiletree_SRCS
  filetreeview.cpp
  filetreebranch.cpp
  filetreeviewitem.cpp
)

add_library(libfiletree STATIC ${libfiletree_SRCS})
target_link_libraries(libfiletree
  Qt5::Core Qt5::Widgets
  KF5::KIOCore KF5::KIOWidgets
)
