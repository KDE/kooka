/***************************************************************************
                  kookapref.cpp  -  Kookas preferences dialog
                             -------------------
    begin                : Wed Jan 5 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "kookapref.h"
#include "img_saver.h"

#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qtooltip.h>

#include <devselector.h>

KookaPreferences::KookaPreferences()
    : KDialogBase(IconList, i18n("Preferences"),
                  Help|Default|Ok|Apply|Cancel, Ok)
{
    // this is the base class for your preferences dialog.  it is now
    // a Treelist dialog.. but there are a number of other
    // possibilities (including Tab, Swallow, and just Plain)
    konf = KGlobal::config ();
    
    setupStartupPage();
    setupSaveFormatPage();    
}


void KookaPreferences::setupStartupPage()
{

    /* startup options */
    konf->setGroup( GROUP_STARTUP );
    
    QFrame *page = addPage( i18n("Startup"), i18n("Kooka Startup Preferences" ),
			    BarIcon("gear", KIcon::SizeMedium ) );
    QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );
    /* Description-Label */
    top->addWidget( new QLabel( i18n("Note that changing this options will affect Kooka's next start!"), page ));

    /* Query for network scanner (Checkbox) */
    cbNetQuery = new QCheckBox( i18n("Query network for available scanners"),
				page,  "CB_NET_QUERY" );
    QToolTip::add( cbNetQuery,
		   i18n( "Check this if you want a network query for available scanners.\nMind that does not mean a query over the entire network but only the stations configured for SANE" ));
    cbNetQuery->setChecked( ! konf->readBoolEntry( "QueryLocalOnly", false ) );

    kdDebug() << "###### fine 1 " << endl;
    
    /* Show scanner selection box on startup (Checkbox) */
    cbShowScannerSelection = new QCheckBox( i18n("Show the scanner selection box on next startup"),
					    page,  "CB_SHOW_SELECTION" );
    QToolTip::add( cbShowScannerSelection,
		   i18n( "Check this if you once checked 'do not show the scanner selection on startup',\nbut you want to see it again" ));

    cbShowScannerSelection->setChecked( !konf->readBoolEntry( "SkipStartupAsk", false ));
    kdDebug() << "###### fine 2 " << endl;

    /* Read startup image on startup (Checkbox) */
    cbReadStartupImage = new QCheckBox( i18n("Load the last image into the viewer on startup"),
					    page,  "CB_LOAD_ON_START" );
    QToolTip::add( cbReadStartupImage,
		   i18n( "Check this if you want Kooka to load the last selected image into the viewer on startup.\nIf your images are large, that might slow down Kooka's start." ));
    cbReadStartupImage->setChecked( konf->readBoolEntry( STARTUP_READ_IMAGE, true));
    kdDebug() << "###### fine 3 " << endl;

    /* -- */ 

    top->addWidget( cbNetQuery );
    top->addWidget( cbShowScannerSelection );
    top->addWidget( cbReadStartupImage );
   
    top->addStretch(10);

}

void KookaPreferences::setupSaveFormatPage( )
{
   konf->setGroup( OP_FILE_GROUP );    
   QFrame *page = addPage( i18n("Image Saving"), i18n("configure the image save assistant" ),
			    BarIcon("hdd_unmount", KIcon::SizeMedium ) );
   QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );

   /* Skip the format asking if a format entry  exists */
   cbSkipFormatAsk = new QCheckBox( i18n("always display image save assistant"),
				     page,  "CB_IMGASSIST_QUERY" );
   cbSkipFormatAsk->setChecked( konf->readBoolEntry( OP_FILE_ASK_FORMAT, true  ));
   QToolTip::add( cbSkipFormatAsk, i18n("Check this if you want to see the image save assistant even if there is a default format for the image type." ));
   top->addWidget( cbSkipFormatAsk );

   top->addStretch(10);
}


void KookaPreferences::slotOk( void )
{
  slotApply();
  accept();
    
}


void KookaPreferences::slotApply( void )
{
    /* ** startup options ** */
    konf->setGroup( GROUP_STARTUP );
    
    konf->writeEntry( STARTUP_ONLY_LOCAL,  !cbNetQuery->isChecked() );
    if( cbShowScannerSelection->isChecked() )
    {
	konf->writeEntry( STARTUP_SKIP_ASK, false );
    }

    if( cbReadStartupImage )
       konf->writeEntry( STARTUP_READ_IMAGE, cbReadStartupImage->isChecked());

    /* ** Image saver option(s) ** */
    konf->setGroup( OP_FILE_GROUP );
    bool showFormatAssist = cbSkipFormatAsk->isChecked();
    konf->writeEntry( OP_FILE_ASK_FORMAT, showFormatAssist );

    konf->sync();
}

void KookaPreferences::slotDefault( void )
{
    
}



#include "kookapref.moc"

