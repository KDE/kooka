/***************************************************************************
                          kocrstartdia.cpp  -  description                              
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

#include <qlayout.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qframe.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qcstring.h>
#include <kapp.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kanimwidget.h>

#include "resource.h"
#include "kocrstartdia.h"

#include <kscanslider.h>

/* defines for konfig-reading */
#define CFG_GROUP_OCR_DIA "ocrDialog"
#define CFG_GOCR_BINARY "gocrBinary"
#define CFG_GOCR_DUSTSIZE "gocrDustSize"
#define CFG_GOCR_GRAYLEVEL "gocrGrayLevel"
#define CFG_GOCR_SPACEWIDTH "gocrSpaceWidth"



KOCRStartDialog::KOCRStartDialog( QWidget *parent )
   :KDialogBase( parent,  "OCRStart", false, I18N("Optical Character Recognition"),
		 Cancel|User1, User1, true, I18N("Start OCR" ) )
{
   kdDebug() << "Starting KOCR-Start-Dialog!" << endl;
   // Layout-Boxes
   QWidget *page = new QWidget( this );
   CHECK_PTR( page );
   setMainWidget( page );

   KConfig *conf = KGlobal::config ();
   conf->setGroup( CFG_GROUP_OCR_DIA );

   // Caption - Label
   QVBoxLayout *topLayout = new QVBoxLayout( page, marginHint(), spacingHint() );
   QLabel *label = new QLabel( I18N( "<B>Starting Optical Character Recognition</B><P>"
				     "Kooka uses <I>gocr</I> for optical character recognition, "
				     "an Open Source Project<P>"
				     "Author of gocr is <B>Jörg Schulenburg</B><BR>"
				     "For more information about gocr see "
				     "<A HREF=http://jocr.sourceforge.net>"
				     "http://jocr.sourceforge.net</A>"),
				     page, "captionImage" );
   CHECK_PTR( label );
   topLayout->addWidget( label,1 );

   // Frame as a horizontal line
   QFrame *f1 = new QFrame( page );
   f1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
   topLayout->addWidget( f1 );

   // Entry-Field.
   entryOcrBinary = new KScanEntry( page, I18N( "Path to gocr-binary:  " ));
   QString res = conf->readPathEntry( CFG_GOCR_BINARY, "/usr/bin/gocr" );
   topLayout->addWidget( entryOcrBinary, 1 );
   entryOcrBinary->slSetEntry( res.local8Bit() );
   
   QFrame *f2 = new QFrame( page );
   f2->setFrameStyle( QFrame::HLine | QFrame::Sunken );
   topLayout->addWidget( f2 );

   /* Second Layout */
   QHBoxLayout *middle = new QHBoxLayout( page, marginHint(), spacingHint());
   topLayout->addLayout( middle );
   QVBoxLayout *sliderLayout = new QVBoxLayout( page, marginHint(), spacingHint());
   middle->addLayout( sliderLayout );

   /* This is for a 'work-in-progress'-Animation */
   ani = new KAnimWidget( (const QString) "kde", 48, page, "ANIMATION");
   middle->addWidget( ani );

   /* Slider for OCR-Options */
   sliderGrayLevel = new KScanSlider( page , I18N("Gray Level"), 0, 254 );
   sliderLayout->addWidget( sliderGrayLevel,1 );
   int numdefault = conf->readNumEntry( CFG_GOCR_GRAYLEVEL, 160 );
   sliderGrayLevel->slSetSlider( numdefault );
   
   sliderDustSize = new KScanSlider( page, I18N("Dustsize" ), 0, 60 );
   numdefault = conf->readNumEntry( CFG_GOCR_DUSTSIZE, 60 );
   sliderLayout->addWidget( sliderDustSize,1 );
   sliderDustSize->slSetSlider( numdefault );
   
   sliderSpace = new KScanSlider( page, I18N( "Spacewidth" ), 0, 60 );
   numdefault = conf->readNumEntry( CFG_GOCR_SPACEWIDTH, 0 );
   sliderLayout->addWidget( sliderSpace,1 );
   sliderSpace->slSetSlider( numdefault );

   topLayout->addStretch(2);

   /* Connect signals which disable the fields and store the configuration */
   connect( this, SIGNAL( user1Clicked()), this, SLOT( disableFields()));
   connect( this, SIGNAL( user1Clicked()), this, SLOT( writeConfig()));

}



KOCRStartDialog::~KOCRStartDialog()
{
   
}

void KOCRStartDialog::writeConfig( void )
{
   KConfig *conf = KGlobal::config ();
   conf->setGroup( CFG_GROUP_OCR_DIA );

   conf->writeEntry( CFG_GOCR_BINARY, getOCRCmd());
   conf->writeEntry( CFG_GOCR_GRAYLEVEL, getGraylevel());
   conf->writeEntry( CFG_GOCR_DUSTSIZE, getDustsize());
   conf->writeEntry( CFG_GOCR_SPACEWIDTH, getSpaceWidth());
}


void KOCRStartDialog::disableFields( void )
{
   kdDebug() << "About to disable the entry fields" << endl;
   sliderGrayLevel->setEnabled( false );
   sliderDustSize->setEnabled( false );
   sliderSpace->setEnabled( false );
   entryOcrBinary->setEnabled( false );

   ani->start();
}

/* The End ;) */
#include "kocrstartdia.moc"
