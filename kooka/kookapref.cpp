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
#include <knuminput.h>
#include <kcolorbutton.h>
#include <kstandarddirs.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qvgroupbox.h>
#include <qgrid.h>

#include <devselector.h>
#include "thumbview.h"
#include "miscwidgets.h"

KookaPreferences::KookaPreferences()
    : KDialogBase(IconList, i18n("Preferences"),
                  Help|Default|Ok|Apply|Cancel, Ok )
{
    // this is the base class for your preferences dialog.  it is now
    // a Treelist dialog.. but there are a number of other
    // possibilities (including Tab, Swallow, and just Plain)
    konf = KGlobal::config ();

    setupStartupPage();
    setupSaveFormatPage();
    setupThumbnailPage();
}


void KookaPreferences::setupStartupPage()
{

    /* startup options */
    konf->setGroup( GROUP_STARTUP );

    QFrame *page = addPage( i18n("Startup"), i18n("Kooka Startup Preferences" ),
			    BarIcon("gear", KIcon::SizeMedium ) );
    QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );
    /* Description-Label */
    top->addWidget( new QLabel( i18n("Note that changing these options will affect Kooka's next start!"), page ));

    /* Query for network scanner (Checkbox) */
    cbNetQuery = new QCheckBox( i18n("Query network for available scanners"),
				page,  "CB_NET_QUERY" );
    QToolTip::add( cbNetQuery,
		   i18n( "Check this if you want a network query for available scanners.\nNote that this does not mean a query over the entire network but only the stations configured for SANE!" ));
    cbNetQuery->setChecked( ! (konf->readBoolEntry( STARTUP_ONLY_LOCAL, false )) );

    
    /* Show scanner selection box on startup (Checkbox) */
    cbShowScannerSelection = new QCheckBox( i18n("Show the scanner selection box on next startup"),
					    page,  "CB_SHOW_SELECTION" );
    QToolTip::add( cbShowScannerSelection,
		   i18n( "Check this if you once checked 'do not show the scanner selection on startup',\nbut you want to see it again." ));

    cbShowScannerSelection->setChecked( !konf->readBoolEntry( STARTUP_SKIP_ASK, false ));

    /* Read startup image on startup (Checkbox) */
    cbReadStartupImage = new QCheckBox( i18n("Load the last image into the viewer on startup"),
					    page,  "CB_LOAD_ON_START" );
    QToolTip::add( cbReadStartupImage,
		   i18n( "Check this if you want Kooka to load the last selected image into the viewer on startup.\nIf your images are large, that might slow down Kooka's start." ));
    cbReadStartupImage->setChecked( konf->readBoolEntry( STARTUP_READ_IMAGE, true));

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
   cbSkipFormatAsk = new QCheckBox( i18n("Always display image save assistant"),
				     page,  "CB_IMGASSIST_QUERY" );
   cbSkipFormatAsk->setChecked( konf->readBoolEntry( OP_FILE_ASK_FORMAT, true  ));
   QToolTip::add( cbSkipFormatAsk, i18n("Check this if you want to see the image save assistant even if there is a default format for the image type." ));
   top->addWidget( cbSkipFormatAsk );

   top->addStretch(10);
}

