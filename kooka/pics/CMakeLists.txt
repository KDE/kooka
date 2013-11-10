
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
  scan.png
  scanadd.png
  scanselect.png
  scaleorig.png
  scaletoheight.png
  scaletowidth.png
  thumbviewtile.png
  rotate-acw.png
  rotate-cw.png
  rotate-180.png
  autoselect.png
)

install(FILES ${kooka_PICS} DESTINATION ${DATA_INSTALL_DIR}/kooka/pics)
