/***************************************************************************
                  kookapref.cpp  -  Kookas preferences dialog
                             -------------------
    begin                : Wed Jan 5 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
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
#include <qtooltip.h>
#include <qvgroupbox.h>
#include <qgrid.h>
#include <qcheckbox.h>

#include <devselector.h>
#include "config.h"
#include "thumbview.h"
#include "imageselectline.h"
#include "kscanslider.h"
#include "ksaneocr.h"

#include <kmessagebox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <kurlrequester.h>

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
    setupOCRPage();
}

void KookaPreferences::setupOCRPage()
{
    konf->setGroup( CFG_GROUP_OCR_DIA );

    QFrame *page = addPage( i18n("OCR"), i18n("Optical Character Recognition" ),
			    BarIcon("ocrImage", KIcon::SizeMedium ) );

    QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );

    /*
     * Switch ocr engines
     */
    QButtonGroup *engGroup = new QButtonGroup( 1, Qt::Horizontal, i18n("OCR Engine to Use"), page );
    m_gocrBut   = new QRadioButton( i18n("GOCR engine")  , engGroup );
    m_kadmosBut = new QRadioButton( i18n("KADMOS engine"), engGroup );

    top->addWidget( engGroup );

     QString eng = konf->readEntry(CFG_OCR_ENGINE);
#ifdef HAVE_KADMOS
     if( eng == QString("kadmos") )
     {
         m_kadmosBut->setChecked( true );
         m_useKadmos = true;
     }
     else
#endif
     {
         m_gocrBut->setChecked( true );
         m_useKadmos = false;
     }
    /*
     * GOCR Option Box
     */
    QVGroupBox *gp = new QVGroupBox( i18n("GOCR OCR"), page );

    QHBox *hbox = new QHBox( gp );
    (void) new QLabel( i18n("Select the gocr binary to use:"), hbox );
    m_urlReq = new KURLRequester( hbox );
    m_urlReq->setMode( KFile::File | KFile::ExistingOnly | KFile::LocalOnly );

    QString res = konf->readPathEntry( CFG_GOCR_BINARY, "notFound" );
    if( res == "notFound" )
    {
        res = tryFindGocr();

        if( res.isEmpty() )
        {
            /* Still not found */
            KMessageBox::sorry( this, i18n( "Could not find the gocr binary.\n"
                                            "Please check your installation and/or "
                                            "install gocr or adjust the path to "
                                            "gocr manually."),
                                i18n("OCR Software not Found") );
            m_gocrBut->setEnabled(false);
        }
    }
    m_urlReq->setURL( res );

    connect( m_urlReq, SIGNAL( textChanged( const QString& )),
             this, SLOT( checkOCRBinaryShort( const QString& )));
    connect( m_urlReq, SIGNAL( returnPressed( const QString& )),
             this, SLOT( checkOCRBinary( const QString& )));

#if 0
    connect( m_entryOCRBin, SIGNAL(valueChanged( const QCString& )),
             this, SLOT( checkOCRBinaryShort( const QCString& )));
    connect( m_entryOCRBin, SIGNAL(returnPressed( const QCString& )),
             this, SLOT( checkOCRBinary( const QCString& )));
#endif
    QToolTip::add( m_urlReq,
                   i18n( "Enter the path to gocr, the optical-character-recognition command line tool."));
    top->addWidget( gp );

    /*
     * Global Kadmos Options
     */
    QVGroupBox *kgp = new QVGroupBox( i18n("KADMOS OCR"), page );

#ifdef HAVE_KADMOS
    (void) new QLabel( i18n("The KADMOS OCR engine is available"), kgp);
#else
    (void) new QLabel( i18n("The KADMOS OCR engine is not available in this version of Kooka"), kgp );
    m_kadmosBut->setChecked(false);
    m_kadmosBut->setEnabled(false);
#endif
    top->addWidget( kgp );
    QWidget *spaceEater = new QWidget( page );
    spaceEater->setSizePolicy( QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ));
    top->addWidget( spaceEater );

}


QCString KookaPreferences::tryFindGocr( void )
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


void KookaPreferences::checkOCRBinary( const QString& cmd )
{
   checkOCRBinIntern( cmd, true );
   m_urlReq->setFocus();
}

void KookaPreferences::checkOCRBinaryShort( const QString& cmd )
{
   checkOCRBinIntern( cmd, false);
}

