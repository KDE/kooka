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
#include <qpushbutton.h>

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
#include <kapplication.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kseparator.h>
#include <kicontheme.h>

#include "devselector.h"
#include "imgsaver.h"

#include "thumbview.h"
#include "imageselectline.h"
#include "formatdialog.h"

#include "ocrgocrengine.h"
#include "ocrocradengine.h"
#include "ocrkadmosengine.h"

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
    engineCB->insertItem(OcrEngine::engineName(OcrEngine::EngineNone),OcrEngine::EngineNone);
    engineCB->insertItem(OcrEngine::engineName(OcrEngine::EngineGocr),OcrEngine::EngineGocr);
    engineCB->insertItem(OcrEngine::engineName(OcrEngine::EngineOcrad),OcrEngine::EngineOcrad);
    engineCB->insertItem(OcrEngine::engineName(OcrEngine::EngineKadmos),OcrEngine::EngineKadmos);

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

    originalEngine = static_cast<OcrEngine::EngineType>(konf->readNumEntry(CFG_OCR_ENGINE2,OcrEngine::EngineNone));
    engineCB->setCurrentItem(originalEngine);
    slotEngineSelected(originalEngine);
}


void KookaPref::slotEngineSelected(int i)
{
    selectedEngine = static_cast<OcrEngine::EngineType>(i);
    kdDebug(28000) << k_funcinfo << "engine=" << selectedEngine << endl;

    QString msg;
    switch (selectedEngine)
    {
case OcrEngine::EngineNone:
        binaryReq->setEnabled(false);
        binaryReq->clear();
        msg = i18n("No OCR engine is selected. Select and configure one to perform OCR.");
        break;

case OcrEngine::EngineGocr:
        binaryReq->setEnabled(true);
        binaryReq->setURL(tryFindGocr());
        msg = OcrGocrEngine::engineDesc();
        break;

case OcrEngine::EngineOcrad:
        binaryReq->setEnabled(true);
        binaryReq->setURL(tryFindOcrad());
        msg = OcrOcradEngine::engineDesc();
        break;

case OcrEngine::EngineKadmos:
        binaryReq->setEnabled(false);
        binaryReq->clear();
        msg = OcrKadmosEngine::engineDesc();
        break;

default:
        binaryReq->setEnabled(false);
        binaryReq->clear();

        msg = i18n("Unknown engine %1!").arg(selectedEngine);
        break;
    }

    ocrDesc->setText(msg);
}


QString tryFindBinary(const QString &bin,const QString &configKey)
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


QString KookaPref::tryFindGocr( void )
{
   return( tryFindBinary( "gocr", CFG_GOCR_BINARY ) );
}


