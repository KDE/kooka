## Makefile.am for kooka

SUBDIRS = pics doc

bin_PROGRAMS = kooka
METASOURCES = AUTO

kooka_SOURCES =	main.cpp kooka.cpp kookaview.cpp kookapref.cpp 		\
		img_saver.cpp ksaneocr.cpp miscwidgets.cpp 		\
		packageritem.cpp scanpackager.cpp 	\
		kocrstartdia.cpp kocrfindia.cpp

kooka_LDADD   = $(LIB_KFILE) -lkdeprint $(top_builddir)/libkscan/libkscan.la
kooka_LDFLAGS = $(KDE_RPATH) $(all_libraries)

INCLUDES         = $(sane_includes) -I$(top_srcdir)/libkscan $(all_includes)

noinst_HEADERS   = \
kookaview.h     scanpackager.h \
img_saver.h    ksaneocr.h       miscwidgets.h \
formathelp.h   kooka.h       packageritem.h   \
kookaiface.h  \
icons.h        kookapref.h   resource.h kocrfindia.h kocrstartdia.h

# kocrstartdia.h kocrfindia.h \

KDE_ICON = kooka

datadir       = $(kde_datadir)/kooka
data_DATA     = kookaui.rc

kdelnkdir   = $(kde_appsdir)/Utilities
kdelnk_DATA = kooka.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kooka.pot

