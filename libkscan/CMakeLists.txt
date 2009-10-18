
project(kooka4)




# Support for SANE
option(SANECONFIG_BIN "Path to the sane-config(1) binary, needed to get SANE configuration" OFF)


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




add_subdirectory(pics)
add_subdirectory(desktop)



add_definitions(-DMAKE_KSCAN_LIB)
add_definitions(-DKDE4)
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=29000)
if (HAVE_SANE)
  add_definitions(-DHAVE_SANE)
  add_definitions(-DSANE_VERSION=\"${SANE_VERSION}\")
endif (HAVE_SANE)


include_directories(..)


if (HAVE_SANE)
  include_directories(${SANE_INCLUDES})
endif (HAVE_SANE)


########### library ###############

# TODO: rename some of these to match class name, others are obsolete
set(libkscan_SRCS
  kscandevice.cpp
  kscancontrols.cpp
  kgammatable.cpp
  kscanoption.cpp
  kscanoptset.cpp
  gammadialog.cpp
  dispgamma.cpp
  scansourcedialog.cpp
  scanparams.cpp
  massscandialog.cpp
  devselector.cpp		# -> deviceselector
  adddevice.cpp			# -> adddevicedialog
  #scandialog.cpp		# not used by kooka
  img_canvas.cpp		# -> imagecanvas
  previewer.cpp
  imgscaledialog.cpp
  sizeindicator.cpp
  imgscaninfo.cpp
  scansizeselector.cpp
)



########### clobbering ###############

add_custom_target(clobber
  COMMENT "Clobber in ${CMAKE_CURRENT_BINARY_DIR}..."
  VERBATIM
  COMMAND sh -c "rm -f *.moc"
  COMMAND sh -c "rm -f *_automoc.cpp *_automoc.cpp.files"
  COMMAND sh -c "rm -r -f CMakeFiles *.cmake"
  COMMAND sh -c "rm -f config-*.h"
)


########### helper executable ###############

# TODO: reinstate this
#libexec_PROGRAMS = sanedump
#sanedump_SOURCES =	sanedump.c
#sanedump_LDADD =	$(LIBSANE_LIBS)



kde4_add_library(libkscan SHARED ${libkscan_SRCS})
target_link_libraries(libkscan
  ${QT_QT3SUPPORT_LIBRARY}
  ${KDE4_KIO_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KDECORE_LIBS}
)
if (HAVE_SANE)
  target_link_libraries(libkscan ${SANE_LIBRARIES})
endif (HAVE_SANE)
# libkscan in kdegraphics 3.5 was version 1, SVN work/kooka-kde3 was version 2
set_target_properties(libkscan PROPERTIES VERSION 3.0.0 SOVERSION 3)





########### install files ###############

install(TARGETS libkscan ${INSTALL_TARGETS_DEFAULT_ARGS})

