## Makefile.am for kooka

SUBDIRS = pics

bin_PROGRAMS = kooka
METASOURCES = AUTO

kooka_SOURCES =	main.cpp kooka.cpp kookaview.cpp kookapref.cpp 		\
		img_saver.cpp ksaneocr.cpp \
		kookaimage.cpp kookaimagemeta.cpp scanpackager.cpp 	\
		imgnamecombo.cpp imageselectline.cpp      \
		thumbview.cpp thumbviewitem.cpp                         \
		dwmenuaction.cpp kocrbase.cpp           \
		kocrgocr.cpp kocrkadmos.cpp kadmosocr.cpp ocrword.cpp   \
		ocrresedit.cpp kookaprint.cpp imgprintdialog.cpp	\
		kocrocrad.cpp
# 		pagesetup.cpp 

kooka_LDADD   = $(LIB_KFILE) -lkdeprint -lktexteditor $(LIBTIFF) $(top_builddir)/libkscan/libkscan.la $(KADMOS_LIB) $(LIB_KSPELL)
kooka_LDFLAGS = $(KDE_RPATH) $(all_libraries)

INCLUDES         = -I$(top_srcdir)/libkscan $(all_includes) $(LIBSANE_INCLUDES) $(KADMOS_INC)

noinst_HEADERS   = \
kookaview.h     scanpackager.h \
img_saver.h    ksaneocr.h      \
formathelp.h   kooka.h  \
kookaiface.h  thumbview.h thumbviewitem.h \
kookapref.h   resource.h \
imgnamecombo.h imageselectline.h kookaimage.h kookaimagemeta.h \
kocrbase.h kocrgocr.h kocrkadmos.h \
kadmosocr.h ocrword.h ocrresedit.h kookaprint.h imgprintdialog.h \
kocrocrad.h

# pagesetup.h

appdatadir       = $(kde_datadir)/kooka
appdata_DATA     = kookaui.rc

kde_conf_DATA = kookarc

xdg_apps_DATA = kooka.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kooka.pot

