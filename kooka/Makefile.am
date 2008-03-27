## Makefile.am for kooka

SUBDIRS =		pics

bin_PROGRAMS =		kooka
METASOURCES =		AUTO

kooka_SOURCES =		main.cpp kooka.cpp kookaview.cpp kookapref.cpp 		\
			imgsaver.cpp formatdialog.cpp ocrengine.cpp 		\
			kookaimage.cpp kookaimagemeta.cpp scanpackager.cpp 	\
			imgnamecombo.cpp imageselectline.cpp      		\
			thumbview.cpp thumbviewitem.cpp                         \
			dwmenuaction.cpp ocrbasedialog.cpp ocrgocrdialog.cpp	\
			ocrkadmosdialog.cpp kadmosocr.cpp ocrword.cpp   	\
			ocrresedit.cpp kookaprint.cpp imgprintdialog.cpp	\
			ocrocraddialog.cpp scanparamsdialog.cpp			\
			newscanparams.cpp ocrgocrengine.cpp ocrocradengine.cpp	\
			ocrkadmosengine.cpp

kooka_CXXFLAGS =	-DUSE_KIMAGEIO

kooka_LDADD =		$(LIB_KFILE) -lkdeprint -lktexteditor $(LIBTIFF)	\
			$(top_builddir)/libkscan/libkscan.la $(KADMOS_LIB)	\
			$(LIB_KSPELL)
kooka_LDFLAGS =		$(KDE_RPATH) $(all_libraries)

INCLUDES =		-I$(top_srcdir)/libkscan $(all_includes)		\
			$(LIBSANE_INCLUDES) $(KADMOS_INC)

noinst_HEADERS =	kookaview.h scanpackager.h				\
			imgsaver.h formatdialog.h ocrbasedialog.h		\
			kooka.h kookaiface.h thumbview.h thumbviewitem.h	\
			kookapref.h resource.h imgnamecombo.h			\
			imageselectline.h kookaimage.h kookaimagemeta.h		\
			ocrbasedialog.h ocrgocrdialog.h ocrkadmosdialog.h	\
			kadmosocr.h ocrword.h ocrresedit.h kookaprint.h		\
			imgprintdialog.h ocrocraddialog.h scanparamsdialog.h	\
			newscanparams.h ocrgocrengine.h ocrocradengine.h	\
			ocrkadmosengine.h

appdatadir =		$(kde_datadir)/kooka
appdata_DATA =		kookaui.rc

kde_conf_DATA =		kookarc

xdg_apps_DATA =		kooka.desktop

messages:		rc.cpp
			$(XGETTEXT) *.cpp -o $(podir)/kooka.pot
