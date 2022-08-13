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

set(libfiletree_SRCS
  filetreeview.cpp
  filetreebranch.cpp
  filetreeviewitem.cpp
)

ecm_qt_declare_logging_category(libfiletree_SRCS
  HEADER "libfiletree_logging.h"
  IDENTIFIER "LIBFILETREE_LOG"
  CATEGORY_NAME "libfiletree"
  EXPORT kookalogging
  DESCRIPTION "libfiletree (Kooka)")

add_library(libfiletree STATIC ${libfiletree_SRCS})
set_target_properties(libfiletree PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(libfiletree
  Qt::Core Qt::Widgets
  KF5::KIOCore
)
