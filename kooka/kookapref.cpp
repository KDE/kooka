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

#include <qlayout.h>
#include <qtooltip.h>
#include <qvgroupbox.h>
#include <qgrid.h>
#include <qcheckbox.h>
#include <qlabel.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <knuminput.h>
#include <kcolorbutton.h>
#include <kstandarddirs.h>
#include <kcombobox.h>
#include <kactivelabel.h>

#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kseparator.h>

#include "devselector.h"
#include "imgsaver.h"

#include "thumbview.h"
#include "imageselectline.h"

#include "ksaneocr.h"
#include "kocrgocr.h"
#include "kocrocrad.h"
#include "kocrkadmos.h"

#include "kookapref.h"
#include "kookapref.moc"


KookaPref::KookaPref()
    : KDialogBase(KDialogBase::IconList, i18n("Preferences"),
                  KDialogBase::Help|KDialogBase::Default|KDialogBase::Ok|KDialogBase::Apply|KDialogBase::Cancel,
                  KDialogBase::Ok )
{
    // this is the base class for your preferences dialog.  it is now
    // a Treelist dialog.. but there are a number of other
    // possibilities (including Tab, Swallow, and just Plain)
    konf = KGlobal::config ();

    setupGeneralPage();
    setupStartupPage();
    setupSaveFormatPage();
    setupThumbnailPage();
    setupOCRPage();
}

void KookaPref::setupOCRPage()
{
    konf->setGroup( CFG_GROUP_OCR_DIA );

    QFrame *page = addPage( i18n("OCR"), i18n("Optical Character Recognition" ),
			    BarIcon("ocr", KIcon::SizeMedium ) );

    QGridLayout *lay = new QGridLayout(page,7,2,KDialog::marginHint(),
                                       KDialog::spacingHint());
    lay->setRowStretch(6,9);
    lay->setColStretch(1,9);

    engineCB = new KComboBox(page);
    engineCB->insertItem(KSaneOcr::engineName(KSaneOcr::OcrNone),KSaneOcr::OcrNone);
    engineCB->insertItem(KSaneOcr::engineName(KSaneOcr::OcrGocr),KSaneOcr::OcrGocr);
    engineCB->insertItem(KSaneOcr::engineName(KSaneOcr::OcrOcrad),KSaneOcr::OcrOcrad);
    engineCB->insertItem(KSaneOcr::engineName(KSaneOcr::OcrKadmos),KSaneOcr::OcrKadmos);

    connect(engineCB,SIGNAL(activated(int)),SLOT(slotEngineSelected(int)));
    lay->addWidget(engineCB,0,1);

    QLabel *lab = new QLabel(i18n("OCR Engine:"),page);
    lab->setBuddy(engineCB);
    lay->addWidget(lab,0,0,Qt::AlignRight);

    lay->setRowSpacing(1,KDialog::marginHint());

    binaryReq = new KURLRequester(page);
    binaryReq->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    lay->addWidget(binaryReq,2,1);

    lab = new QLabel(i18n("Engine executable:"),page);
    lab->setBuddy(binaryReq);
    lay->addWidget(lab,2,0,Qt::AlignRight);

    lay->setRowSpacing(3,KDialog::marginHint());

    KSeparator *sep = new KSeparator(KSeparator::HLine,page);
    lay->addMultiCellWidget(sep,4,4,0,1);

    lay->setRowSpacing(5,KDialog::marginHint());

    ocrDesc = new KActiveLabel("?",page);
    lay->addMultiCellWidget(ocrDesc,6,6,0,1);

    originalEngine = static_cast<KSaneOcr::OcrEngine>(konf->readNumEntry(CFG_OCR_ENGINE2,KSaneOcr::OcrNone));
    engineCB->setCurrentItem(originalEngine);
    slotEngineSelected(originalEngine);
}


void KookaPref::slotEngineSelected(int i)
{
    selectedEngine = static_cast<KSaneOcr::OcrEngine>(i);
    kdDebug(29000) << k_funcinfo << "engine=" << selectedEngine << endl;

    QString msg;
    switch (selectedEngine)
    {
case KSaneOcr::OcrNone:
        binaryReq->setEnabled(false);
        binaryReq->clear();
        msg = i18n("No OCR engine is selected. Select and configure one to perform OCR.");
        break;

case KSaneOcr::OcrGocr:
        binaryReq->setEnabled(true);
        binaryReq->setURL(tryFindGocr());
        msg = KGOCRDialog::engineDesc();
        break;

case KSaneOcr::OcrOcrad:
        binaryReq->setEnabled(true);
        binaryReq->setURL(tryFindOcrad());
        msg = ocradDialog::engineDesc();
        break;

case KSaneOcr::OcrKadmos:
        binaryReq->setEnabled(false);
        binaryReq->clear();
        msg = KadmosDialog::engineDesc();
        break;

default:
        binaryReq->setEnabled(false);
        binaryReq->clear();

        msg = i18n("Unknown engine %1!").arg(selectedEngine);
        break;
    }

    ocrDesc->setText(msg);
}


