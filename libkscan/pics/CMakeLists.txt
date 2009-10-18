
project(kooka4)

########### clobbering ###############

add_custom_target(clobber
  COMMENT "Clobber in ${CMAKE_CURRENT_BINARY_DIR}..."
  VERBATIM
  COMMAND sh -c "rm -r -f CMakeFiles *.cmake"
)


########### install files ###############

kde4_install_icons(${ICON_INSTALL_DIR})
