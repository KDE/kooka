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
#include "kookapref.moc"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRERROR
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <qlayout.h>
#include <qtooltip.h>
#include <q3vgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qdir.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <knuminput.h>
#include <kcolorbutton.h>
#include <kstandarddirs.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kseparator.h>
#include <kicontheme.h>

#include "libkscan/kscandevice.h"
#include "libkscan/scanglobal.h"

#include "imgsaver.h"
#include "thumbview.h"
#include "formatdialog.h"
#include "ocrgocrengine.h"
#include "ocrocradengine.h"
#include "ocrkadmosengine.h"



KookaPref::KookaPref(QWidget *parent)
    : KPageDialog(parent)
{
    setObjectName("KookaPref");

    setModal(true);
    setButtons(KDialog::Help|KDialog::Default|KDialog::Ok|KDialog::Apply|KDialog::Cancel);
    setDefaultButton(KDialog::Ok);
    setCaption(i18n("Preferences"));
    showButtonSeparator(true);

    konf = KGlobal::config().data();

    setupGeneralPage();
    setupStartupPage();
    setupSaveFormatPage();
    setupThumbnailPage();
    setupOCRPage();

    connect(this, SIGNAL(okClicked()), SLOT(slotSaveSettings()));
    connect(this, SIGNAL(applyClicked()), SLOT(slotSaveSettings()));
    connect(this, SIGNAL(defaultClicked()), SLOT(slotSetDefaults()));

    setMinimumSize(670, 380);
}



void KookaPref::setupOCRPage()
{
    KConfigGroup grp = konf->group(CFG_GROUP_OCR_DIA);

    QFrame *page = new QFrame(this);
    QGridLayout *lay = new QGridLayout(page);
    lay->setRowStretch(6, 9);
    lay->setColumnStretch(1, 9);

    engineCB = new KComboBox(page);
    engineCB->addItem(OcrEngine::engineName(OcrEngine::EngineNone), OcrEngine::EngineNone);
    engineCB->addItem(OcrEngine::engineName(OcrEngine::EngineGocr), OcrEngine::EngineGocr);
    engineCB->addItem(OcrEngine::engineName(OcrEngine::EngineOcrad), OcrEngine::EngineOcrad);
    engineCB->addItem(OcrEngine::engineName(OcrEngine::EngineKadmos), OcrEngine::EngineKadmos);

    connect(engineCB, SIGNAL(activated(int)), SLOT(slotEngineSelected(int)));
    lay->addWidget(engineCB, 0, 1);

    QLabel *lab = new QLabel(i18n("OCR Engine:"), page);
    lab->setBuddy(engineCB);
    lay->addWidget(lab, 0, 0, Qt::AlignRight);

    lay->setRowMinimumHeight(1, KDialog::marginHint());

    binaryReq = new KUrlRequester(page);
    binaryReq->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    lay->addWidget(binaryReq,2,1);

    lab = new QLabel(i18n("Engine executable:"),page);
    lab->setBuddy(binaryReq);
    lay->addWidget(lab, 2, 0, Qt::AlignRight);

    lay->setRowMinimumHeight(3, KDialog::marginHint());

    KSeparator *sep = new KSeparator(Qt::Horizontal, page);
    lay->addWidget(sep, 4, 0, 1, 2);
    lay->setRowMinimumHeight(5, KDialog::marginHint());

    ocrDesc = new QLabel("?", page);
    ocrDesc->setOpenExternalLinks(true);
    ocrDesc->setWordWrap(true);
    lay->addWidget(ocrDesc, 6, 0, 1, 2, Qt::AlignTop);

    KPageWidgetItem *item = addPage(page, i18n("OCR"));
    item->setHeader(i18n("Optical Character Recognition"));
    item->setIcon(KIcon("ocr"));

    originalEngine = static_cast<OcrEngine::EngineType>(grp.readEntry(CFG_OCR_ENGINE2, static_cast<int>(OcrEngine::EngineNone)));
    engineCB->setCurrentIndex(originalEngine);
    slotEngineSelected(originalEngine);
}


