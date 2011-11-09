
project(kooka4)

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
  scanglobal.cpp
  kscandevice.cpp
  scandevices.cpp
  kscancontrols.cpp
  kgammatable.cpp
  kscanoption.cpp
  kscanoptset.cpp
  gammadialog.cpp
  gammawidget.cpp
  scansourcedialog.cpp
  scanparams.cpp
  massscandialog.cpp
  deviceselector.cpp
  adddevicedialog.cpp
  imagecanvas.cpp
  previewer.cpp
  imgscaledialog.cpp
  sizeindicator.cpp
  imagemetainfo.cpp
  scansizeselector.cpp
)

kde4_add_library(libkscan SHARED ${libkscan_SRCS})
target_link_libraries(libkscan
  ${KDE4_KIO_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KDECORE_LIBS}
)
if (HAVE_SANE)
  target_link_libraries(libkscan ${SANE_LIBRARIES})
endif (HAVE_SANE)

# libkscan in kdegraphics 3.5 was version 1, SVN work/kooka-kde3 was version 2
set_target_properties(libkscan PROPERTIES VERSION 3.0.0 SOVERSION 3)

########### clobbering ###############

add_custom_target(clobber
  COMMENT "Clobber in ${CMAKE_CURRENT_BINARY_DIR}..."
  VERBATIM
  COMMAND sh -c "rm -f *.moc"
  COMMAND sh -c "rm -f *_automoc.cpp *_automoc.cpp.files"
  COMMAND sh -c "rm -r -f CMakeFiles *.cmake"
  COMMAND sh -c "rm -f config-*.h"
)

########### install files ###############

install(TARGETS libkscan ${INSTALL_TARGETS_DEFAULT_ARGS})
