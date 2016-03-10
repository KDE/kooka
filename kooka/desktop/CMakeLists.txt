
project(kooka5)

########### install files ###############

ecm_install_icons(ICONS hi48-apps-kooka.png DESTINATION ${ICON_INSTALL_DIR})
install(FILES org.kde.kooka.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES kookaui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kooka)
