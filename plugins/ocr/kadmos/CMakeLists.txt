#########################################################################
#  Project:		Kooka						#
#  Edit:		14-Dec-18					#
#########################################################################

project(kooka5)

#########################################################################
#									#
#  Options
#									#
#########################################################################

option(KADMOS_DIR "Path to Kadmos include and library files, default '/usr/local'" "/usr/local")

#########################################################################
#									#
#  Locating required Kadmos support					#
#									#
#########################################################################

if (WITH_KADMOS)
  find_file(KADMOS_HEADER_FOUND kadmos.h PATHS ${KADMOS_DIR})
  if (KADMOS_HEADER_FOUND)
    string(REGEX REPLACE "/.*$" "" KADMOS_INCLUDE_DIR KADMOS_HEADER_FOUND)
    message(STATUS "Kadmos includes: ${KADMOS_INCLUDE_DIR}")
  else (KADMOS_HEADER_FOUND)
    message(SEND_ERROR "WITH_KADMOS requested, but Kadmos engine header file 'kadmos.h' not found in ${KADMOS_DIR} - set KADMOS_DIR to locate it")
  endif (KADMOS_HEADER_FOUND)

  find_file(KADMOS_LIB_FOUND librep.a PATHS ${KADMOS_DIR})
  if (KADMOS_LIB_FOUND)
    string(REGEX REPLACE "/.*$" "" KADMOS_LIB_DIR KADMOS_LIB_FOUND)
    message(STATUS "Kadmos libraries: ${KADMOS_LIB_DIR}")
  else (KADMOS_LIB_FOUND)
    message(SEND_ERROR "WITH_KADMOS requested, but Kadmos engine library 'librep.a' not found in ${KADMOS_DIR} - set KADMOS_DIR to locate it")
  endif (KADMOS_LIB_FOUND)

  set(HAVE_KADMOS true)
endif (WITH_KADMOS)

#########################################################################
#									#
#  Definitions								#
#									#
#########################################################################

if (HAVE_KADMOS)
  add_definitions(-DHAVE_KADMOS)

  if (KADMOS_INCLUDE_DIR)
    include_directories(${KADMOS_INCLUDE_DIR})
  endif (KADMOS_INCLUDE_DIR)
  if (KADMOS_LIB_DIR)
    link_directories(${KADMOS_LIB_DIR})
  endif (KADMOS_LIB_DIR)
endif (HAVE_KADMOS)

#########################################################################
#									#
#  OCR plugin for Kadmos						#
#									#
#########################################################################

set(kookaocrkadmos_SRCS
  ocrkadmosengine.cpp
  ocrkadmosdialog.cpp
  kadmosocr.cpp
)

add_library(kookaocrkadmos MODULE ${kookaocrkadmos_SRCS})
kcoreaddons_desktop_to_json(kookaocrkadmos kookaocr-kadmos.desktop)

target_link_libraries(kookaocrkadmos Qt5::Core Qt5::Gui)
target_link_libraries(kookaocrkadmos KF5::I18n KF5::WidgetsAddons)
target_link_libraries(kookaocrkadmos kookaocr kookascan)
target_link_libraries(kookaocrkadmos librep)

if (INSTALL_BINARIES)
  install(TARGETS kookaocrkadmos DESTINATION ${PLUGIN_INSTALL_DIR}/kooka)
endif (INSTALL_BINARIES)
install(FILES kookaocr-kadmos.desktop DESTINATION ${SERVICES_INSTALL_DIR})