void KookaPref::slotEngineSelected(int i)
{
    selectedEngine = static_cast<OcrEngine::EngineType>(i);
    kDebug() << "engine is" << selectedEngine;

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
        binaryReq->setUrl(tryFindGocr());
        msg = OcrGocrEngine::engineDesc();
        break;

case OcrEngine::EngineOcrad:
        binaryReq->setEnabled(true);
        binaryReq->setUrl(tryFindOcrad());
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

        msg = i18n("Unknown engine %1!", selectedEngine);
        break;
    }

    ocrDesc->setText(msg);
}


QString tryFindBinary(const QString &bin,const QString &configKey)
{
    KConfigGroup grp = KGlobal::config()->group(CFG_GROUP_OCR_DIA);

    /* First check the config files for an entry */
    QString exe = grp.readPathEntry(configKey, QString::null);	// try from config file

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
                                                   "The path <filename>%1</filename> is not a valid binary.\n"
                                                   "Please check the path and and install the program if necessary.", cmd),
                                         i18n("OCR Engine Not Found"));
        return (false);
    }
    else
    {
        /* File exists, check if not dir and executable */
        if (fi.isDir() || (!fi.isExecutable()))
        {
            if (show_msg) KMessageBox::sorry(this,i18n("<qt>"
                                                       "The program <filename>%1</filename> exists, but is not executable.\n"
                                                       "Please check the path and permissions, and/or reinstall the program if necessary.", cmd),
                                             i18n("OCR Engine Not Executable"));
            return (false);
        }
    }

    return (true);
}



void KookaPref::setupGeneralPage()
{
    KConfigGroup grp = konf->group(GROUP_GALLERY);

    QFrame *page = new QFrame(this);
    QVBoxLayout *top = new QVBoxLayout(page);

    /* Description-Label */
    top->addWidget(new QLabel(i18n("These options will take effect immediately."), page));
    top->addSpacing(2*KDialog::spacingHint());

    /* Gallery options */
    Q3VGroupBox *gg = new Q3VGroupBox(i18n("Image Gallery"),page);

    /* Layout */
    KHBox *hb1 = new KHBox(gg);
    QLabel *lab = new QLabel(i18n("Show recent folders: "), hb1);

    layoutCB = new KComboBox(hb1);
    layoutCB->addItem(i18n("Not shown"), KookaGallery::NoRecent);
    layoutCB->addItem(i18n("At top"), KookaGallery::RecentAtTop);
    layoutCB->addItem(i18n("At bottom"), KookaGallery::RecentAtBottom);
    layoutCB->setCurrentIndex(grp.readEntry(GALLERY_LAYOUT, static_cast<int>(KookaGallery::RecentAtTop)));
    lab->setBuddy(layoutCB);
    hb1->setStretchFactor(layoutCB, 1);

    /* Allow renaming */
    cbAllowRename = new QCheckBox(i18n("Allow click-to-rename"), gg);
    cbAllowRename->setToolTip(i18n("Check this if you want to be able to rename gallery items by clicking on them (otherwise, use the \"Rename\" menu option)"));
    cbAllowRename->setChecked(grp.readEntry(GALLERY_ALLOW_RENAME, false));

    top->addWidget(gg);

    top->addSpacing(2*KDialog::marginHint());

    /* Enable messages and questions */
    lab = new QLabel(i18n("Use this button to reenable all messages and questions which\nhave been hidden by using \"Don't ask me again\"."), page);
    top->addWidget(lab);

    pbEnableMsgs = new QPushButton(i18n("Enable Messages/Questions"), page);
    connect(pbEnableMsgs, SIGNAL(clicked()), SLOT(slotEnableWarnings()));
    top->addWidget(pbEnableMsgs,0,Qt::AlignLeft);

    top->addStretch(10);

    KPageWidgetItem *item = addPage(page, i18n("General"));
    item->setHeader(i18n("General Options"));
    item->setIcon(KIcon("configure"));
}



