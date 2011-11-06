
project(kooka4)

########### clobbering ###############

add_custom_target(clobber
  COMMENT "Clobber in ${CMAKE_CURRENT_BINARY_DIR}..."
  VERBATIM
  COMMAND sh -c "rm -r -f CMakeFiles *.cmake"
)


########### install files ###############

install(FILES scanservice.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES scantypes.dat DESTINATION ${DATA_INSTALL_DIR}/libkscan)
