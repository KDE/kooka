
########### clobbering ###############

#add_custom_target(clobber
#  COMMENT "Clobber in ${CMAKE_CURRENT_BINARY_DIR}..."
#  VERBATIM
#  COMMAND sh -c "rm -f *.moc"
#  COMMAND sh -c "rm -f *_automoc.cpp *_automoc.cpp.files"
#  COMMAND sh -c "rm -r -f CMakeFiles *.cmake"
#  COMMAND sh -c "rm -f config-*.h"
#)

########### install files ###############

ecm_install_icons(ICONS hi48-apps-kooka.png DESTINATION ${ICON_INSTALL_DIR})
install(FILES org.kde.kooka.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES kookaui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kooka)