void KookaPref::setupStartupPage()
{
    const KConfigGroup grp1 = konf->group(GROUP_STARTUP);
    const KConfigGroup grp2 = ScanGlobal::self()->configGroup();

    /* startup options */
    QFrame *page = new QFrame(this);
    QVBoxLayout *top = new QVBoxLayout(page);

    /* Description-Label */
    top->addWidget(new QLabel(i18n("These options will take effect when Kooka is next started."), page));
    top->addSpacing(2*KDialog::spacingHint());

    /* Query for network scanner (Checkbox) */
    cbNetQuery = new QCheckBox(i18n("Query network for available scanners"), page);
    cbNetQuery->setToolTip(i18n("Check this if you want a network query for available scanners.\nNote that this does not mean a query over the entire network but only the stations configured for SANE."));
    cbNetQuery->setChecked(!grp2.readEntry(STARTUP_ONLY_LOCAL, false));
    top->addWidget(cbNetQuery);

    /* Show scanner selection box on startup (Checkbox) */
    cbShowScannerSelection = new QCheckBox(i18n("Show the scanner selection dialog"), page);
    cbShowScannerSelection->setToolTip(i18n("Check this to show the scanner selection dialogue on startup."));
    cbShowScannerSelection->setChecked(!grp2.readEntry(STARTUP_SKIP_ASK, false));
    top->addWidget(cbShowScannerSelection);

    /* Read startup image on startup (Checkbox) */
    cbReadStartupImage = new QCheckBox(i18n("Load the last selected image into the viewer"), page);
    cbReadStartupImage->setToolTip(i18n("Check this if you want Kooka to load the last selected image into the viewer on startup.\nIf your images are large, that might slow down Kooka's startup."));
    cbReadStartupImage->setChecked(grp1.readEntry(STARTUP_READ_IMAGE, true));
    top->addWidget(cbReadStartupImage);

    top->addStretch(10);

    KPageWidgetItem *item = addPage(page, i18n("Startup"));
    item->setHeader(i18n("Startup Options"));
    item->setIcon(KIcon("system-run"));
}



void KookaPref::setupSaveFormatPage( )
{
    KConfigGroup grp = konf->group(OP_SAVER_GROUP);

    // TODO: needs to be a QFrame, or can be a QWidget?
    QFrame *page = new QFrame(this);
    QVBoxLayout *top = new QVBoxLayout(page);

    /* Description-Label */
    top->addWidget(new QLabel(i18n("These options will take effect immediately."), page));
    top->addSpacing(2*KDialog::spacingHint());

    /* Skip the format asking if a format entry  exists */
    cbSkipFormatAsk = new QCheckBox(i18n("Always use the Save Assistant"), page);
    cbSkipFormatAsk->setChecked(grp.readEntry(OP_SAVER_ASK_FORMAT, false));
    cbSkipFormatAsk->setToolTip(i18n("Check this if you want to use the image save assistant even if there is a default format for the image type."));
    top->addWidget(cbSkipFormatAsk);

    cbFilenameAsk = new QCheckBox( i18n("Ask for filename when saving"), page);
    cbFilenameAsk->setChecked(grp.readEntry(OP_SAVER_ASK_FILENAME, false));
    cbFilenameAsk->setToolTip(i18n("Check this if you want to enter a filename when an image has been scanned."));
    top->addWidget(cbFilenameAsk);

    top->addStretch(10);

    KPageWidgetItem *item = addPage(page, i18n("Image Saving"));
    item->setHeader(i18n("Image Saving Options"));
    item->setIcon(KIcon("document-save"));
}


