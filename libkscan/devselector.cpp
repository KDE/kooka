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

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcstring.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qstrlist.h>
#include <qstringlist.h>

#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "devselector.h"


DeviceSelector::DeviceSelector( QWidget *parent, const QStrList& devList,
				const QStringList& hrdevList )
    : KDialogBase( parent,  "DeviceSel", true, i18n("Welcome to Kooka"),
		   Ok|Cancel, Ok, true )
{
   kdDebug(29000) << "Starting DevSelector!" << endl;
   // Layout-Boxes
   QWidget *page = new QWidget( this );
   CHECK_PTR( page );
   setMainWidget( page );

   QVBoxLayout *topLayout = new QVBoxLayout( page, marginHint(), spacingHint() );
   QLabel *label = new QLabel( page, "captionImage" );
   CHECK_PTR( label );
   label->setPixmap( QPixmap( "kookalogo.png" ));
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

QCString DeviceSelector::getSelectedDevice( void ) const
{
   unsigned int selID = selectBox->id( selectBox->selected() );

   int c = devices.count();
   kdDebug(29000) << "The Selected ID is <" << selID << ">/" << c << endl;

   const char * dev = devices.at( selID );

   kdDebug(29000) << "The selected device: <" << dev << ">" << endl;
   KGlobal::config()->setGroup( GROUP_STARTUP );
   KGlobal::config()->writeEntry( STARTUP_SCANDEV, dev );

   return dev;
}


void DeviceSelector::setScanSources( const QStrList& sources,
				     const QStringList& hrSources )
{
   bool default_ok = false;

   KGlobal::config()->setGroup( GROUP_STARTUP );
   QCString defstr = KGlobal::config()->readEntry( STARTUP_SCANDEV, "" ).local8Bit();

   /* Popup if no scanner exists */
   if( sources.isEmpty() )
   {
      // No device found -> seems to be no scanner installed
      QString msg;
      msg = i18n("There is a problem in your scanner configuration."
		 "\nNo scanner was found on your system."
		 "\nCheck the SANE installation!\n\n"
		 "Do you want to continue?");
      int result = KMessageBox::questionYesNo(this, msg, i18n("SANE Installation Problem"));

      if( result == KMessageBox::No ) {
#ifdef __GNUC__
#warning Needs some change, we cannot exit() here...
#endif
	 exit(0);
      }
   }

   /* Selector-Stuff*/
   uint nr = 0;
   int  checkDefNo = 0;

   QStrListIterator it( sources );
   QStringList::ConstIterator it2 = hrSources.begin();
   for ( ; it.current(); ++it, ++it2 )
   {
      QString text = QString::fromLatin1("&%1. %2\n%3").arg(1+nr).arg( QString::fromLocal8Bit(*it) ).arg( *it2 );
      QRadioButton *rb = new QRadioButton( text, selectBox );
      selectBox->insert( rb );

      devices.append( *it );

      if( *it == defstr )
	 checkDefNo = nr;

      nr++;
   }

   /* Default still needs to be set */
   if( ! default_ok )
   {
      /* if no default found, set the first */
      QRadioButton *rb = (QRadioButton*) selectBox->find( checkDefNo );
      if ( rb )
	  rb->setChecked( true );
   }
}

DeviceSelector::~DeviceSelector()
{

}

/* The End ;) */
#include "devselector.moc"
