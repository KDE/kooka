## Makefile.am for kooka

SUBDIRS = pics

bin_PROGRAMS = kooka
METASOURCES = AUTO

kooka_SOURCES =	main.cpp kooka.cpp kookaview.cpp kookapref.cpp 		\
		img_saver.cpp ksaneocr.cpp miscwidgets.cpp 		\
		kookaimage.cpp kookaimagemeta.cpp scanpackager.cpp 	\
		imgnamecombo.cpp        \
		thumbview.cpp thumbviewitem.cpp                         \
		dwmenuaction.cpp kocrbase.cpp           \
		kocrgocr.cpp kocrkadmos.cpp kadmosocr.cpp 
# 		pagesetup.cpp

kooka_LDADD   = $(LIB_KFILE) -lkdeprint -lktexteditor $(LIBTIFF) $(top_builddir)/libkscan/libkscan.la $(KADMOS_LIB)
kooka_LDFLAGS = $(KDE_RPATH) $(all_libraries)

INCLUDES         = -I$(top_srcdir)/libkscan $(all_includes) $(LIBSANE_INCLUDES) $(KADMOS_INC)

noinst_HEADERS   = \
kookaview.h     scanpackager.h \
img_saver.h    ksaneocr.h       miscwidgets.h \
formathelp.h   kooka.h  \
kookaiface.h  thumbview.h thumbviewitem.h \
kookapref.h   resource.h \
imgnamecombo.h kookaimage.h kookaimagemeta.h \
kocrbase.h kocrgocr.h kocrkadmos.h \
kadmosocr.h 
# pagesetup.h

KDE_ICON = kooka

datadir       = $(kde_datadir)/kooka
data_DATA     = kookaui.rc

kde_conf_DATA = kookarc

kdelnkdir   = $(kde_appsdir)/Graphics
kdelnk_DATA = kooka.desktop



messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kooka.pot

