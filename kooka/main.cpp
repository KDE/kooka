/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Thu Dec  9 20:16:54 MET 1999

    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/


/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

#include <qdict.h>
#include <qpixmap.h>

#include <kapplication.h>
#include <dcopclient.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kglobal.h>
#include <kimageio.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kwin.h>

#include "kooka.h"
#include "version.h"

static const char description[] = I18N_NOOP(
	"Kooka is a KDE application which provides access to scanner hardware\n"
	"using the SANE library.\n\n"
	"Kooka allows you to scan and save in any image format that KDE supports,\n"
	"and can perform Optical Character Recognition using the open source GOCR\n"
        "or OCRAD programs, or the commercial KADMOS library.");

static const char license[] = I18N_NOOP(
	"This program is distributed under the terms of the GPL v2 as publishec by\n"
	"the Free Software Foundation\n\n"
	"As a special exception, permission is given to link this program\n"
	"with any version of the KADMOS ocr/icr engine of reRecognition GmbH,\n"
	"Kreuzlingen and distribute the resulting executable without\n"
	"including the source code for KADMOS in the source distribution.\n\n"
	"As a special exception, permission is given to link this program\n"
	"with any edition of Qt, and distribute the resulting executable,\n"
	"without including the source code for Qt in the source distribution.\n");


static KCmdLineOptions options[] =
{
  { "d ", I18N_NOOP("The SANE compatible device specification (e.g. umax:/dev/sg0)"), "" },
  { "g", I18N_NOOP("Gallery mode - do not connect to scanner"), "" },
  KCmdLineLastOption
};



int main( int argc, char *argv[] )
{
   KAboutData about("kooka", I18N_NOOP("Kooka"), KOOKA_VERSION, description,
		    KAboutData::License_GPL_V2, "(C) 2000 Klaas Freitag", 0,
		    I18N_NOOP("http://techbase.kde.org/Projects/Kooka"));

   about.addAuthor( "Jonathan Marten", I18N_NOOP("current maintainer"), "jjm@keelhaul.me.uk" );
   about.addAuthor( "Klaas Freitag", I18N_NOOP("developer"), "freitag@suse.de" );
   about.addAuthor( "Mat Colton", I18N_NOOP("graphics, web"), "mat@colton.de" );
   about.setLicenseText( license );

   KCmdLineArgs::init(argc, argv, &about);
   KCmdLineArgs::addCmdLineOptions( options ); // Add my own options.

   KApplication app;
   KGlobal::locale()->insertCatalogue("libkscan");
   KImageIO::registerFormats();
   KIconLoader *loader = KGlobal::iconLoader();

   KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
   QCString  devToUse = args->getOption( "d" );
   if( args->isSet("g") )
   {
      devToUse = "gallery";
   }
   kdDebug(28000) << "DevToUse is " << devToUse << endl;

   if (args->count() == 1)
   {
      args->usage();
      // exit(-1);
   }


   Kooka  *kooka = new Kooka(devToUse);
   app.setMainWidget( kooka );

   KWin::setIcons(kooka->winId(), loader->loadIcon( "scanner", KIcon::Desktop ),
		  loader->loadIcon("scanner", KIcon::Small) );

   kooka->show();
   app.processEvents();
   kooka->startup();
   args->clear();
   int ret = app.exec();

   return ret;

}