void KookaPreferences::setupThumbnailPage()
{
   konf->setGroup( THUMB_GROUP );

   QFrame *page = addPage( i18n("Thumbnail View"), i18n("Thumbnail Gallery View" ),
			    BarIcon("appearance", KIcon::SizeMedium ) );
   QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );

   top->addWidget( new QLabel( i18n("Here you can configure the appearance of the thumbnail view of your scan picture gallery."),page ));

   /* Backgroundimage */
   KStandardDirs stdDir;
   QString bgImg = konf->readEntry( BG_WALLPAPER, "" );
   if( bgImg == "" )
      bgImg = stdDir.findResource( "data", STD_TILE_IMG );

   /* image file selector */
   QVGroupBox *hgb1 = new QVGroupBox( i18n("Thumbview Background" ), page );
   m_tileSelector = new ImageSelectLine( hgb1, i18n("Select background image:"));
   kdDebug(29000) << "Setting tile url " << bgImg << endl;
   m_tileSelector->setURL( KURL(bgImg) );
   
   top->addWidget( hgb1 );

   QHBoxLayout *layMain = new QHBoxLayout( page, 0, spacingHint());
   top->addLayout( layMain, 0 );

   QVBoxLayout *layBBoxes = new QVBoxLayout( page, 0, spacingHint());
   layMain->addLayout( layBBoxes, 0 );

   /* Add the Boxes to configure size, framestyle and background */

   
   QVGroupBox *hgb2 = new QVGroupBox( i18n("Thumbnail Size" ), page );
   QVGroupBox *hgb3 = new QVGroupBox( i18n("Thumbnail Frame" ), page );

   /* Thumbnail size */
   int w = konf->readNumEntry( PIXMAP_WIDTH, 100);
   int h = konf->readNumEntry( PIXMAP_HEIGHT, 120 );
   QGrid *lGrid = new QGrid( 2, hgb2 );
   lGrid->setSpacing( 2 );
   QLabel *l1 = new QLabel( i18n("Thumbnail maximum &width:"), lGrid );
   m_thumbWidth = new KIntNumInput( w, lGrid );
   l1->setBuddy( m_thumbWidth );

   lGrid->setSpacing( 4 );
   l1 = new QLabel( i18n("Thumbnail maximum &height:"), lGrid );
   m_thumbHeight = new KIntNumInput( m_thumbWidth, h, lGrid );
   l1->setBuddy( m_thumbHeight );
   
   /* Frame Stuff */
   int frameWidth = konf->readNumEntry( THUMB_MARGIN, 3 );
   QColor col1    = konf->readColorEntry( MARGIN_COLOR1, &(colorGroup().base()));
   QColor col2    = konf->readColorEntry( MARGIN_COLOR2, &(colorGroup().foreground()));

   QGrid *fGrid = new QGrid( 2, hgb3 );
   fGrid->setSpacing( 2 );
   l1 = new QLabel(i18n("Thumbnail &frame width:"), fGrid );
   m_frameWidth = new KIntNumInput( frameWidth, fGrid );
   l1->setBuddy( m_frameWidth );

   l1 = new QLabel(i18n("Frame color &1: "), fGrid );
   m_colButt1 = new KColorButton( col1, fGrid );
   l1->setBuddy( m_colButt1 );

   l1 = new QLabel(i18n("Frame color &2: "), fGrid );
   m_colButt2 = new KColorButton( col2, fGrid );
   l1->setBuddy( m_colButt2 );
   /* TODO: Gradient type */
   
   layBBoxes->addWidget( hgb2, 10);
   layBBoxes->addWidget( hgb3, 10);
   layBBoxes->addStretch(10);

}


void KookaPreferences::slotOk( void )
{
  slotApply();
  accept();
    
}


void KookaPreferences::slotApply( void )
{
    /* ** startup options ** */

   /** write the global one, to read from libkscan also */
   konf->setGroup(QString::fromLatin1(GROUP_STARTUP));
   bool cbVal = !(cbShowScannerSelection->isChecked());
   kdDebug(29000) << "Writing for " << STARTUP_SKIP_ASK << ": " << cbVal << endl;
   konf->writeEntry( STARTUP_SKIP_ASK, cbVal, true, true ); /* global flag goes to kdeglobals */

   /* only search for local (=non-net) scanners ? */
   konf->writeEntry( STARTUP_ONLY_LOCAL,  !cbNetQuery->isChecked(), true, true ); /* global */

   /* Should kooka open the last displayed image in the viewer ? */
   if( cbReadStartupImage )
      konf->writeEntry( STARTUP_READ_IMAGE, cbReadStartupImage->isChecked()); 

    /* ** Image saver option(s) ** */
    konf->setGroup( OP_FILE_GROUP );
    bool showFormatAssist = cbSkipFormatAsk->isChecked();
    konf->writeEntry( OP_FILE_ASK_FORMAT, showFormatAssist );

    /* ** Thumbnail options ** */
    konf->setGroup( THUMB_GROUP );
    konf->writeEntry( PIXMAP_WIDTH, m_thumbWidth->value() );
    konf->writeEntry( PIXMAP_HEIGHT, m_thumbHeight->value() );
    konf->writeEntry( THUMB_MARGIN, m_frameWidth->value() );
    konf->writeEntry( MARGIN_COLOR1, m_colButt1->color());
    konf->writeEntry( MARGIN_COLOR2, m_colButt2->color());

    KURL bgUrl = m_tileSelector->selectedURL().url();
    bgUrl.setProtocol("");
    kdDebug(28000) << "Writing tile-pixmap " << bgUrl.prettyURL() << endl;
    konf->writeEntry( BG_WALLPAPER, bgUrl.url() );
    
    konf->sync();

    emit dataSaved();
}

void KookaPreferences::slotDefault( void )
{
    
}



#include "kookapref.moc"

