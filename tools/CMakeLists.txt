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

########### dependencies ###############

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n CoreAddons)

########### sanedump ###############

set(sanedump_SRCS
  sanedump.c
)

if (HAVE_SANE)
  include_directories(${SANE_INCLUDES})
endif (HAVE_SANE)

add_executable(sanedump ${sanedump_SRCS})
if (HAVE_SANE)
  target_link_libraries(sanedump ${SANE_LIBRARIES})
endif (HAVE_SANE)

########### qimageioinfo ###############

set(qimageioinfo_SRCS
  qimageioinfo.cpp
)

add_executable(qimageioinfo ${qimageioinfo_SRCS})
target_link_libraries(qimageioinfo Qt::Core Qt::Gui)
target_link_libraries(qimageioinfo KF5::CoreAddons)

########### install files ###############

if (INSTALL_BINARIES)
  install(TARGETS sanedump ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
  install(TARGETS qimageioinfo ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
endif (INSTALL_BINARIES)