QString KookaPref::tryFindOcrad( void )
{
   return( tryFindBinary( "ocrad", CFG_OCRAD_BINARY ) );
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
    QFrame *page = addPage( i18n("General"), i18n("General Options" ),
			    BarIcon("configure", KIcon::SizeMedium ) );
    QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );

    /* Description-Label */
    top->addWidget(new QLabel(i18n("These options will take effect immediately."),page));
    top->addSpacing(2*KDialogBase::spacingHint());

    /* Gallery options */
    QVGroupBox *gg = new QVGroupBox(i18n("Image Gallery"),page);

    konf->setGroup(GROUP_GALLERY);

    /* Layout */
    QHBox *hb1 = new QHBox(gg);
    QLabel *lab = new QLabel(i18n("Show recent folders: "),hb1);

    layoutCB = new KComboBox(hb1);
    layoutCB->insertItem(i18n("Not shown"),KookaGallery::NoRecent);
    layoutCB->insertItem(i18n("At top"),KookaGallery::RecentAtTop);
    layoutCB->insertItem(i18n("At bottom"),KookaGallery::RecentAtBottom);
    layoutCB->setCurrentItem(konf->readNumEntry(GALLERY_LAYOUT,KookaGallery::RecentAtTop));
    lab->setBuddy(layoutCB);
    hb1->setStretchFactor(layoutCB,1);

    /* Allow renaming */
    cbAllowRename = new QCheckBox(i18n("Allow click-to-rename"),gg);
    QToolTip::add( cbAllowRename,
		   i18n( "Check this if you want to be able to rename gallery items by clicking on them "
                         "(otherwise, use the \"Rename\" menu option)"));
    cbAllowRename->setChecked(konf->readBoolEntry(GALLERY_ALLOW_RENAME,false));

    top->addWidget(gg);

    top->addSpacing(2*KDialogBase::marginHint());

    /* Enable messages and questions */
    QLabel *l = new QLabel(i18n("Use this button to reenable all messages and questions which\nhave been hidden by using \"Don't ask me again\"."),page);
    top->addWidget(l);

    pbEnableMsgs = new QPushButton(i18n("Enable Messages/Questions"),page);
    connect(pbEnableMsgs,SIGNAL(clicked()),SLOT(slotEnableWarnings()));
    top->addWidget(pbEnableMsgs,0,Qt::AlignLeft);

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
    top->addWidget(new QLabel(i18n("These options will take effect when Kooka is next started."),page));
    top->addSpacing(2*KDialogBase::spacingHint());

    /* Query for network scanner (Checkbox) */
    cbNetQuery = new QCheckBox( i18n("Query network for available scanners"),
				page);
    QToolTip::add( cbNetQuery,
		   i18n( "Check this if you want a network query for available scanners.\nNote that this does not mean a query over the entire network but only the stations configured for SANE!" ));
    cbNetQuery->setChecked( ! (konf->readBoolEntry( STARTUP_ONLY_LOCAL, false )) );


    /* Show scanner selection box on startup (Checkbox) */
    cbShowScannerSelection = new QCheckBox( i18n("Show the scanner selection dialog"),page);
    QToolTip::add( cbShowScannerSelection,
		   i18n( "Check this if you once checked 'do not show the scanner selection on startup',\nbut you want to see it again." ));

    cbShowScannerSelection->setChecked( !konf->readBoolEntry( STARTUP_SKIP_ASK, false ));

    /* Read startup image on startup (Checkbox) */
    cbReadStartupImage = new QCheckBox( i18n("Load the last selected image into the viewer"),page);
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
   QFrame *page = addPage( i18n("Image Saving"), i18n("Image Saving Options" ),
			    BarIcon("filesave", KIcon::SizeMedium ) );
   QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );
   top->addSpacing(KDialogBase::spacingHint());

   /* Skip the format asking if a format entry  exists */
   cbSkipFormatAsk = new QCheckBox( i18n("Always use the Save Assistant"),
				     page);
   cbSkipFormatAsk->setChecked( konf->readBoolEntry( OP_FILE_ASK_FORMAT, true  ));
   QToolTip::add( cbSkipFormatAsk, i18n("Check this if you want to see the image save assistant even if there is a default format for the image type." ));
   top->addWidget( cbSkipFormatAsk );

   cbFilenameAsk = new QCheckBox( i18n("Ask for filename when saving"),
                    page);
   cbFilenameAsk->setChecked( konf->readBoolEntry( OP_ASK_FILENAME, false));
   QToolTip::add( cbFilenameAsk, i18n("Check this if you want to enter a filename when an image has been scanned." ));
   top->addWidget( cbFilenameAsk );

   top->addStretch(10);
}

