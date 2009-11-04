
project(kooka4)
set(VERSION "0.60")
message(STATUS "Configuring for Kooka/libkscan version ${VERSION}")

# where to look first for cmake modules. This line must be the first one or cmake will use the system's FindFoo.cmake
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${KDE4_DATA_DIR}/cmake/modules)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /ws/trunk/kdepim/cmake/modules)


############### Build Options ###############

option(SANECONFIG_BIN "Path to the sane-config(1) binary, needed to get SANE configuration" OFF)

############### search packages used by KDE ###############

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(CheckIncludeFiles)
include(MacroBoolTo01)

############### Windows specific ###############

if(WIN32)
  # detect oxygen icon dir at configure time based on KDEDIRS - there may be different package installation locations
  execute_process(COMMAND "${KDE4_KDECONFIG_EXECUTABLE}" --path icon OUTPUT_VARIABLE _dir ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  file(TO_CMAKE_PATH "${_dir}" __dir)
  find_path(KDE4_ICON_DIR oxygen PATHS
    ${__dir}
  )
  message(STATUS "Using Oxygen application icons from ${KDE4_ICON_DIR}")
else(WIN32)
    set (KDE4_ICON_DIR  ${CMAKE_INSTALL_PREFIX}/share/icons)
endif(WIN32)


############### Find the stuff we need ###############

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
  message(SEND_ERROR "libkscan needs SANE (http://www.sane-project.org) - specify location of sane-config(1) with SANECONFIG_BIN")
endif (SANECONFIG_PROG)

if (HAVE_SANE)
  execute_process(COMMAND ${SANECONFIG_PROG} --version OUTPUT_VARIABLE SANE_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "Found SANE, version ${SANE_VERSION}")

  execute_process(COMMAND ${SANECONFIG_PROG} --libs OUTPUT_VARIABLE SANE_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "SANE libraries: ${SANE_LIBRARIES}")

  execute_process(COMMAND ${SANECONFIG_PROG} --cflags OUTPUT_VARIABLE SANE_INCLUDES OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "SANE includes: ${SANE_INCLUDES}")
endif (HAVE_SANE)

############### Needed commands before building anything ###############

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${QT_QTDBUS_INCLUDE_DIR})

############### Macros ###############




############### Support for the SVN revision number in svnversion.h ###############

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
  if (NOT KdeSubversion_FOUND)
    find_package(KdeSubversion)
  endif (NOT KdeSubversion_FOUND)
  if (KdeSubversion_FOUND)
    KdeSubversion_WC_INFO(${PROJECT_SOURCE_DIR} k)
    string(SUBSTRING "${k_WC_LAST_CHANGED_DATE}" 0 10 k_WC_LAST_CHANGED_DATE)
    message(STATUS "Current SVN revision:           ${k_WC_REVISION}")
    message(STATUS "Current SVN last changed date:  ${k_WC_LAST_CHANGED_DATE}")
    set(svn_revision "${k_WC_REVISION}")
    set(svn_last_change "${k_WC_LAST_CHANGED_DATE}")
    set(svn_have_version 1)
  else (KdeSubversion_FOUND)
    set(svn_revision "Unknown")
    set(svn_last_change "")
    set(svn_have_version 1)
  endif (KdeSubversion_FOUND)
else (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
    set(svn_revision "")
    set(svn_last_change "")
    set(svn_have_version 0)
endif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")

configure_file(svnversion.h.in ${CMAKE_CURRENT_BINARY_DIR}/svnversion.h)

########### clobbering ###############

add_custom_target(clobber
  COMMENT "Clobber in ${CMAKE_CURRENT_BINARY_DIR}..."
  VERBATIM
  COMMAND sh -c "rm -r -f CMakeTmp *.cmake"
  COMMAND sh -c "rm -f svnversion.h"
  COMMAND sh -c "rm -r -f bin lib"
  COMMAND sh -c "rm -r -f CMakeFiles"
)

############### Now, we add the Kooka components ###############

add_subdirectory(libkscan)
add_subdirectory(kooka)
add_subdirectory(tools EXCLUDE_FROM_ALL)




macro_display_feature_log()
