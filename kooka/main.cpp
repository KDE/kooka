/***************************************************************************
                          main.cpp  -  description                              
                             -------------------                                         
    begin                : Thu Dec  9 20:16:54 MET 1999
                                           
    copyright            : (C) 1999 by Klaas Freitag                         
    email                : Klaas.Freitag@gmx.de                                     
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/



#include <qapplication.h>
#include <qwindowsstyle.h>
#include <qfont.h>
#include <qdict.h>
#include <qpixmap.h>

#include <kapp.h>
#include <dcopclient.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include "kooka.h"
#include "icons.h"

static const char *description =
    I18N_NOOP("A KDE Application");

static const char *version = "v0.1";

QDict<QPixmap> icons;

void *dbg_ptr;

int main( int argc, char ** argv )
{
   KAboutData about("kooka", I18N_NOOP("Kooka"), version, description,
		    KAboutData::License_GPL, "(C) 2000 Klaas Freitag");
   about.addAuthor( "Klaas Freitag", 0, "Freitag@SuSE.de" );
    KCmdLineArgs::init(argc, argv, &about);
    KApplication app;

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


    // register ourselves as a dcop client
    app.dcopClient()->registerAs(app.name(), false);
    if (app.isRestored()) {
       // RESTORE( )

    }
    else
    {
        // no session.. just start up normally
       
       // a.setFont(QFont("helvetica",12));
       // a.setStyle( new QWindowsStyle() );


       Kooka  *kooka = new Kooka();
       // a.setMainWidget(ksanetest);
       /* Ugly, but works for now... */
       // ksanetest->resize( 900, 720 );
       // ksanetest->setCaption( "Scanning with SANE" );
       kooka->show();
    }
    return app.exec();
}
