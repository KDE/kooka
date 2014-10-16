
project(kooka4)

########### clobbering ###############

add_custom_target(clobber
  COMMENT "Clobber in ${CMAKE_CURRENT_BINARY_DIR}..."
  VERBATIM
  COMMAND sh -c "rm -r -f CMakeFiles *.cmake"
)


########### install files ###############

set(libkscan_PICS
  palette-color.png
  palette-gray.png
  palette-halftone.png
  palette-lineart.png
)

#ecm_install_icons(${ICON_INSTALL_DIR})
install(FILES ${libkscan_PICS} DESTINATION ${DATA_INSTALL_DIR}/libkscan/pics)
