/***************************************************************************
                          main.cpp  -  description                              
                             -------------------                                         
    begin                : Thu Dec  9 20:16:54 MET 1999
                                           
    copyright            : (C) 1999 by Klaas Freitag                         
    email                : freitag@suse.de                                     
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/

#include <qdict.h>
#include <qpixmap.h>

#include <kapp.h>
#include <dcopclient.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kglobal.h>
#include <kimageio.h>
#include <kiconloader.h>

#include "kooka.h"
#include "icons.h"
#include "version.h"

static const char *description =
          "<B>Kooka</B> is a KDE2 application which provides access to scanner hardware "
	      "using the SANE library.<P>"
	      "Kooka helps you scan, save your image in the correct "
	      "image format and perform <B>O</B>ptical <B>C</B>haracter <B>R</B>ecognition on it,"
	      "using <I>gocr</I>, Joerg Schulenburg's and friends' Open Source ocr program.<P>"
	      "For information on Kooka see <A HREF=http://>The kooka page</A><P>";


QDict<QPixmap> icons;

void *dbg_ptr;

int main( int argc, char ** argv )
{
   KAboutData about("kooka", I18N_NOOP("Kooka"), KOOKA_VERSION, I18N_NOOP(description),
		    KAboutData::License_GPL, "(C) 2000 Klaas Freitag");
   about.addAuthor( "Klaas Freitag", 0, "freitag@suse.de" );
   KCmdLineArgs::init(argc, argv, &about);
   KApplication app;
   KGlobal::locale()->insertCatalogue("libkscan");
   KImageIO::registerFormats();
   KIconLoader *loader = KGlobal::iconLoader();
   
   icons.insert("mini-color", new QPixmap( mini_color ));
   icons.insert("mini-gray", new QPixmap( mini_gray )); 	
   icons.insert("mini-lineart", new QPixmap( mini_lineart ));
   icons.insert("mini-folder", new QPixmap( mini_folder ));
   icons.insert("mini-floppy", new QPixmap( mini_floppy ));	
   icons.insert("mini-ray", new QPixmap( mini_ray ));	
   icons.insert("mini-folder_new", new QPixmap( mini_folder_new ));	
   icons.insert("mini-trash", new QPixmap( mini_trash ));
   icons.insert("mini-scan", new QPixmap( mini_scan ));
   icons.insert("mini-ocr", new QPixmap( mini_ocr ));
   icons.insert("mini-colorlock", new QPixmap( mini_colorlock ));
   icons.insert("mini-preview", new QPixmap( mini_preview ));
   icons.insert("mini-fitwidth", new QPixmap( mini_fitwidth ));
   icons.insert("mini-fitheight", new QPixmap( mini_fitheight ));


   icons.insert("mini-folder", new QPixmap( loader->loadIcon( "folder",KIcon::Small ))); 
   icons.insert("mini-folder-open", new QPixmap( loader->loadIcon( "folder_open",KIcon::Small )));

   // register ourselves as a dcop client
   app.dcopClient()->registerAs(app.name(), false);
   if (app.isRestored()) {
      RESTORE(Kooka);

   }
   else
   {
      // no session.. just start up normally

      Kooka  *kooka = new Kooka();
      
      kooka->show();
      app.processEvents();
      kooka->startup();
      
   }
   return app.exec();
}
