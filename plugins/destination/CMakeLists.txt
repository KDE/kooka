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

#########################################################################
#									#
#  Additional dependencies for scan destinations			#
#									#
#########################################################################

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n CoreAddons WidgetsAddons)

find_package(KF5Purpose ${KF5_MIN_VERSION})
set_package_properties(KF5Purpose PROPERTIES
  URL "https://invent.kde.org/frameworks/purpose"
  DESCRIPTION "A framework for services and actions integration"
  PURPOSE "Required for the Share Scan destination plugin"
  TYPE OPTIONAL
)

#########################################################################
#									#
#  Destination plugin base library					#
#									#
#########################################################################

ecm_qt_declare_logging_category(kookadestination_LOG_SRCS
  HEADER "destination_logging.h"
  IDENTIFIER "DESTINATION_LOG"
  CATEGORY_NAME "destination"
  EXPORT kookalogging
  DESCRIPTION "destination plugins (Kooka)")

set(kookadestination_SRCS
  abstractdestination.cpp
  ${kookadestination_LOG_SRCS}
)

add_definitions(-DLIBEXEC_DIR=\"${CMAKE_INSTALL_FULL_LIBEXECDIR_KF5}\")
add_library(kookadestination SHARED ${kookadestination_SRCS})
set_target_properties(kookadestination PROPERTIES SOVERSION "1.0.0")
target_link_libraries(kookadestination Qt::Core Qt::Widgets)
target_link_libraries(kookadestination KF5::I18n KF5::WidgetsAddons)
target_link_libraries(kookadestination kookascan kookacore)

if (INSTALL_BINARIES)
  install(TARGETS kookadestination ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
endif (INSTALL_BINARIES)

#########################################################################
#									#
#  Subdirectories							#
#									#
#########################################################################

add_subdirectory(gallery)
add_subdirectory(application)
add_subdirectory(clipboard)
add_subdirectory(print)
add_subdirectory(save)

if (KF5Purpose_FOUND)
  add_subdirectory(share)
endif (KF5Purpose_FOUND)
