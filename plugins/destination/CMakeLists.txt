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
#  Additional dependencies for scan destinations			#
#									#
#########################################################################

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS I18n CoreAddons WidgetsAddons)

find_package(KF6Purpose ${KF_MIN_VERSION})
set_package_properties(KF6Purpose PROPERTIES
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

add_definitions(-DLIBEXEC_DIR=\"${CMAKE_INSTALL_FULL_LIBEXECDIR}\")
add_library(kookadestination SHARED ${kookadestination_SRCS})
set_target_properties(kookadestination PROPERTIES VERSION ${VERSION} SOVERSION 6)
target_link_libraries(kookadestination Qt6::Core Qt6::Widgets)
target_link_libraries(kookadestination KF6::I18n KF6::WidgetsAddons)
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
add_subdirectory(multipage)

if (KF6Purpose_FOUND)
  add_subdirectory(share)
endif (KF6Purpose_FOUND)

if (BUILD_TOOLS)
  add_subdirectory(null)
endif (BUILD_TOOLS)
