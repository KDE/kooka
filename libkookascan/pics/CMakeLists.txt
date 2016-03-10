
project(kookascan)

########### install files ###############

set(libkookascan_PICS
  palette-color.png
  palette-gray.png
  palette-halftone.png
  palette-lineart.png
)

install(FILES ${libkookascan_PICS} DESTINATION ${DATA_INSTALL_DIR}/libkookascan/pics)
