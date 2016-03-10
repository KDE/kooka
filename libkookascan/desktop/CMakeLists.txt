
project(kookascan)

########### install files ###############

install(FILES scanservice.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES scantypes.dat DESTINATION ${DATA_INSTALL_DIR}/libkookascan)