QString KookaPref::tryFindGocr( void )
{
   return( tryFindBinary( "gocr", CFG_GOCR_BINARY ) );
}


QString KookaPref::tryFindOcrad( void )
{
   return( tryFindBinary( "ocrad", CFG_OCRAD_BINARY ) );
}


QString KookaPref::tryFindBinary(const QString &bin,const QString &configKey)
{
    KConfig *cfg = KGlobal::config();

    /* First check the config files for an entry */
    cfg->setGroup(CFG_GROUP_OCR_DIA);
    QString exe = cfg->readPathEntry(configKey);	// try from config file

    // Why do we do the second test here?  checkOCRBin() does the same, why also?
    if (!exe.isEmpty() && exe.contains(bin))
    {
        QFileInfo fi(exe);				// check for valid executable
        if (fi.exists() && fi.isExecutable() && !fi.isDir()) return (exe);
    }

    /* Otherwise find the program on the user's search path */
    return (KGlobal::dirs()->findExe(bin));		// search using $PATH
}


bool KookaPref::checkOCRBin(const QString &cmd,const QString &bin,bool show_msg)
{
    // Why do we do this test?  See above.
    if (!cmd.contains(bin)) return (false);

    QFileInfo fi(cmd);
    if (!fi.exists())
    {
        if (show_msg) KMessageBox::sorry(this,i18n("<qt>"
                                                   "The path <b>%1</b> does not lead to a valid binary.\n"
                                                   "Please check the path and and install the program if necessary.").arg(cmd),
                                         i18n("OCR Engine Not Found"));
        return (false);
    }
    else
    {
        /* File exists, check if not dir and executable */
        if (fi.isDir() || (!fi.isExecutable()))
        {
            if (show_msg) KMessageBox::sorry(this,i18n("<qt>"
                                                       "The program <b>%1</b> exists, but is not executable.\n"
                                                       "Please check the path and permissions, and/or reinstall the program if necessary.").arg(cmd),
                                             i18n("OCR Engine Not Executable"));
            return (false);
        }
    }

    return (true);
}



void KookaPref::setupGeneralPage()
{
    konf->setGroup( GROUP_GENERAL );

    QFrame *page = addPage( i18n("General"), i18n("General Options" ),
			    BarIcon("configure", KIcon::SizeMedium ) );
    QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );
    top->addSpacing(KDialogBase::spacingHint());

    /* Allow renaming in gallery */
    cbAllowRename = new QCheckBox( i18n("Click-to-rename in gallery"),
				page);
    QToolTip::add( cbAllowRename,
		   i18n( "Check this if you want to be able to rename gallery items by clicking on them "
                         "(otherwise, use the \"Rename\" menu option)"));
    cbAllowRename->setChecked(konf->readBoolEntry(GENERAL_ALLOW_RENAME,false));

    top->addWidget(cbAllowRename);

    top->addStretch(10);
}



void KookaPref::setupStartupPage()
{

    /* startup options */
    konf->setGroup( GROUP_STARTUP );

    QFrame *page = addPage( i18n("Startup"), i18n("Startup Options" ),
			    BarIcon("gear", KIcon::SizeMedium ) );
    QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );
    /* Description-Label */
    top->addWidget( new QLabel( i18n("Note that changing these options will affect Kooka's next start!"), page ));
    top->addSpacing(KDialogBase::spacingHint());

    /* Query for network scanner (Checkbox) */
    cbNetQuery = new QCheckBox( i18n("Query network for available scanners"),
				page);
    QToolTip::add( cbNetQuery,
		   i18n( "Check this if you want a network query for available scanners.\nNote that this does not mean a query over the entire network but only the stations configured for SANE!" ));
    cbNetQuery->setChecked( ! (konf->readBoolEntry( STARTUP_ONLY_LOCAL, false )) );


    /* Show scanner selection box on startup (Checkbox) */
    cbShowScannerSelection = new QCheckBox( i18n("Show the scanner selection box on next startup"),
					    page);
    QToolTip::add( cbShowScannerSelection,
		   i18n( "Check this if you once checked 'do not show the scanner selection on startup',\nbut you want to see it again." ));

    cbShowScannerSelection->setChecked( !konf->readBoolEntry( STARTUP_SKIP_ASK, false ));

    /* Read startup image on startup (Checkbox) */
    cbReadStartupImage = new QCheckBox( i18n("Load the last image into the viewer on startup"),
					    page);
    QToolTip::add( cbReadStartupImage,
		   i18n( "Check this if you want Kooka to load the last selected image into the viewer on startup.\nIf your images are large, that might slow down Kooka's start." ));
    cbReadStartupImage->setChecked( konf->readBoolEntry( STARTUP_READ_IMAGE, true));

    /* -- */

    top->addWidget( cbNetQuery );
    top->addWidget( cbShowScannerSelection );
    top->addWidget( cbReadStartupImage );

    top->addStretch(10);

}



