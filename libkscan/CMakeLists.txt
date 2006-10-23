
add_subdirectory( pics ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkscan ${SANE_INCLUDE_DIR}  )


########### next target ###############

set(kscan_LIB_SRCS 
   kscandevice.cpp 
   kscanslider.cpp 
   kgammatable.cpp 
   kscanoption.cpp 
   kscanoptset.cpp 
   gammadialog.cpp 
   dispgamma.cpp 
   scansourcedialog.cpp 
   scanparams.cpp 
   massscandialog.cpp 
   devselector.cpp 
   scandialog.cpp 
   img_canvas.cpp 
   previewer.cpp 
   imgscaledialog.cpp 
   sizeindicator.cpp 
   imgscaninfo.cpp )

kde4_automoc(${kscan_LIB_SRCS})

kde4_add_library(kscan SHARED ${kscan_LIB_SRCS})

target_link_libraries(kscan  ${KDE4_KIO_LIBS} ${SANE_LIBRARIES})

set_target_properties(kscan PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS kscan  DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############

install( FILES scanservice.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/libkscan $(all_includes) $(LIBSANE_INCLUDES)
#
#lib_LTLIBRARIES = libkscan.la
#
#noinst_HEADERS = kscandevice.h kscanslider.h kgammatable.h \
#	kscanoption.h kscanoptset.h \
#	gammadialog.h dispgamma.h \
#	scansourcedialog.h scanparams.h massscandialog.h devselector.h \
#        img_canvas.h previewer.h imgscaledialog.h sizeindicator.h \
#	imgscaninfo.h
#
#libkscan_la_SOURCES = kscandevice.cpp kscanslider.cpp kgammatable.cpp\
#        kscanoption.cpp kscanoptset.cpp \
#	gammadialog.cpp dispgamma.cpp \
#	scansourcedialog.cpp scanparams.cpp massscandialog.cpp \
#        devselector.cpp scandialog.cpp \
#	img_canvas.cpp previewer.cpp imgscaledialog.cpp sizeindicator.cpp \
#	imgscaninfo.cpp
#
#
#libkscan_la_LDFLAGS = $(all_libraries) $(LIBSANE_LDFLAGS) -no-undefined -version-info 1:0
#libkscan_la_LIBADD = $(LIBSANE_LIBS) $(LIB_KFILE)
#
#kde_services_DATA = scanservice.desktop
#
#METASOURCES = AUTO
#
#SUBDIRS = pics
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/libkscan.pot
#
