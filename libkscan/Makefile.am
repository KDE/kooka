INCLUDES = $(sane_includes) $(all_includes)

lib_LTLIBRARIES = libkscan.la

include_HEADERS = kscandevice.h kscanslider.h kgammatable.h \
	kscanoption.h kscanoptset.h \
	gammadialog.h dispgamma.h \
	scansourcedialog.h scanparams.h massscandialog.h devselector.h \
        img_canvas.h previewer.h imgscaledialog.h

# noinst_HEADERS = 

libkscan_la_SOURCES = kscandevice.cpp kscanslider.cpp kgammatable.cpp\
        kscanoption.cpp kscanoptset.cpp \
	gammadialog.cpp dispgamma.cpp \
	scansourcedialog.cpp scanparams.cpp massscandialog.cpp \
        devselector.cpp scandialog.cpp \
	img_canvas.cpp previewer.cpp imgscaledialog.cpp


libkscan_la_LDFLAGS = $(all_libraries) $(SANE_LDFLAGS) -no-undefined -version-info 1:0 -module
libkscan_la_LIBADD = $(LIB_SANE) $(LIB_KFILE)

kde_services_DATA = scanservice.desktop

METASOURCES = AUTO

SUBDIRS = pics

messages:
	$(XGETTEXT) *.cpp -o $(podir)/libkscan.pot

