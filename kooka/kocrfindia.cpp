/***************************************************************************
                          kocrfindia.cpp  -  description                              
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
#include <krun.h>
#include <kurl.h>
#include <ktempfile.h>
#include <qtextstream.h>
#include <qsplitter.h>

#include "img_canvas.h"
#include "resource.h"
#include "kocrfindia.h"

#include <kscanslider.h>

/* defines for konfig-reading */
#if 0
#define CFG_GROUP_OCR_DIA "ocrDialog"
#define CFG_GOCR_BINARY "gocrBinary"
#define CFG_GOCR_DUSTSIZE "gocrDustSize"
#define CFG_GOCR_GRAYLEVEL "gocrGrayLevel"
#define CFG_GOCR_SPACEWIDTH "gocrSpaceWidth"
#endif


KOCRFinalDialog::KOCRFinalDialog( QWidget *parent, QString resultimg )
   :KDialogBase( parent,  "OCRFinish", true, I18N("Optical Character Recognition finished"),
		 Close|User1, Close, true, I18N("Open in kwrite" ) )
{
   debug( "Finished KOCR!" );
   // Layout-Boxes
   QWidget *page = new QWidget( this );
   CHECK_PTR( page );
   setMainWidget( page );
   
   // Caption - Label
   QVBoxLayout *topLayout = new QVBoxLayout( page, marginHint(), spacingHint() );
   
   QLabel *label = new QLabel( I18N( "<B>Optical Character Recognition Results</B>"),
				     page, "captionImage" );
   CHECK_PTR( label );
   topLayout->addWidget( label);

   QSplitter *splitpage = new QSplitter( QSplitter::Vertical , page );
   topLayout->addWidget( splitpage );
   
   resultImg = new QImage( resultimg );
   ImageCanvas *imgCanvas = new ImageCanvas( splitpage, resultImg, "RESIMG" );
   
   CHECK_PTR( imgCanvas );
   // topLayout->addWidget( imgCanvas);

   // textEdit
   textEdit = new KEdit( splitpage, "OCRResultEditor" );
   CHECK_PTR( textEdit );
   // topLayout->addWidget( textEdit );

   connect( this, SIGNAL(user1Clicked()), this, SLOT(openTextResult()));
}

void  KOCRFinalDialog::openTextResult( void )
{
   debug( "Opening text file <%s>", (const char*) ocrTextFile );
   new KRun( KURL( ocrTextFile ));
}

void  KOCRFinalDialog::fillText( QString str )
{
   textEdit->setText( str );

   /* ... and save to temp file to open with kwrite */
   KTempFile textFile( QString::null, ".txt" );
   ocrTextFile = textFile.name();
   debug(" Sending ocr-Result-Text to file <%s>", (const char*) textFile.name() );

   QTextStream *ts = textFile.textStream();
   (*ts) << str;
   textFile.close();
   
}

KOCRFinalDialog::~KOCRFinalDialog()
{
   delete ( resultImg );
}

void KOCRFinalDialog::writeConfig( void )
{
#if 0
   KConfig *conf = KGlobal::config ();
   conf->setGroup( CFG_GROUP_OCR_DIA );
   conf->writeEntry( CFG_GOCR_BINARY, getOCRCmd());
   conf->writeEntry( CFG_GOCR_GRAYLEVEL, getGraylevel());
   conf->writeEntry( CFG_GOCR_DUSTSIZE, getDustsize());
   conf->writeEntry( CFG_GOCR_SPACEWIDTH, getSpaceWidth());
#endif
}



/* The End ;) */
