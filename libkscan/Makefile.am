INCLUDES = $(sane_includes) $(all_includes) 

lib_LTLIBRARIES = libkscan.la

include_HEADERS = kscandevice.h kscanslider.h kgammatable.h \
	kscanoption.h kscanoptset.h \
	gammadialog.h dispgamma.h \
	scansourcedialog.h scanparams.h massscandialog.h devselector.h

# noinst_HEADERS = 

libkscan_la_SOURCES = kscandevice.cpp kscanslider.cpp kgammatable.cpp\
        kscanoption.cpp kscanoptset.cpp \
	gammadialog.cpp dispgamma.cpp \
	scansourcedialog.cpp scanparams.cpp massscandialog.cpp \
        devselector.cpp scandialog.cpp


libkscan_la_LDFLAGS = $(SANE_LDFLAGS) $(all_libraries) -no-undefined -version-info 1:0 -module
libkscan_la_LIBADD = $(LIB_KFILE) $(LIB_SANE)

kde_services_DATA = scanservice.desktop

METASOURCES = AUTO
