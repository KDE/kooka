
project(kooka5)

########### sanedump ###############

set(sanedump_SRCS
  sanedump.c
)

if (HAVE_SANE)
  include_directories(${SANE_INCLUDES})
endif (HAVE_SANE)

add_executable(sanedump ${sanedump_SRCS})
if (HAVE_SANE)
  target_link_libraries(sanedump ${SANE_LIBRARIES})
endif (HAVE_SANE)

########### kimageioinfo is KDE4 only ###############


########### install files ###############

install(TARGETS sanedump ${INSTALL_TARGETS_DEFAULT_ARGS})
