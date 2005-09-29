/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>  

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <stdlib.h>

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcstring.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qfile.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qstrlist.h>
#include <qstringlist.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>

#include "devselector.h"


DeviceSelector::DeviceSelector( QWidget *parent, QStrList& devList,
				const QStringList& hrdevList )
    : KDialogBase( parent,  "DeviceSel", true, i18n("Welcome to Kooka"),
		   Ok|Cancel, Ok, true )
{
   kdDebug(29000) << "Starting DevSelector!" << endl;
   // Layout-Boxes
   QWidget *page = new QWidget( this );
   Q_CHECK_PTR( page );
   setMainWidget( page );

   QVBoxLayout *topLayout = new QVBoxLayout( page, marginHint(), spacingHint() );
   QLabel *label = new QLabel( page, "captionImage" );
   Q_CHECK_PTR( label );
   label->setPixmap( QPixmap( "kookalogo.png" ));
   label->resize( 100, 350 );
   topLayout->addWidget( label );

   selectBox = new QButtonGroup( 1, Horizontal, i18n( "Select Scan Device" ),
				 page, "ButtonBox");
   Q_CHECK_PTR( selectBox );
   selectBox->setExclusive( true );
   topLayout->addWidget( selectBox );
   setScanSources( devList, hrdevList );

   cbSkipDialog = new QCheckBox( i18n("&Do not ask on startup again, always use this device"),
				 page, "CBOX_SKIP_ON_START" );

   KConfig *gcfg = KGlobal::config();
   gcfg->setGroup(QString::fromLatin1(GROUP_STARTUP));
   bool skipDialog = gcfg->readBoolEntry( STARTUP_SKIP_ASK, false );
   cbSkipDialog->setChecked( skipDialog );

   topLayout->addWidget(cbSkipDialog);
   
}

QCString DeviceSelector::getDeviceFromConfig( void ) const
{
   KConfig *gcfg = KGlobal::config();
   gcfg->setGroup(QString::fromLatin1(GROUP_STARTUP));
   bool skipDialog = gcfg->readBoolEntry( STARTUP_SKIP_ASK, false );
   
   QCString result;

   /* in this case, the user has checked 'Do not ask me again' and does not
    * want to be bothered any more.
    */
   result = QFile::encodeName(gcfg->readEntry( STARTUP_SCANDEV, "" ));
   kdDebug(29000) << "Got scanner from config file to use: " << result << endl;
   
   /* Now check if the scanner read from the config file is available !
    * if not, ask the user !
    */
   if( skipDialog && devices.find( result ) > -1 )
   {
      kdDebug(29000) << "Scanner from Config file is available - fine." << endl;
   }
   else
   {
      kdDebug(29000) << "Scanner from Config file is _not_ available" << endl;
      result = QCString();
   }
   
   return( result );
}

bool DeviceSelector::getShouldSkip( void ) const
{
   return( cbSkipDialog->isChecked());
}

QCString DeviceSelector::getSelectedDevice( void ) const
{
   unsigned int selID = selectBox->id( selectBox->selected() );

   int dcount = devices.count();
   kdDebug(29000) << "The Selected ID is <" << selID << ">/" << dcount << endl;

   const char * dev = devices.at( selID );

   kdDebug(29000) << "The selected device: <" << dev << ">" << endl;

   /* Store scanner selection settings */
   KConfig *c = KGlobal::config();
   c->setGroup(QString::fromLatin1(GROUP_STARTUP));
   /* Write both the scan device and the skip-start-dialog flag global. */
   c->writeEntry( STARTUP_SCANDEV, dev, true, true );
   c->writeEntry( STARTUP_SKIP_ASK, getShouldSkip(), true, true );
   c->sync();
   
   return dev;
}


void DeviceSelector::setScanSources( const QStrList& sources,
				     const QStringList& hrSources )
{
   bool default_ok = false;
   KConfig *gcfg = KGlobal::config();
   gcfg->setGroup(QString::fromLatin1(GROUP_STARTUP));
   QCString defstr = gcfg->readEntry( STARTUP_SCANDEV, "" ).local8Bit();

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
