/***************************************************************************
                          devselector.cpp  -  description
                             -------------------
    begin                : Fri Now 10 2000
    copyright            : (C) 2000 by Klaas Freitag
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

#include <stdlib.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qframe.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qcstring.h>
#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "devselector.h"
#include <kdebug.h>


/* Definitions for the kapplication Config object */


DeviceSelector::DeviceSelector( QWidget *parent, QStrList& devList, QStringList& hrdevList )
   :KDialogBase( parent,  "DeviceSel", true, i18n("Welcome to Kooka"),
		 Ok|Cancel, Ok, true )
{
   kdDebug() << "Starting DevSelector!" << endl;
   // Layout-Boxes
   QWidget *page = new QWidget( this );
   CHECK_PTR( page );
   setMainWidget( page );

   QVBoxLayout *topLayout = new QVBoxLayout( page, marginHint(), spacingHint() );
   QLabel *label = new QLabel( page, "captionImage" );
   CHECK_PTR( label );
   label->setPixmap( *(new QPixmap( "kookalogo.png" )));
   label->resize( 100, 350 );
   topLayout->addWidget( label );

   selectBox = new QButtonGroup( 1, Horizontal, i18n( "Select a scan device" ),
				 page, "ButtonBox");
   CHECK_PTR( selectBox );
   selectBox->setExclusive( true );
   topLayout->addWidget( selectBox );
   setScanSources( devList, hrdevList );

   cbSkipDialog = new QCheckBox( i18n("&Do not ask on startup again, always use this device."),
				 page, "CBOX_SKIP_ON_START" );
   topLayout->addWidget(cbSkipDialog);

}


bool DeviceSelector::getShouldSkip( void ) const
{
   return( cbSkipDialog->isChecked());
}

QString DeviceSelector::getSelectedDevice( void ) const
{
   CHECK_PTR( selectBox );

   unsigned int selID = selectBox->id( selectBox->selected());

   int c = devices.count();
   kdDebug() << "The Selected ID is <" << selID << ">/" << c << endl;

   QString dev = devices[ selID ];

   kdDebug() << "The selected device: <" << dev << ">" << endl;
   KGlobal::config()->setGroup( GROUP_STARTUP );
   KGlobal::config()->writeEntry( STARTUP_SCANDEV, dev );

   return dev;
}


void DeviceSelector::setScanSources( QStrList& sources, QStringList& hrSources )
{
   bool default_ok = false;

   KGlobal::config()->setGroup( GROUP_STARTUP );
   QString defstr = KGlobal::config()->readEntry( STARTUP_SCANDEV, "" );
   uint c = sources.count();

   /* Popup if no scanner exists */
   if( c == 0 )
   {
      // No device found -> seems to be no scanner installed
      QString msg;
      msg = i18n("There is a problem in your scanner configuration.");
      msg += i18n("\nNo scanner was found on your system.");
      msg += i18n( "\nCheck the SANE installation !\n\n" );
      msg += i18n( "Do you want to continue?");
      int result = KMessageBox::questionYesNo(this, msg, i18n("SANE Installation Problem"));

      if( result == KMessageBox::No ) {
	 exit(0);
      }
   }

   /* Selector-Stuff*/
   uint nr = 0;
   int  checkDefNo = 0;

   for ( const char* s = sources.first(); selectBox && s; s=sources.next() )
   {
      QString css = QString::fromLocal8Bit( s );
      QString hr( hrSources[nr]);
      QString num;
      num.sprintf( "&%d. ", 1+nr );
      kdDebug() << num << hr << endl;
      QRadioButton *rb = new QRadioButton( num+css +"\n"+hr, selectBox );

      devices.append(css);

      if( css == defstr )
      {
	 checkDefNo = nr;
      }
      selectBox->insert( rb );
      nr++;
   }

   /* Default still needs to be set */
   if( selectBox && ! default_ok )
   {
      /* if no default found, set the first */
      QRadioButton *rb = (QRadioButton*) selectBox->find( checkDefNo );
      CHECK_PTR( rb );
      rb->setChecked( true );
   }

}

DeviceSelector::~DeviceSelector()
{

}

/* The End ;) */
#include "devselector.moc"