void KookaPref::setupThumbnailPage()
{
    konf->setGroup(THUMB_GROUP);

    QFrame *page = addPage( i18n("Thumbnail View"), i18n("Thumbnail Gallery View" ),
			    BarIcon("thumbnail", KIcon::SizeMedium ) );
    QGridLayout *lay = new QGridLayout(page,5,2,0,
                                       KDialog::spacingHint());
    lay->setRowStretch(4,9);
    lay->setColStretch(1,9);

    QLabel *title = new QLabel(i18n("Here you can configure the appearance of the scan gallery thumbnail view."),page);
    lay->addMultiCellWidget(title,0,0,0,2);
    lay->setRowSpacing(1,2*KDialogBase::spacingHint());

    /* Background image */
    QString bgImg = konf->readPathEntry(THUMB_BG_WALLPAPER,ThumbView::standardBackground());

    QLabel *l = new QLabel(i18n("Background:"),page);
    lay->addWidget(l,2,0,Qt::AlignRight);

    /* image file selector */
    m_tileSelector = new ImageSelectLine(page,QString::null);
    kdDebug(28000) << "Setting tile url " << bgImg << endl;
    m_tileSelector->setURL(bgImg);
    lay->addWidget(m_tileSelector,2,1);
    l->setBuddy(m_tileSelector);

    /* Preview size */
    l = new QLabel(i18n("Preview size:"),page);
    lay->addWidget(l,3,0,Qt::AlignRight);

    m_thumbSizeCb = new KComboBox(page);
    m_thumbSizeCb->insertItem(ThumbView::sizeName(KIcon::SizeEnormous));	// 0
    m_thumbSizeCb->insertItem(ThumbView::sizeName(KIcon::SizeHuge));		// 1
    m_thumbSizeCb->insertItem(ThumbView::sizeName(KIcon::SizeLarge));		// 2
    m_thumbSizeCb->insertItem(ThumbView::sizeName(KIcon::SizeMedium));		// 3
    m_thumbSizeCb->insertItem(ThumbView::sizeName(KIcon::SizeSmallMedium));	// 4

    KIcon::StdSizes size = static_cast<KIcon::StdSizes>(konf->readNumEntry(THUMB_PREVIEW_SIZE,
                                                                           KIcon::SizeHuge));
    int sel;
    switch (size)
    {
case KIcon::SizeEnormous:	sel = 0;	break;
default:
case KIcon::SizeHuge:		sel = 1;	break;
case KIcon::SizeLarge:		sel = 2;	break;
case KIcon::SizeMedium:		sel = 3;	break;
case KIcon::SizeSmallMedium:	sel = 4;	break;
    }
    m_thumbSizeCb->setCurrentItem(sel);

    lay->addWidget(m_thumbSizeCb,3,1);
    l->setBuddy(m_thumbSizeCb);
}


void KookaPref::slotOk( void )
{
  slotApply();
  accept();
}


void KookaPref::slotApply( void )
{
   /* ** general - gallery options ** */
   konf->setGroup(GROUP_GALLERY);
   konf->writeEntry(GALLERY_ALLOW_RENAME,galleryAllowRename());
   konf->writeEntry(GALLERY_LAYOUT,galleryLayout());

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
    konf->setGroup(THUMB_GROUP);

    KURL bgUrl = m_tileSelector->selectedURL().url();
    bgUrl.setProtocol("");
    kdDebug(28000) << "Writing tile-pixmap " << bgUrl.prettyURL() << endl;
    konf->writePathEntry(THUMB_BG_WALLPAPER,bgUrl.url());

    KIcon::StdSizes size;
    switch (m_thumbSizeCb->currentItem())
    {
case 0:		size = KIcon::SizeEnormous;	break;
default:
case 1:		size = KIcon::SizeHuge;		break;
case 2:		size = KIcon::SizeLarge;	break;
case 3:		size = KIcon::SizeMedium;	break;
case 4:		size = KIcon::SizeSmallMedium;	break;
    }
    konf->writeEntry(THUMB_PREVIEW_SIZE,size);

    /* ** OCR Options ** */
    konf->setGroup( CFG_GROUP_OCR_DIA );
    konf->writeEntry(CFG_OCR_ENGINE2,selectedEngine);

    QString path = binaryReq->url();
    if (!path.isEmpty())
    {
        switch (selectedEngine)
        {
case OcrEngine::EngineGocr:
            if (checkOCRBin(path,"gocr",true)) konf->writePathEntry(CFG_GOCR_BINARY,path);
            break;

case OcrEngine::EngineOcrad:
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
    layoutCB->setCurrentItem(KookaGallery::RecentAtTop);

    cbNetQuery->setChecked( true );
    cbShowScannerSelection->setChecked( true);
    cbReadStartupImage->setChecked( true);
    cbSkipFormatAsk->setChecked( true  );

    m_tileSelector->setURL(ThumbView::standardBackground());
    m_thumbSizeCb->setCurrentItem(1);			// "Very Large"

    slotEngineSelected(OcrEngine::EngineNone);
}


bool KookaPref::galleryAllowRename() const
{
    return (cbAllowRename->isChecked());
}


KookaGallery::Layout KookaPref::galleryLayout() const
{
    return (static_cast<KookaGallery::Layout>(layoutCB->currentItem()));
}


void KookaPref::slotEnableWarnings()
{
    kdDebug(28000) << k_funcinfo << endl;
    KMessageBox::enableAllMessages();
    FormatDialog::forgetRemembered();
    kapp->config()->reparseConfiguration();

    pbEnableMsgs->setEnabled(false);			// show this has been done
}
