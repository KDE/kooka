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

cmake_minimum_required(VERSION 2.8.12)

project(kooka5)
set(VERSION "0.90")
message(STATUS "Configuring for Kooka/libkookascan version ${VERSION}")

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.4.0")
set(KF5_MIN_VERSION "5.10.0")
set(ECM_MIN_VERSION "1.2.0")

# ECM setup (Extra Cmake Modules)
find_package(ECM ${ECM_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(FeatureSummary)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMPackageConfigHelpers)
include(CheckFunctionExists)
include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(GenerateExportHeader)
include(ECMInstallIcons)

# Required Qt5 components to build this package
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets)

# REMOVE IT
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

# Support for SANE, here because library and sanedump both need it
if (SANECONFIG_BIN)
  set(SANECONFIG_PROG ${SANECONFIG_BIN})
  message(STATUS "Specified sane-config(1), ${SANECONFIG_PROG}")
else (SANECONFIG_BIN)
  find_program(SANECONFIG_PROG NAMES sane-config)
  message(STATUS "Found sane-config(1), ${SANECONFIG_PROG}")
endif (SANECONFIG_BIN)

if (SANECONFIG_PROG)
  set(HAVE_SANE true)
else (SANECONFIG_PROG)
  message(SEND_ERROR "libkookascan needs SANE (http://www.sane-project.org) - specify location of sane-config(1) with SANECONFIG_BIN")
endif (SANECONFIG_PROG)

if (HAVE_SANE)
  execute_process(COMMAND ${SANECONFIG_PROG} --version OUTPUT_VARIABLE SANE_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "Found SANE, version ${SANE_VERSION}")

  execute_process(COMMAND ${SANECONFIG_PROG} --cflags OUTPUT_VARIABLE SANE_INCLUDES OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "SANE includes: ${SANE_INCLUDES}")

  execute_process(COMMAND ${SANECONFIG_PROG} --libs OUTPUT_VARIABLE SANE_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "SANE libraries: ${SANE_LIBRARIES}")
endif (HAVE_SANE)

############### Now, we add the Kooka components ###############

add_subdirectory(libdialogutil)
add_subdirectory(libkookascan)
add_subdirectory(libfiletree)
add_subdirectory(kooka)
add_subdirectory(doc)

add_subdirectory(tools EXCLUDE_FROM_ALL)

############### VCS revision number in vcsversion.h ###############

add_custom_target(vcsversion
  ALL
  COMMENT "Checking VCS version"
  VERBATIM
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/vcsversion.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${VERSION}
)

# ########### documentation ###############
#
# if (HAVE_APIDOX)
#   add_custom_target(apidox
#     COMMENT "Generating API documentation in ${CMAKE_CURRENT_BINARY_DIR}..."
#     VERBATIM
#     COMMAND sh -c "${KDELIBS_SOURCE_DIR}/doc/api/doxygen.sh --no-modulename --recurse --doxdatadir=${KDELIBS_SOURCE_DIR}/doc/common ${CMAKE_CURRENT_SOURCE_DIR}; echo 'API documentation at file://${CMAKE_CURRENT_BINARY_DIR}/apidocs/index.html';")
# endif (HAVE_APIDOX)

############### Configuration information ###############

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