void KookaPref::setupThumbnailPage()
{
    KConfigGroup grp = konf->group(THUMB_GROUP);

    QFrame *page = new QFrame(this);
    QGridLayout *lay = new QGridLayout(page);
    lay->setColumnStretch(1, 9);

    QLabel *title = new QLabel(i18n("Here you can configure the appearance of the scan gallery thumbnail view."), page);
    lay->addWidget(title, 0, 0, 1, -1);
    lay->setRowMinimumHeight(1, 2*KDialog::spacingHint());

    // Do we want a background image?
    cbCustomThumbBgnd = new QCheckBox(i18n("Use a custom background image"), page);
    cbCustomThumbBgnd->setChecked(grp.readEntry(THUMB_CUSTOM_BGND, false));
    connect(cbCustomThumbBgnd, SIGNAL(toggled(bool)), SLOT(slotCustomThumbBgndToggled(bool)));
    lay->addWidget(cbCustomThumbBgnd, 2, 0, 1, 2);

    /* Background image */
    QString bgImg = grp.readPathEntry(THUMB_BG_WALLPAPER, ThumbView::standardBackground());

    QLabel *l = new QLabel(i18n("Image:"), page);
    lay->addWidget(l, 3, 0, Qt::AlignRight);

    /* Image file selector */
    m_tileSelector = new KUrlRequester(page);
    m_tileSelector->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    kDebug() << "Setting tile URL" << bgImg;
    m_tileSelector->setUrl(bgImg);

    lay->addWidget(m_tileSelector, 3, 1);
    l->setBuddy(m_tileSelector);

    lay->setRowMinimumHeight(4, 2*KDialog::spacingHint());

    /* Preview size */
    l = new QLabel(i18n("Preview size:"), page);
    lay->addWidget(l, 5, 0, Qt::AlignRight);

    m_thumbSizeCb = new KComboBox(page);
    m_thumbSizeCb->addItem(ThumbView::sizeName(KIconLoader::SizeEnormous));	// 0
    m_thumbSizeCb->addItem(ThumbView::sizeName(KIconLoader::SizeHuge));		// 1
    m_thumbSizeCb->addItem(ThumbView::sizeName(KIconLoader::SizeLarge));	// 2
    m_thumbSizeCb->addItem(ThumbView::sizeName(KIconLoader::SizeMedium));	// 3
    m_thumbSizeCb->addItem(ThumbView::sizeName(KIconLoader::SizeSmallMedium));	// 4

    KIconLoader::StdSizes size = static_cast<KIconLoader::StdSizes>(grp.readEntry(THUMB_PREVIEW_SIZE,
                                                                                  static_cast<int>(KIconLoader::SizeHuge)));
    int sel;
    switch (size)
    {
case KIconLoader::SizeEnormous:		sel = 0;	break;
default:
case KIconLoader::SizeHuge:		sel = 1;	break;
case KIconLoader::SizeLarge:		sel = 2;	break;
case KIconLoader::SizeMedium:		sel = 3;	break;
case KIconLoader::SizeSmallMedium:	sel = 4;	break;
    }
    m_thumbSizeCb->setCurrentIndex(sel);

    lay->addWidget(m_thumbSizeCb, 5, 1);
    l->setBuddy(m_thumbSizeCb);

    lay->setRowStretch(6, 1);

    slotCustomThumbBgndToggled(cbCustomThumbBgnd->isChecked());

    KPageWidgetItem *item = addPage(page, i18n("Thumbnail View"));
    item->setHeader(i18n("Thumbnail Gallery View"));
    item->setIcon(KIcon("view-list-icons"));
}


void KookaPref::slotCustomThumbBgndToggled(bool state)
{
    m_tileSelector->setEnabled(state);
}


