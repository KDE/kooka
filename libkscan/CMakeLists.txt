
project(kooka4)

########### dependencies ###############

# libpaper (at least in Gentoo Linux) provides neither a
# Find<package>.cmake or a pkg-config file.
find_library(LIBPAPER_LIB NAMES "libpaper" "libpaper.so")
find_file(LIBPAPER_HDR "paper.h")
if (LIBPAPER_LIB AND LIBPAPER_HDR)
  message(STATUS "libpaper library: ${LIBPAPER_LIB}")
  message(STATUS "libpaper header: ${LIBPAPER_HDR}")
  set(HAVE_LIBPAPER 1)
else (LIBPAPER_LIB AND LIBPAPER_HDR)
  message(STATUS "libpaper library not found, using built-in list of paper sizes")
endif (LIBPAPER_LIB AND LIBPAPER_HDR)

########### subdirectories ###############

add_subdirectory(pics)
add_subdirectory(desktop)

########### setup ###############

add_definitions(-DMAKE_KSCAN_LIB)
add_definitions(-DKDE4)
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=29000)
if (HAVE_SANE)
  add_definitions(-DHAVE_SANE)
  add_definitions(-DSANE_VERSION=\"${SANE_VERSION}\")
  include_directories(${SANE_INCLUDES})
endif (HAVE_SANE)
if (HAVE_LIBPAPER)
  add_definitions(-DHAVE_LIBPAPER)
  get_filename_component(LIBPAPER_INCLUDES ${LIBPAPER_HDR} PATH)
  include_directories(${LIBPAPER_INCLUDES})
endif (HAVE_LIBPAPER)

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
if (HAVE_LIBPAPER)
  target_link_libraries(libkscan ${LIBPAPER_LIB})
endif (HAVE_LIBPAPER)

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

########### installation ###############

install(TARGETS libkscan ${INSTALL_TARGETS_DEFAULT_ARGS})
