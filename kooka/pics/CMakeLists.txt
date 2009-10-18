
project(kooka4)

########### clobbering ###############

add_custom_target(clobber
  COMMENT "Clobber in ${CMAKE_CURRENT_BINARY_DIR}..."
  VERBATIM
  COMMAND sh -c "rm -r -f CMakeFiles *.cmake"
)


########### install files ###############

set(kooka_PICS
  lockzoom.png
  mirror-both.png
  mirror-horiz.png
  mirror-vert.png
  newfromselect.png
  ocr.png
  ocr-select.png
  gocr.png
  ocrad.png
  photocopy.png
  preview.png
  scaleorig.png
  scaletoheight.png
  scaletowidth.png
  thumbviewtile.png
)

kde4_install_icons(${ICON_INSTALL_DIR})
install(FILES ${kooka_PICS} DESTINATION ${DATA_INSTALL_DIR}/kooka/pics)
