
project(kookascan)

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

########### i18n ###############

add_definitions(-DTRANSLATION_DOMAIN="libkookascan")

########### library ###############

# TODO: rename some of these to match class name, others are obsolete
set(kookascan_SRCS
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
  autoselectbar.cpp
  autoselectdialog.cpp
  scanicons.cpp
)
kconfig_add_kcfg_files(kookascan_SRCS scansettings.kcfgc)

add_library(kookascan SHARED ${kookascan_SRCS})
generate_export_header(kookascan BASE_NAME kookascan)
target_link_libraries(kookascan
  dialogutil
  KF5::KIOCore
  KF5::KDELibs4Support
)

if (HAVE_SANE)
  target_link_libraries(kookascan ${SANE_LIBRARIES})
endif (HAVE_SANE)
if (HAVE_LIBPAPER)
  target_link_libraries(kookascan ${LIBPAPER_LIB})
endif (HAVE_LIBPAPER)

set_target_properties(kookascan PROPERTIES VERSION 1.0.0 SOVERSION 1)

########### installation ###############

install(TARGETS kookascan ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES scansettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
