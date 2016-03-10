
project(kooka5)

########### sanedump ###############

set(sanedump_SRCS
  sanedump.c
)

if (HAVE_SANE)
  include_directories(${SANE_INCLUDES})
endif (HAVE_SANE)

kde4_add_executable(sanedump ${sanedump_SRCS})
if (HAVE_SANE)
  target_link_libraries(sanedump ${SANE_LIBRARIES})
endif (HAVE_SANE)

########### kimageioinfo ###############

set(kimageioinfo_SRCS
  kimageioinfo.cpp
)

add_executable(kimageioinfo ${kimageioinfo_SRCS})
target_link_libraries(kimageioinfo 
  Qt5::Core
  KF5::KDELibs4Support
)

########### install files ###############

install(TARGETS sanedump ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kimageioinfo ${INSTALL_TARGETS_DEFAULT_ARGS})
