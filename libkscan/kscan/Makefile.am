INCLUDES = $(all_includes) $(sane_includes)

lib_LTLIBRARIES = libkscan.la

include_HEADERS = ksanedev.h ksaneslider.h
# noinst_HEADERS = 

libkscan_la_SOURCES = ksanedev.cpp ksaneslider.cpp
libkscan_la_LDFLAGS = $(SANE_LDFLAGS) -no-undefined -version-info 1:0
libkscan_la_LIBADD = $(LIB_KDEUI) $(LIB_SANE)

METASOURCES = AUTO