// TODO: split up into 1 function per page
void KookaPref::slotSaveSettings()
{
    kDebug();

    /* ** general - gallery options ** */
    KConfigGroup grp = konf->group(GROUP_GALLERY);
    grp.writeEntry(GALLERY_ALLOW_RENAME, galleryAllowRename());
    grp.writeEntry(GALLERY_LAYOUT, layoutCB->currentIndex());

    /* ** startup options ** */
    /** write the global one, to read from libkscan also */
    grp = ScanGlobal::self()->configGroup();
    bool cbVal = !cbShowScannerSelection->isChecked();
    kDebug() << "Writing" << STARTUP_SKIP_ASK << ":" << cbVal;
    grp.writeEntry(STARTUP_SKIP_ASK, cbVal);
    /* only search for local (=non-net) scanners ? */
    grp.writeEntry(STARTUP_ONLY_LOCAL, !cbNetQuery->isChecked());

    grp = konf->group(GROUP_STARTUP);
    /* Should kooka open the last displayed image in the viewer ? */
    if (cbReadStartupImage) grp.writeEntry(STARTUP_READ_IMAGE, cbReadStartupImage->isChecked());

    /* ** Image saver option(s) ** */
    grp = konf->group(OP_SAVER_GROUP);
    grp.writeEntry(OP_SAVER_ASK_FORMAT, cbSkipFormatAsk->isChecked());
    grp.writeEntry(OP_SAVER_ASK_FILENAME, cbFilenameAsk->isChecked());

    /* ** Thumbnail options ** */
    grp = konf->group(THUMB_GROUP);

    KUrl bgUrl = m_tileSelector->url();
    bgUrl.setProtocol("");
    grp.writePathEntry(THUMB_BG_WALLPAPER, bgUrl.url());
    grp.writeEntry(THUMB_CUSTOM_BGND, cbCustomThumbBgnd->isChecked());

    KIconLoader::StdSizes size;
    switch (m_thumbSizeCb->currentIndex())
    {
case 0:		size = KIconLoader::SizeEnormous;	break;
default:
case 1:		size = KIconLoader::SizeHuge;		break;
case 2:		size = KIconLoader::SizeLarge;		break;
case 3:		size = KIconLoader::SizeMedium;		break;
case 4:		size = KIconLoader::SizeSmallMedium;	break;
    }
    grp.writeEntry(THUMB_PREVIEW_SIZE, static_cast<int>(size));

    /* ** OCR Options ** */
    grp = konf->group(CFG_GROUP_OCR_DIA);
    grp.writeEntry(CFG_OCR_ENGINE2, static_cast<int>(selectedEngine));

    QString path = binaryReq->url().path();
    if (!path.isEmpty())
    {
        switch (selectedEngine)
        {
case OcrEngine::EngineGocr:
            if (checkOCRBin(path,"gocr",true)) grp.writePathEntry(CFG_GOCR_BINARY, path);
            break;

case OcrEngine::EngineOcrad:
            if (checkOCRBin(path,"ocrad",true)) grp.writePathEntry(CFG_OCRAD_BINARY, path);
            break;

default:    break;
        }
    }

    konf->sync();
    emit dataSaved();
}


// TODO: check that all controls are reset here
// TODO: split up into 1 function per page
void KookaPref::slotSetDefaults()
{
    kDebug();

    cbAllowRename->setChecked(false);
    layoutCB->setCurrentIndex(KookaGallery::RecentAtTop);

    cbNetQuery->setChecked( true );
    cbShowScannerSelection->setChecked( true);
    cbReadStartupImage->setChecked( true);
    cbSkipFormatAsk->setChecked( true  );


    cbCustomThumbBgnd->setChecked(false);
    m_tileSelector->setUrl(ThumbView::standardBackground());
    m_thumbSizeCb->setCurrentIndex(1);			// "Very Large"

    slotCustomThumbBgndToggled(cbCustomThumbBgnd->isChecked());
    slotEngineSelected(OcrEngine::EngineNone);
}


bool KookaPref::galleryAllowRename() const
{
    return (cbAllowRename->isChecked());
}


void KookaPref::slotEnableWarnings()
{
    kDebug();

    KMessageBox::enableAllMessages();
    FormatDialog::forgetRemembered();
    KGlobal::config()->reparseConfiguration();

    pbEnableMsgs->setEnabled(false);			// show this has been done
}


// Support for the gallery location - moved here from Previewer class in libkscan

QString KookaPref::sGalleryRoot = QString::null;	// global resolved location


// The static variable above ensures that the user is only asked
// at most once in an application run.

QString KookaPref::galleryRoot()
{
    if (sGalleryRoot.isNull()) sGalleryRoot = findGalleryRoot();
    return (sGalleryRoot);
}


// Get the user's configured KDE documents path.  It may not exist yet, in
// which case QDir::canonicalPath() will fail.  Use QDir::absolutePath() in
// this case.  If all else fails then the last resort is the home directory.

static QString docsPath()
{
    QString docpath = QDir(KGlobalSettings::documentPath()).canonicalPath();
    if (docpath.isEmpty()) docpath = QDir(KGlobalSettings::documentPath()).absolutePath();
    if (docpath.isEmpty()) docpath = getenv("HOME");
    return (docpath);
}


