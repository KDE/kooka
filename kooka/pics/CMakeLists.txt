kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kooka/pics FILES mirror-both.png mirror-horiz.png mirror-vert.png scaletoheight.png  	scaletowidth.png scaleorig.png ocr.png ocr-select.png newfromselect.png  	thumbviewtile.png gocr.png ocrad.png lockzoom.png )

kde4_footer()



#original Makefile.am contents follow:

## Add all of your pixmaps here
#pics_DATA = mirror-both.png mirror-horiz.png mirror-vert.png scaletoheight.png \
#	scaletowidth.png scaleorig.png ocr.png ocr-select.png newfromselect.png \
#	thumbviewtile.png gocr.png ocrad.png lockzoom.png
#
#
## This is where it will all be installed
#picsdir = $(kde_datadir)/kooka/pics
