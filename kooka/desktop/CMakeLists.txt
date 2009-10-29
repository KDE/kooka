
project(kooka4)


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

kde4_install_icons(${ICON_INSTALL_DIR})
install(FILES kooka.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
