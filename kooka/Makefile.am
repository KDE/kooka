## Makefile.am for kooka

SUBDIRS =		pics

bin_PROGRAMS =		kooka
METASOURCES =		AUTO

kooka_SOURCES =		main.cpp kooka.cpp kookaview.cpp kookapref.cpp 		\
			imgsaver.cpp formatdialog.cpp ksaneocr.cpp 		\
			kookaimage.cpp kookaimagemeta.cpp scanpackager.cpp 	\
			imgnamecombo.cpp imageselectline.cpp      		\
			thumbview.cpp thumbviewitem.cpp                         \
			dwmenuaction.cpp kocrbase.cpp           		\
			kocrgocr.cpp kocrkadmos.cpp kadmosocr.cpp ocrword.cpp   \
			ocrresedit.cpp kookaprint.cpp imgprintdialog.cpp	\
			kocrocrad.cpp scanparamsdialog.cpp newscanparams.cpp

kooka_CXXFLAGS =	-DUSE_KIMAGEIO

kooka_LDADD =		$(LIB_KFILE) -lkdeprint -lktexteditor $(LIBTIFF)	\
			$(top_builddir)/libkscan/libkscan.la $(KADMOS_LIB)	\
			$(LIB_KSPELL)
kooka_LDFLAGS =		$(KDE_RPATH) $(all_libraries)

INCLUDES =		-I$(top_srcdir)/libkscan $(all_includes)		\
			$(LIBSANE_INCLUDES) $(KADMOS_INC)

noinst_HEADERS =	kookaview.h scanpackager.h				\
			imgsaver.h formatdialog.h ksaneocr.h			\
			kooka.h kookaiface.h thumbview.h thumbviewitem.h	\
			kookapref.h resource.h imgnamecombo.h			\
			imageselectline.h kookaimage.h kookaimagemeta.h		\
			kocrbase.h kocrgocr.h kocrkadmos.h			\
			kadmosocr.h ocrword.h ocrresedit.h kookaprint.h		\
			imgprintdialog.h kocrocrad.h scanparamsdialog.h		\
			newscanparams.h

appdatadir =		$(kde_datadir)/kooka
appdata_DATA =		kookaui.rc

kde_conf_DATA =		kookarc

xdg_apps_DATA =		kooka.desktop

messages:		rc.cpp
			$(XGETTEXT) *.cpp -o $(podir)/kooka.pot
