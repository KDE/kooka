INCLUDES = $(all_includes) $(sane_includes)

lib_LTLIBRARIES = libkscan.la

include_HEADERS = kscandevice.h kscanslider.h kgammatable.h
# noinst_HEADERS = 

libkscan_la_SOURCES = kscandevice.cpp kscanslider.cpp kgammatable.cpp
libkscan_la_LDFLAGS = $(SANE_LDFLAGS) $(all_libraries) -no-undefined -version-info 1:0
libkscan_la_LIBADD = $(LIB_KDEUI) $(LIB_SANE)

METASOURCES = AUTO
