## Makefile.am for kooka

# this has all of the subdirectories that make will recurse into.  if
# there are none, comment this out
SUBDIRS = pics doc

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = kooka

sane_includes=-I/usr/include/sane
# set the include path for X, qt and KDE
INCLUDES         = $(all_includes) $(sane_includes) -I$(top_srcdir)/kscan/kscan

# the library search path. 
kooka_LDFLAGS = $(KDE_RPATH) $(all_libraries)

# the libraries to link against.
kooka_LDADD   = $(LIB_KFILE) -L$(top_srcdir)/kscan/kscan -lkscan

# $(LIB_KSCAN)

# which sources should be compiled for kooka
kooka_SOURCES =	main.cpp kooka.cpp kookaview.cpp kookapref.cpp \
			img_canvas.cpp  \
			img_saver.cpp   \
			ksaneocr.cpp miscwidgets.cpp packageritem.cpp  \
			previewer.cpp scanpackager.cpp   \
			kocrstartdia.cpp kocrfindia.cpp

# these are the headers for your project
noinst_HEADERS   = \
img_canvas.h   kookaview.h     scanpackager.h \
img_saver.h    ksaneocr.h       miscwidgets.h \
formathelp.h   kooka.h       packageritem.h   \
kookaiface.h  previewer.h  \
icons.h        kookapref.h   resource.h kocrfindia.h kocrstartdia.h

# kocrstartdia.h kocrfindia.h \

kookadatadir = $(kde_datadir)/kooka
kookadata_DATA = kookaui.rc

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kooka.pot

KDE_ICON = kooka

 # this is where the kdelnk file will go 
kdelnkdir   = $(kde_appsdir)/Utilities
kdelnk_DATA = kooka.desktop