void KookaPref::setupSaveFormatPage( )
{
   konf->setGroup( OP_FILE_GROUP );
   QFrame *page = addPage( i18n("Image Saving"), i18n("Configure Image Saving" ),
			    BarIcon("filesave", KIcon::SizeMedium ) );
   QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );

   /* Skip the format asking if a format entry  exists */
   cbSkipFormatAsk = new QCheckBox( i18n("Always display image save assistant"),
				     page);
   cbSkipFormatAsk->setChecked( konf->readBoolEntry( OP_FILE_ASK_FORMAT, true  ));
   QToolTip::add( cbSkipFormatAsk, i18n("Check this if you want to see the image save assistant even if there is a default format for the image type." ));
   top->addWidget( cbSkipFormatAsk );

   cbFilenameAsk = new QCheckBox( i18n("Ask for filename when saving file"),
                    page);
   cbFilenameAsk->setChecked( konf->readBoolEntry( OP_ASK_FILENAME, false));
   QToolTip::add( cbFilenameAsk, i18n("Check this if you want to enter a filename when an image has been scanned." ));
   top->addWidget( cbFilenameAsk );



   top->addStretch(10);
}

void KookaPref::setupThumbnailPage()
{
   konf->setGroup( THUMB_GROUP );

   QFrame *page = addPage( i18n("Thumbnail View"), i18n("Thumbnail Gallery View" ),
			    BarIcon("thumbnail", KIcon::SizeMedium ) );
   QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );

   top->addWidget( new QLabel( i18n("Here you can configure the appearance of the thumbnail view of your scan picture gallery."),page ));

   /* Backgroundimage */
   KStandardDirs stdDir;
   QString bgImg = konf->readPathEntry( BG_WALLPAPER );
   if( bgImg.isEmpty() )
      bgImg = stdDir.findResource( "data", STD_TILE_IMG );

   /* image file selector */
   QVGroupBox *hgb1 = new QVGroupBox( i18n("Thumbview Background" ), page );
   m_tileSelector = new ImageSelectLine( hgb1, i18n("Select background image:"));
   kdDebug(28000) << "Setting tile url " << bgImg << endl;
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


void KookaPref::slotOk( void )
{
  slotApply();
  accept();
}


void KookaPref::slotApply( void )
{
   /* ** general options ** */
   konf->setGroup(GROUP_GENERAL);
   konf->writeEntry(GENERAL_ALLOW_RENAME,allowGalleryRename());

    /* ** startup options ** */
   /** write the global one, to read from libkscan also */
   konf->setGroup(GROUP_STARTUP);
   bool cbVal = !(cbShowScannerSelection->isChecked());
   kdDebug(28000) << "Writing for " << STARTUP_SKIP_ASK << ": " << cbVal << endl;
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
    konf->writeEntry( OP_ASK_FILENAME, cbFilenameAsk->isChecked() );

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
    konf->writePathEntry( BG_WALLPAPER, bgUrl.url() );

    /* ** OCR Options ** */
    konf->setGroup( CFG_GROUP_OCR_DIA );
    konf->writeEntry(CFG_OCR_ENGINE2,selectedEngine);
    if (selectedEngine!=originalEngine)
    {
        // selection of the ocr engine has changed. Popup button.
        KMessageBox::information(this,i18n("The OCR engine has been changed.\n"
                                           "Kooka needs to be restarted for this "
                                           "change to take effect."),
                                 i18n("OCR Engine Changed"));
    }

    QString path = binaryReq->url();
    if (!path.isEmpty())
    {
        switch (selectedEngine)
        {
case KSaneOcr::OcrGocr:
            if (checkOCRBin(path,"gocr",true)) konf->writePathEntry(CFG_GOCR_BINARY,path);
            break;

case KSaneOcr::OcrOcrad:
            if (checkOCRBin(path,"ocrad",true)) konf->writePathEntry(CFG_OCRAD_BINARY,path);
            break;

default:    break;
        }
    }

    konf->sync();
    emit dataSaved();
}

void KookaPref::slotDefault( void )
{
    cbAllowRename->setChecked(false);
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

    slotEngineSelected(KSaneOcr::OcrNone);
}


bool KookaPref::allowGalleryRename()
{
    return (cbAllowRename->isChecked());
}
