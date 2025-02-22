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

cmake_minimum_required(VERSION 3.16)

# KDE CI sets this for Qt6 runners but not for Qt5.
if (NOT DEFINED QT_MAJOR_VERSION)
  set (QT_MAJOR_VERSION 5)
endif ()
set(Q ${QT_MAJOR_VERSION})

project(kooka)
set(VERSION "${Q}.90.0")
message(STATUS "Configuring for Kooka/libkookascan version ${VERSION}")

if (Q EQUAL 6)
  set(QT_MIN_VERSION "6.7.0")
  set(KF_MIN_VERSION "6.1.0")
else ()
  if (Q EQUAL 5)
    set(QT_MIN_VERSION "5.15.0")
    set(KF_MIN_VERSION "5.104.0")
  else ()
    message(FATAL_ERROR "Expecting to build for either Qt5 or Qt6")
  endif ()
endif ()

# ECM setup (Extra Cmake Modules)
find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(FeatureSummary)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(CheckFunctionExists)
include(GenerateExportHeader)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(KDEClangFormat)

# Options
option(INSTALL_BINARIES "Install the binaries and libraries, turn off for development in place" ON)
option(BUILD_TOOLS "Build the testing tools and null destination plugin" OFF)

# Required Qt components to build this package
find_package(Qt${Q} ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets)

# Rigourousness
add_definitions("-DQT_USE_QSTRINGBUILDER")
add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY")
add_definitions("-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")
add_definitions("-DQT_NO_CAST_TO_ASCII")
add_definitions("-DQT_NO_URL_CAST_FROM_STRING")

# Permissiveness
remove_definitions("-DQT_NO_CAST_FROM_ASCII")
remove_definitions("-DQT_NO_KEYWORDS")

# Support for SANE, here because library and sanedump both need it
#
# Prefer pkg-config(1), because sane-config(1) was removed from Debian
# package sane-backends 1.0.25 in December 2016.  Assuming here that
# pkg-config(1) is available on any reasonable system.
find_package(PkgConfig)
if (PkgConfig_FOUND)
  pkg_check_modules(SANE sane-backends)
endif (PkgConfig_FOUND)

if (SANE_FOUND)
  set(SANE_CFLAGS "${SANE_CFLAGS}")
  string(REPLACE "-I" "" SANE_INCLUDES ${SANE_CFLAGS})
  set(SANE_LIBRARIES "${SANE_LDFLAGS}")
else (SANE_FOUND)
  # if pkg-config(1) did not find anything, then fall back to sane-config(1)
  message(STATUS "SANE not found via pkg-config(1), trying sane-config(1)")
  if (SANECONFIG_BIN)
    set(SANECONFIG_PROG ${SANECONFIG_BIN})
    message(STATUS "Specified sane-config(1), ${SANECONFIG_PROG}")
  else (SANECONFIG_BIN)
    find_program(SANECONFIG_PROG NAMES sane-config)
    message(STATUS "Found sane-config(1), ${SANECONFIG_PROG}")
  endif (SANECONFIG_BIN)

  if (SANECONFIG_PROG)
    set(SANE_FOUND true)
    execute_process(COMMAND ${SANECONFIG_PROG} --version OUTPUT_VARIABLE SANE_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${SANECONFIG_PROG} --cflags OUTPUT_VARIABLE SANE_INCLUDES OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${SANECONFIG_PROG} --libs OUTPUT_VARIABLE SANE_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Found SANE, version ${SANE_VERSION}")
endif (SANECONFIG_PROG)

endif (SANE_FOUND)

if (SANE_FOUND)
  set(HAVE_SANE true)
  message(STATUS "  SANE includes: ${SANE_INCLUDES}")
  message(STATUS "  SANE libraries: ${SANE_LIBRARIES}")
else (SANE_FOUND)
  message(SEND_ERROR "libkookascan needs SANE (http://www.sane-project.org) - install package or specify location of sane-config(1) with SANECONFIG_BIN")
endif (SANE_FOUND)

############### Common build flags ###############

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")

   # both gcc & clang support it since the same version
   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.0.0)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wzero-as-null-pointer-constant" )
   endif()
endif()

############### Common install locations ###############

set(PICS_INSTALL_DIR ${KDE_INSTALL_DATADIR}/kooka/pics)

############### Common definitions ###############

add_definitions(-DKF5)

############### Now, we add the Kooka components ###############

add_subdirectory(libdialogutil)
add_subdirectory(libkookascan)
add_subdirectory(libfiletree)
add_subdirectory(app)
add_subdirectory(plugins)
add_subdirectory(doc)

if (BUILD_TOOLS)
  add_subdirectory(tools)
endif (BUILD_TOOLS)

############### VCS revision number in vcsversion.h ###############

add_custom_target(vcsversion
  ALL
  COMMENT "Checking VCS version"
  VERBATIM
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/vcsversion.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${VERSION}
)

########### Code formatting ###############

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

# ########### documentation ###############
#
# if (HAVE_APIDOX)
#   add_custom_target(apidox
#     COMMENT "Generating API documentation in ${CMAKE_CURRENT_BINARY_DIR}..."
#     VERBATIM
#     COMMAND sh -c "${KDELIBS_SOURCE_DIR}/doc/api/doxygen.sh --no-modulename --recurse --doxdatadir=${KDELIBS_SOURCE_DIR}/doc/common ${CMAKE_CURRENT_SOURCE_DIR}; echo 'API documentation at file://${CMAKE_CURRENT_BINARY_DIR}/apidocs/index.html';")
# endif (HAVE_APIDOX)

############### Logging ###############

ecm_qt_install_logging_categories(
  EXPORT kookalogging
  FILE kooka.categories
  DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}")

############### Configuration information ###############

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
