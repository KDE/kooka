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
#include <qfileinfo.h>
#include <qtooltip.h>

#include <kapp.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kanimwidget.h>
#include <kseparator.h>
#include <kmessagebox.h>

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
   :KDialogBase( parent,  "OCRStart", false, i18n("Optical Character Recognition"),
		 Cancel|User1, User1, true, i18n("Start OCR" ) )
{
   kdDebug(28000) << "Starting KOCR-Start-Dialog!" << endl;
   // Layout-Boxes
   QWidget *page = new QWidget( this );
   CHECK_PTR( page );
   setMainWidget( page );

   KConfig *conf = KGlobal::config ();
   conf->setGroup( CFG_GROUP_OCR_DIA );

   // Caption - Label
   QVBoxLayout *topLayout = new QVBoxLayout( page, marginHint(), spacingHint() );
   QLabel *label = new QLabel( i18n( "<B>Starting Optical Character Recognition</B><P>"
				     "Kooka uses <I>gocr</I> for optical character recognition, "
				     "an Open Source Project<P>"
				     "Author of gocr is <B>Joerg Schulenburg</B><BR>"
				     "For more information about gocr see "
				     "<A HREF=http://jocr.sourceforge.net>"
				     "http://jocr.sourceforge.net</A>"),
				     page, "captionImage" );
   CHECK_PTR( label );
   topLayout->addWidget( label,1 );

   // Horizontal line
   KSeparator* f1 = new KSeparator( KSeparator::HLine, page);
   topLayout->addWidget( f1 );

   // Entry-Field.
   entryOcrBinary = new KScanEntry( page, i18n( "Path to 'gocr' binary: " ));
   QString res = conf->readPathEntry( CFG_GOCR_BINARY, "notFound" );
   if( res == "notFound" )
   {
      res = tryFindGocr();
   }
   topLayout->addWidget( entryOcrBinary, 1 );
   entryOcrBinary->slSetEntry( res );

   connect( entryOcrBinary, SIGNAL(valueChanged( const QCString& )),
	    this, SLOT( checkOCRBinaryShort( const QCString& )));
   connect( entryOcrBinary, SIGNAL(returnPressed( const QCString& )),
	    this, SLOT( checkOCRBinary( const QCString& )));

   QToolTip::add( entryOcrBinary,
		  i18n( "Enter the path to gocr, the optical-character-recognition command line tool."));
   KSeparator* f2 = new KSeparator( KSeparator::HLine, page);
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
   sliderGrayLevel = new KScanSlider( page , i18n("Gray level"), 0, 254 );
   sliderLayout->addWidget( sliderGrayLevel,1 );
   int numdefault = conf->readNumEntry( CFG_GOCR_GRAYLEVEL, 160 );
   sliderGrayLevel->slSetSlider( numdefault );
   QToolTip::add( sliderGrayLevel,
		  i18n( "The numeric value gray pixels are \nconsidered to be black.\n\nDefault is 160"));
		 
   sliderDustSize = new KScanSlider( page, i18n("Dust size" ), 0, 60 );
   numdefault = conf->readNumEntry( CFG_GOCR_DUSTSIZE, 10 );
   sliderLayout->addWidget( sliderDustSize,1 );
   sliderDustSize->slSetSlider( numdefault );
   QToolTip::add( sliderDustSize,
		  i18n( "Clusters smaller than this values\nwill be considered to be dust and \nremoved from the image.\n\nDefault is 10")); 
		 
   sliderSpace = new KScanSlider( page, i18n( "Space width" ), 0, 60 );
   numdefault = conf->readNumEntry( CFG_GOCR_SPACEWIDTH, 0 );
   sliderLayout->addWidget( sliderSpace,1 );
   sliderSpace->slSetSlider( numdefault );
   QToolTip::add( sliderSpace, i18n("Spacing between characters.\n\nDefault is 0 what means autodetection"));
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

   conf->writeEntry( CFG_GOCR_BINARY, QString(getOCRCmd()));
   conf->writeEntry( CFG_GOCR_GRAYLEVEL, getGraylevel());
   conf->writeEntry( CFG_GOCR_DUSTSIZE, getDustsize());
   conf->writeEntry( CFG_GOCR_SPACEWIDTH, getSpaceWidth());
}

void KOCRStartDialog::checkOCRBinary( const QCString& cmd )
{
   checkOCRBinIntern( cmd, true );
   entryOcrBinary->setFocus();
}

void KOCRStartDialog::checkOCRBinaryShort( const QCString& cmd )
{
   checkOCRBinIntern( cmd, false);
}

void KOCRStartDialog::checkOCRBinIntern( const QCString& cmd, bool show_msg )
{
   bool ret = true;

   QFileInfo fi( cmd );

   if( ! fi.exists() )
   {
      if( show_msg )
      KMessageBox::sorry( this, i18n( "The path does not lead to the gocr-binary.\n"
				      "Please check your installation and/or install gocr."),
			  i18n("OCR software not found") );
      ret = false;
   }
   else
   {
      /* File exists, check if not dir and executable */
      if( fi.isDir() || (! fi.isExecutable()) )
      {
	 if( show_msg )
	    KMessageBox::sorry( this, i18n( "The gocr exists, but is not executable.\n"
					    "Please check your installation and/or install gocr properly."),
				i18n("OCR software not executable") );
	 ret = false;
      }
   }

   enableButton( User1, ret );
}


void KOCRStartDialog::disableFields( void )
{
   kdDebug(28000) << "About to disable the entry fields" << endl;
   sliderGrayLevel->setEnabled( false );
   sliderDustSize->setEnabled( false );
   sliderSpace->setEnabled( false );
   entryOcrBinary->setEnabled( false );

   ani->start();
}

QCString KOCRStartDialog::tryFindGocr( void ) const
{
   QStrList locations;
   QCString res = "";
   
   locations.append( "/usr/bin/gocr" );
   locations.append( "/bin/gocr" );
   locations.append( "/usr/X11R6/bin/gocr" );
   locations.append( "/usr/local/bin/gocr" );
   
   for( QCString loc = locations.first(); loc != 0; loc=locations.next())
   {
      QFileInfo fi( loc );
      if( fi.exists() && fi.isExecutable() && !fi.isDir())
      {
	 res = loc;
	 break;
      }
   }

   return( res );
}

/* The End ;) */
#include "kocrstartdia.moc"