void KookaPreferences::checkOCRBinIntern( const QString& cmd, bool show_msg )
{
   bool ret = true;

   QFileInfo fi( cmd );

   if( ! fi.exists() )
   {
      if( show_msg )
      KMessageBox::sorry( this, i18n( "The path does not lead to the gocr-binary.\n"
				      "Please check your installation and/or install gocr."),
			  i18n("OCR Software not Found") );
      ret = false;
   }
   else
   {
      /* File exists, check if not dir and executable */
      if( fi.isDir() || (! fi.isExecutable()) )
      {
	 if( show_msg )
	    KMessageBox::sorry( this, i18n( "gocr exists, but is not executable.\n"
					    "Please check your installation and/or install gocr properly."),
				i18n("OCR Software not Executable") );
	 ret = false;
      }
   }

   m_gocrBut->setEnabled( ret );

   enableButton( User1, ret );
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
   QFrame *page = addPage( i18n("Image Saving"), i18n("Configure the Image Save Assistant" ),
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
   if( bgImg.isEmpty() )
      bgImg = stdDir.findResource( "data", STD_TILE_IMG );

   /* image file selector */
   QVGroupBox *hgb1 = new QVGroupBox( i18n("Thumbview Background" ), page );
   m_tileSelector = new ImageSelectLine( hgb1, i18n("Select background image:"));
   kdDebug(29000) << "Setting tile url " << bgImg << endl;
   m_tileSelector->setURL( KURL(bgImg) );

   top->addWidget( hgb1 );

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
   m_thumbWidth->setMinValue(1);
   l1->setBuddy( m_thumbWidth );

   lGrid->setSpacing( 4 );
   l1 = new QLabel( i18n("Thumbnail maximum &height:"), lGrid );
   m_thumbHeight = new KIntNumInput( m_thumbWidth, h, lGrid );
   m_thumbHeight->setMinValue(1);
   l1->setBuddy( m_thumbHeight );

   /* Frame Stuff */
   int frameWidth = konf->readNumEntry( THUMB_MARGIN, 3 );
   QColor col1    = konf->readColorEntry( MARGIN_COLOR1, &(colorGroup().base()));
   QColor col2    = konf->readColorEntry( MARGIN_COLOR2, &(colorGroup().foreground()));

   QGrid *fGrid = new QGrid( 2, hgb3 );
   fGrid->setSpacing( 2 );
   l1 = new QLabel(i18n("Thumbnail &frame width:"), fGrid );
   m_frameWidth = new KIntNumInput( frameWidth, fGrid );
   m_frameWidth->setMinValue(0);
   l1->setBuddy( m_frameWidth );

   l1 = new QLabel(i18n("Frame color &1: "), fGrid );
   m_colButt1 = new KColorButton( col1, fGrid );
   l1->setBuddy( m_colButt1 );

   l1 = new QLabel(i18n("Frame color &2: "), fGrid );
   m_colButt2 = new KColorButton( col2, fGrid );
   l1->setBuddy( m_colButt2 );
   /* TODO: Gradient type */

   top->addWidget( hgb2, 10);
   top->addWidget( hgb3, 10);
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

    /* ** OCR Options ** */
    konf->setGroup( CFG_GROUP_OCR_DIA );
    QString eng( "gocr" );

#ifdef HAVE_KADMOS
    bool kadmosChecked = m_kadmosBut->isChecked();

    if( kadmosChecked != m_useKadmos )
    {
        // selection of the ocr engine has changed. Popup button.
        KMessageBox::sorry( this, i18n( "The OCR engine settings were changed.\n"
                                        "Note that Kooka needs to be restarted to change the OCR engine."),
                            i18n("OCR Engine Change") );

    }
#endif
    if( m_kadmosBut->isChecked() )
        eng = "kadmos";
    konf->writeEntry(CFG_OCR_ENGINE, eng );

    konf->sync();

    emit dataSaved();
}

void KookaPreferences::slotDefault( void )
{
    cbNetQuery->setChecked( true );
    cbShowScannerSelection->setChecked( true);
    cbReadStartupImage->setChecked( true);
    cbSkipFormatAsk->setChecked( true  );
    KStandardDirs stdDir;
    QString bgImg = stdDir.findResource( "data", STD_TILE_IMG );
    m_tileSelector->setURL( KURL(bgImg) );
    m_thumbWidth->setValue( 100 );
    m_thumbHeight->setValue( 120 );
    QColor col1    = QColor( colorGroup().base());
    QColor col2    = QColor( colorGroup().foreground());

    m_frameWidth->setValue( 3 );
    m_colButt1->setColor( col1 );
    m_colButt2->setColor( col2 );
    m_gocrBut->setChecked(true);
}



#include "kookapref.moc"