// TODO: maybe save a .directory file there which shows a 'scanner' logo?
static QString createGallery(const QDir &d, bool *success = NULL)
{
    if (!d.exists())					// does not already exist
    {
        if (mkdir(d.path().toLocal8Bit(), 0755)!=0)	// using mkdir(2) so that we can
        {						// get the errno if it fails
#ifdef HAVE_STRERROR
            const char *reason = strerror(errno);
#else
            const char *reason = "";
#endif
            QString docs = docsPath();
            KMessageBox::error(NULL,
                               i18n("<qt>"
                                    "<p>Unable to create the directory<br>"
                                    "<filename>%1</filename><br>"
                                    "for the Kooka gallery"
#ifdef HAVE_STRERROR
                                    " - %3."
#else
                                    "."
#endif
                                    "<p>Your document directory,<br>"
                                    "<filename>%2</filename><br>"
                                    "will be used."
                                    "<p>"
                                    "Check the document directory setting and permissions.",
                                    d.absolutePath(), docs, reason),
                               i18n("Error creating gallery"));
            if (success!=NULL) *success = false;
            return (docs);
        }
    }

    if (success!=NULL) *success = true;
    return (d.absolutePath());
}


QString KookaPref::findGalleryRoot()
{
    const KConfigGroup grp = KGlobal::config()->group(GROUP_GALLERY);
    QString galleryName = grp.readEntry(GALLERY_LOCATION, GALLERY_DEFAULT_LOC);
    if (galleryName.isEmpty()) galleryName = GALLERY_DEFAULT_LOC;

    QString oldloc = KGlobal::dirs()->saveLocation("data", "ScanImages", false);
    QDir olddir(oldloc);
    QString oldpath = olddir.absolutePath();
    bool oldexists = (olddir.exists());

    QString newloc = galleryName;
    QDir newdir(newloc);
    if (newdir.isRelative()) newdir.setPath(docsPath()+QDir::separator()+galleryName);
    QString newpath = newdir.absolutePath();
    bool newexists = (newdir.exists());

    kDebug() << "old" << oldpath << "exists" << oldexists;
    kDebug() << "new" << newpath << "exists" << newexists;

    QString dir;

    if (!oldexists && !newexists)			// no directories present
    {
        dir = createGallery(newdir);			// create and use new
    }
    else if (!oldexists && newexists)			// only new exists
    {
        dir = newpath;					// fine, just use that
    }
    else if (oldexists && !newexists)			// only old exists
    {
        if (KMessageBox::questionYesNo(NULL,
                                       i18n("<qt>"
                                            "<p>An old Kooka gallery was found at<br>"
                                            "<filename>%1</filename>."
                                            "<p>The preferred new location is now<br>"
                                            "<filename>%2</filename>."
                                            "<p>Do you want to create a new gallery at the new location?",
                                            oldpath, newpath),
                                       i18n("Create New Gallery"),
                                       KStandardGuiItem::yes(), KStandardGuiItem::no(),
                                       "GalleryNoMigrate")==KMessageBox::Yes)
        {						// yes, create new
            bool created;
            dir = createGallery(newdir, &created);
            if (created)				// new created OK
            {
                KMessageBox::information(NULL,
                                         i18n("<qt>"
                                              "<p>Kooka will use the new gallery,<br>"
                                              "<a href=\"file:%1\"><filename>%1</filename></a>."
                                              "<p>If you wish to add the images from your old gallery,<br>"
                                              "<a href=\"file:%2\"><filename>%2</filename></a>,<br>"
                                              "then you may do so by simply copying or moving the files.",
                                              newpath, oldpath),
                                         i18n("New Gallery Created"),
                                         QString::null,
                                         KMessageBox::Notify|KMessageBox::AllowLink);
            }
        }
        else						// no, don't create
        {
            dir = oldpath;				// stay with old location
        }
    }
    else						// both exist
    {
        KMessageBox::information(NULL,
                                 i18n("<qt>"
                                      "<p>Kooka will use the new gallery,<br>"
                                      "<a href=\"file:%1\"><filename>%1</filename></a>."
                                      "<p>If you wish to add the images from your old gallery,<br>"
                                      "<a href=\"file:%2\"><filename>%2</filename></a>,<br>"
                                      "then you may do so by simply copying or moving the files.",
                                      newpath, oldpath),
                                 i18n("Old Gallery Exists"),
                                 "GalleryNoRemind",
                                 KMessageBox::Notify|KMessageBox::AllowLink);
        dir = newpath;					// just use new one
    }

    if (!dir.endsWith("/")) dir += "/";
    kDebug() << "returning" << dir;
    return (dir);
}
