INCLUDES = $(LIBSANE_INCLUDES) $(all_includes)

lib_LTLIBRARIES = libkscan.la

include_HEADERS = kscandevice.h kscanslider.h kgammatable.h \
	kscanoption.h kscanoptset.h \
	gammadialog.h dispgamma.h \
	scansourcedialog.h scanparams.h massscandialog.h devselector.h \
        img_canvas.h previewer.h imgscaledialog.h sizeindicator.h

# noinst_HEADERS = 

libkscan_la_SOURCES = kscandevice.cpp kscanslider.cpp kgammatable.cpp\
        kscanoption.cpp kscanoptset.cpp \
	gammadialog.cpp dispgamma.cpp \
	scansourcedialog.cpp scanparams.cpp massscandialog.cpp \
        devselector.cpp scandialog.cpp \
	img_canvas.cpp previewer.cpp imgscaledialog.cpp sizeindicator.cpp


libkscan_la_LDFLAGS = $(LIBSANE_LDFLAGS) $(all_libraries) -no-undefined -version-info 1:0
libkscan_la_LIBADD = $(LIBSANE_LIBS) $(LIB_KFILE)

kde_services_DATA = scanservice.desktop

METASOURCES = AUTO

SUBDIRS = pics

messages:
	$(XGETTEXT) *.cpp -o $(podir)/libkscan.pot

