/**************************************************************************
              kookaview.cpp  -  kookas visible stuff
                             -------------------
    begin                : ?
    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de

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

#include "kookaview.h"
#include "kookaview.moc"

#include <qlabel.h>
#include <qlayout.h>
#include <qsplitter.h>
#include <qimage.h>
#include <qapplication.h>
#include <qsignalmapper.h>

#include <kurl.h>
#include <krun.h>
#include <kapplication.h>
#include <kstatusbar.h>
#include <kconfig.h>
#include <kdebug.h>
#include <ktrader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kled.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kshortcut.h>
#include <kmenu.h>
#include <ktabwidget.h>
#include <kfileitem.h>
#include <kmainwindow.h>

#include "libkscan/scandevices.h"
#include "libkscan/kscandevice.h"
#include "libkscan/imgscaninfo.h"
#include "libkscan/deviceselector.h"
#include "libkscan/adddevice.h"
#include "libkscan/previewer.h"
#include "libkscan/imagecanvas.h"

#include "imgsaver.h"
#include "kookapref.h"
#include "galleryhistory.h"
#include "thumbview.h"
#include "kookaimage.h"
#include "kookaimagemeta.h"
#include "ocrengine.h"
#include "ocrresedit.h"
#include "scanparamsdialog.h"
#include "kookagallery.h"
#include "kookascanparams.h"
#include "scangallery.h"

#ifndef KDE4
#include "kookaprint.h"
#include "imgprintdialog.h"
#include "photocopyprintdialogpage.h"
#endif


#define STARTUP_IMG_SELECTION   "SelectedImageOnStartup"

#define WCONF_TAB_INDEX		"TabIndex"

#define WCONF_SCAN_LAYOUT1	"ScanLayout1"
#define WCONF_SCAN_LAYOUT2	"ScanLayout2"
#define WCONF_GALLERY_LAYOUT1	"GalleryLayout1"
#define WCONF_GALLERY_LAYOUT2	"GalleryLayout2"
#define WCONF_OCR_LAYOUT1	"OcrLayout1"
#define WCONF_OCR_LAYOUT2	"OcrLayout2"

#define COLUMN_STATES_KEY	"GalleryState%1"


// ---------------------------------------------------------------------------

// Some of the UI panels (the gallery and the image viewer) are common
// to more that one of the main task tabs.  This means that they can't simply
// be added to the tabs/splitters in the normal way, as a widget can only be
// a child of one parent at a time.
//
// This WidgetSite acts as a layout placeholder for such reassignable widgets.
// It is assigned a new child widget when tabs are switched.
//
// It also defines the frame style for the panels.  So, in order to maintain a
// consistent look, all of those panels should derive from QWidget (or, if
// from QFrame, do not set a frame style) and should set the margin of any
// internal layout to 0.  (KHBox/KVBox do this automatically, but any other
// layout needs the margin explicitly set).

class WidgetSite : public QFrame
{
public:
    WidgetSite(QWidget *parent, QWidget *widget = NULL);
    void setWidget(QWidget *widget);

private:
    static int sCount;
};


int WidgetSite::sCount = 0;


WidgetSite::WidgetSite(QWidget *parent, QWidget *widget)
    : QFrame(parent)
{
    QString name = QString("WidgetSite-#%1").arg(++sCount);
    setObjectName(name.toAscii());

    setFrameStyle(QFrame::Panel|QFrame::Raised);	// from "scanparams.cpp"
    setLineWidth(1);

    QGridLayout* lay = new QGridLayout(this);
    lay->setRowStretch(0, 1);
    lay->setColumnStretch(0, 1);

    if (widget==NULL)
    {
        QLabel* l = new QLabel(name, this);
        l->setAlignment(Qt::AlignVCenter|Qt::AlignHCenter);
        widget = l;
    }

    //kDebug() << name
    //         << "widget is a" << widget->metaObject()->className()
    //         << "parent is a" << widget->parent()->metaObject()->className();
    lay->addWidget(widget, 0, 0);
    widget->show();
}


void WidgetSite::setWidget(QWidget *widget)
{
    QGridLayout *lay = static_cast<QGridLayout *>(layout());

    QObjectList childs = children();
    for (QObjectList::iterator it = childs.begin(); it!=childs.end(); ++it)
    {
        QObject *ch = (*it);
        if (ch->isWidgetType())
        {
            QWidget *w = static_cast<QWidget *>(ch);
            w->hide();
            lay->removeWidget(w);
        }
    }

    //kDebug() << objectName()
    //         << "widget is a" << widget->metaObject()->className()
    //         << "parent is a" << widget->parent()->metaObject()->className();
    lay->addWidget(widget, 0, 0);
    widget->show();
}


// ---------------------------------------------------------------------------

// Convenience class for layout splitter.

class WidgetSplitter : public QSplitter
{
public:
    WidgetSplitter(Qt::Orientation orientation, QWidget *parent = NULL);
};


WidgetSplitter::WidgetSplitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent)
{
    setChildrenCollapsible(false);
    setContentsMargins(0, 0, 0, 0);
    setStretchFactor(1, 1);
}


// ---------------------------------------------------------------------------


KookaView::KookaView(KMainWindow *parent, const QByteArray &deviceToUse)
    : KTabWidget(parent)
{
    setObjectName("KookaView");

    mMainWindow = parent;
    mOcrResultImg = NULL;
    mScanParams = NULL;
    mOcrEngine = NULL;
    mCurrentTab = KookaView::TabNone;

    mIsPhotoCopyMode = false;

    mOpenWithMapper = NULL;

    /** Image Viewer **/
    mImageCanvas = new ImageCanvas(this);
    mImageCanvas->setMinimumSize(100,200);
    KMenu *ctxtmenu = mImageCanvas->contextMenu();
    if (ctxtmenu!=NULL) ctxtmenu->addTitle(i18n("Image View"));

    // Connections ImageCanvas --> myself
    connect(mImageCanvas, SIGNAL(newRect(const QRect &)), SLOT(slotSelectionChanged(const QRect &)));
   
    /** Thumbnail View **/
    mThumbView = new ThumbView(this);

    /** Scan Gallery **/
    mGallery = new KookaGallery(this);
    ScanGallery *packager = mGallery->galleryTree();

    // Connections ScanGallery --> myself
    connect(packager, SIGNAL(itemHighlighted(const KUrl &,bool)),
            SLOT(slotGallerySelectionChanged()));
    connect(packager, SIGNAL(showImage(const KookaImage *,bool)),
            SLOT(slotShowAImage(const KookaImage *,bool)));
    connect(packager, SIGNAL(aboutToShowImage(const KUrl &)),
            SLOT(slotStartLoading(const KUrl &)));
    connect(packager, SIGNAL(unloadImage(const KookaImage *)),
            SLOT(slotUnloadAImage(const KookaImage *)));

    // Connections ScanGallery --> ThumbView
    connect(packager, SIGNAL(itemHighlighted(const KUrl &,bool)),
            mThumbView, SLOT(slotHighlightItem(const KUrl &,bool)));
    connect(packager, SIGNAL(imageChanged(const KFileItem *)),
            mThumbView, SLOT(slotImageChanged(const KFileItem *)));
    connect(packager, SIGNAL(fileRenamed(const KFileItem *,const QString &)),
            mThumbView, SLOT(slotImageRenamed(const KFileItem *,const QString &)));

    // Connections ThumbView --> ScanGallery
    connect(mThumbView, SIGNAL(itemHighlighted(const KUrl &)),
            packager, SLOT(slotHighlightItem(const KUrl &)));
    connect(mThumbView, SIGNAL(itemActivated(const KUrl &)),
            packager, SLOT(slotActivateItem(const KUrl &)));

    GalleryHistory *recentFolder = mGallery->galleryRecent();

    // Connections ScanGallery <--> recent folder history
    connect(packager, SIGNAL(galleryPathChanged(const FileTreeBranch *,const QString &)),
            recentFolder, SLOT(slotPathChanged(const FileTreeBranch *,const QString &)));
    connect(packager, SIGNAL(galleryDirectoryRemoved(const FileTreeBranch *,const QString &)),
            recentFolder, SLOT(slotPathRemoved(const FileTreeBranch *,const QString &)));
    connect(recentFolder, SIGNAL(pathSelected(const QString &,const QString &)),
            packager, SLOT(slotSelectDirectory(const QString &,const QString &)));

    /** Scanner Settings **/

    if (!deviceToUse.isEmpty() && deviceToUse!="gallery")
    {
        ScanDevices::self()->addUserSpecifiedDevice(deviceToUse,
                                                    "on command line",
                                                    "",
                                                    true);
    }

    /** Scanner Device **/
    mScanDevice = new KScanDevice(this);

    // Connections KScanDevice --> myself
    connect(mScanDevice, SIGNAL(sigScanFinished(KScanDevice::Status)), SLOT(slotScanFinished(KScanDevice::Status)));
    connect(mScanDevice, SIGNAL(sigScanFinished(KScanDevice::Status)), SLOT(slotPhotoCopyScan(KScanDevice::Status)));
    /* New image created after scanning now call save dialog*/
    connect(mScanDevice, SIGNAL(sigNewImage(const QImage *, const ImgScanInfo *)), SLOT(slotNewImageScanned(const QImage *,const ImgScanInfo *)));
    /* New image created after scanning now print it*/    
    connect(mScanDevice, SIGNAL(sigNewImage(const QImage *,const ImgScanInfo *)), SLOT(slotPhotoCopyPrint(const QImage *,const ImgScanInfo *)));
    /* New preview image */
    connect(mScanDevice, SIGNAL(sigNewPreview(const QImage *,const ImgScanInfo *)), SLOT(slotNewPreview(const QImage *,const ImgScanInfo *)));

    connect(mScanDevice, SIGNAL(sigScanStart()), SLOT(slotScanStart()));
    connect(mScanDevice, SIGNAL(sigAcquireStart()), SLOT(slotAcquireStart()));

    /** Scan Preview **/
    mPreviewCanvas = new Previewer(this);
    mPreviewCanvas->setMinimumSize(100,100);
    ctxtmenu = mPreviewCanvas->getImageCanvas()->contextMenu();
    if (ctxtmenu!=NULL) ctxtmenu->addTitle(i18n("Scan Preview"));

    /** Ocr Result Text **/
    mOcrResEdit  = new OcrResEdit(this);
    mOcrResEdit->setAcceptRichText(false);
    mOcrResEdit->setWordWrapMode(QTextOption::NoWrap);
    mOcrResEdit->setClickMessage(i18n("OCR results will appear here"));
    connect(mOcrResEdit, SIGNAL(spellCheckStatus(const QString &)),
            this, SIGNAL(signalChangeStatusbar(const QString &)));

    /** Status Bar **/
    KStatusBar *statBar = mMainWindow->statusBar();
    statBar->insertPermanentFixedItem((ImageCanvas::imageInfoString(2000, 2000, 48)+"--"),
                                      KookaView::StatusImage);
    statBar->changeItem(QString::null, KookaView::StatusImage);

    /** Tabs **/
    // TODO: not sure which tab position is best, make this configurable
    setTabPosition(QTabWidget::West);
    setTabsClosable(false);

    mScanPage = new WidgetSplitter(Qt::Horizontal, this);
    addTab(mScanPage, KIcon("scan"), i18n("Scan"));

    mGalleryPage = new WidgetSplitter(Qt::Horizontal, this);
    addTab(mGalleryPage, KIcon("image-x-generic"), i18n("Gallery"));

    mOcrPage = new WidgetSplitter(Qt::Vertical, this);
    addTab(mOcrPage, KIcon("ocr"), i18n("OCR"));

    connect(this, SIGNAL(currentChanged(int)), SLOT(slotTabChanged(int)));

    // TODO: make the splitter layouts and sizes configurable

    mParamsSite = new WidgetSite(this);

    mScanGallerySite = new WidgetSite(this);
    mGalleryGallerySite = new WidgetSite(this);
    mOcrGallerySite = new WidgetSite(this);

    mGalleryImgviewSite = new WidgetSite(this);
    mOcrImgviewSite = new WidgetSite(this);

    // Even widgets that are not shared between tabs are placed in a WidgetSite,
    // to keep a consistent frame appearance.

    // "Scan" page: gallery top left, scan parameters bottom left, preview right
    mScanSubSplitter = new WidgetSplitter(Qt::Vertical, mScanPage);
    mScanSubSplitter->addWidget(mScanGallerySite);			// TL
    mScanSubSplitter->addWidget(mParamsSite);				// BL
    mScanPage->addWidget(new WidgetSite(this, mPreviewCanvas));		// R

    // "Gallery" page: gallery left, viewer top right, thumbnails bottom right
    mGalleryPage->addWidget(mGalleryGallerySite);			// L
    mGallerySubSplitter = new WidgetSplitter(Qt::Vertical, mGalleryPage);
    mGallerySubSplitter->addWidget(mGalleryImgviewSite);		// TR
    mGallerySubSplitter->addWidget(new WidgetSite(this, mThumbView));	// BR

    // "OCR" page: gallery top left, viewer top right, results bottom
    mOcrSubSplitter = new WidgetSplitter(Qt::Horizontal, mOcrPage);
    mOcrSubSplitter->addWidget(mOcrGallerySite);			// TL
    mOcrSubSplitter->addWidget(mOcrImgviewSite);			// TR
    mOcrPage->addWidget(new WidgetSite(this, mOcrResEdit));		// B

    if (slotSelectDevice(deviceToUse, false)) slotTabChanged(KookaView::TabScan);
    else setCurrentIndex(KookaView::TabGallery);
}


KookaView::~KookaView()
{
    delete mScanDevice;

    kDebug();
}


// this gets called via Kooka::closeEvent() at shutdown
void KookaView::saveWindowSettings(KConfigGroup &grp)
{
    kDebug() << "to group" << grp.name();

    grp.writeEntry(WCONF_TAB_INDEX, currentIndex());

    grp.writeEntry(WCONF_SCAN_LAYOUT1, mScanPage->saveState().toBase64());
    grp.writeEntry(WCONF_SCAN_LAYOUT2, mScanSubSplitter->saveState().toBase64());
    grp.writeEntry(WCONF_GALLERY_LAYOUT1, mGalleryPage->saveState().toBase64());
    grp.writeEntry(WCONF_GALLERY_LAYOUT2, mGallerySubSplitter->saveState().toBase64());
    grp.writeEntry(WCONF_OCR_LAYOUT1, mOcrPage->saveState().toBase64());
    grp.writeEntry(WCONF_OCR_LAYOUT2, mOcrSubSplitter->saveState().toBase64());

    saveGalleryState();					// for the current tab
    grp.sync();
}


// this gets called by Kooka::applyMainWindowSettings() at startup
void KookaView::applyWindowSettings(const KConfigGroup &grp)
{
    kDebug() << "from group" << grp.name();

    QString set;

    set = grp.readEntry(WCONF_SCAN_LAYOUT1, "");
    if (!set.isEmpty()) mScanPage->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));
    set = grp.readEntry(WCONF_SCAN_LAYOUT2, "");
    if (!set.isEmpty()) mScanSubSplitter->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));

    set = grp.readEntry(WCONF_GALLERY_LAYOUT1, "");
    if (!set.isEmpty()) mGalleryPage->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));
    set = grp.readEntry(WCONF_GALLERY_LAYOUT2, "");
    if (!set.isEmpty()) mGallerySubSplitter->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));

    set = grp.readEntry(WCONF_OCR_LAYOUT1, "");
    if (!set.isEmpty()) mOcrPage->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));
    set = grp.readEntry(WCONF_OCR_LAYOUT2, "");
    if (!set.isEmpty()) mOcrSubSplitter->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));
}


void KookaView::saveGalleryState(int index) const
{
    if (index==-1) index = currentIndex();
    gallery()->saveHeaderState(QString(COLUMN_STATES_KEY).arg(index));
}


void KookaView::restoreGalleryState(int index)
{
    if (index==-1) index = currentIndex();
    gallery()->restoreHeaderState(QString(COLUMN_STATES_KEY).arg(index));
}


// this gets called by Kooka::closeEvent() at shutdown
void KookaView::saveProperties(KConfigGroup &grp)
{
    KConfigGroup grp2 = grp.config()->group(GROUP_STARTUP);
    kDebug() << "to group" << grp2.name();
    grp2.writePathEntry(STARTUP_IMG_SELECTION, gallery()->getCurrImageFileName(true));
}


void KookaView::slotTabChanged(int index)
{
    kDebug() << index;
    if (mCurrentTab!=KookaView::TabNone) saveGalleryState(mCurrentTab);
							// save state of previous tab
    switch (index)
    {
case KookaView::TabScan:				// Scan
        mScanGallerySite->setWidget(mGallery);
        mGalleryImgviewSite->setWidget(mImageCanvas);	// somewhere to park it
        break;

case KookaView::TabGallery:				// Gallery
        mGalleryGallerySite->setWidget(mGallery);
        mGalleryImgviewSite->setWidget(mImageCanvas);
        break;

case KookaView::TabOcr:					// OCR
        mOcrGallerySite->setWidget(mGallery);
        mOcrImgviewSite->setWidget(mImageCanvas);
        break;
    }

    restoreGalleryState(index);				// restore state of new tab
    mCurrentTab = static_cast<KookaView::TabPage>(index);
							// note for next tab change
    updateSelectionState();				// update image action states
}


bool KookaView::slotSelectDevice(const QByteArray &useDevice, bool alwaysAsk)
{
    kDebug() << "use device" << useDevice << "ask" << alwaysAsk;

    bool haveConnection = false;
    bool gallery_mode = (useDevice=="gallery");
    QByteArray selDevice;

    /* in case useDevice is the term 'gallery', the user does not want to
     * connect to a scanner, but only work in gallery mode. Otherwise, try
     * to read the device to use from config or from a user dialog */
    if (!gallery_mode)
    {
        selDevice =  useDevice;
        if (selDevice.isEmpty()) selDevice = userDeviceSelection(alwaysAsk);

        if (selDevice.isEmpty())			// dialogue cancelled
        {
            if (mScanParams!=NULL) return (false);	// have setup, do nothing
            gallery_mode = true;
        }
    }

    if (mScanParams!=NULL) closeScanDevice();		// remove existing GUI object

    mScanParams = new KookaScanParams(this);		// and create a new one
    connect(mScanParams, SIGNAL(actionSelectScanner()), SLOT(slotSelectDevice()));
    connect(mScanParams, SIGNAL(actionAddScanner()), SLOT(slotAddDevice()));

    mParamsSite->setWidget(mScanParams);

    if (!selDevice.isEmpty())				// connect to the selected scanner
    {
        while (!haveConnection)
        {
            kDebug() << "Opening device" << selDevice;
            KScanDevice::Status stat = mScanDevice->openDevice(selDevice);
            if (stat==KScanDevice::Ok) haveConnection = true;
            else
            {
                QString msg = i18n("<qt><p>"
                                   "There was a problem opening the scanner device."
                                   "<br>"
                                   "Check that the scanner is connected and switched on, "
                                   "and that SANE support for it is correctly configured."
                                   "<p>"
                                   "Trying to use scanner device: <b>%3</b><br>"
                                   "libkscan reported error: <b>%2</b><br>"
                                   "SANE reported error: <b>%1</b>",
                                   mScanDevice->lastSaneErrorMessage(),
                                   KScanDevice::statusMessage(stat),
                                   selDevice.constData());

                int tryAgain = KMessageBox::warningContinueCancel(mMainWindow, msg, QString::null,
                                                       KGuiItem("Retry"));
                if (tryAgain==KMessageBox::Cancel) break;
            }
        }

        if (haveConnection)				// scanner connected OK
        {
            QSize s = mScanDevice->getMaxScanSize();	// fix for 160148
            kDebug() << "scanner max size" << s;
            mPreviewCanvas->setScannerBedSize(s.width(),s.height());

            // Connections ScanParams --> Previewer
            connect(mScanParams,SIGNAL(scanResolutionChanged(int,int)),
                    mPreviewCanvas,SLOT(slotNewScanResolutions(int,int)));
            connect(mScanParams,SIGNAL(scanModeChanged(int)),
                    mPreviewCanvas,SLOT(slotNewScanMode(int)));
            connect(mScanParams,SIGNAL(newCustomScanSize(const QRect &)),
                    mPreviewCanvas,SLOT(slotNewCustomScanSize(const QRect &)));

            // Connections Previewer --> ScanParams
            connect(mPreviewCanvas,SIGNAL(newPreviewRect(const QRect &)),
                    mScanParams,SLOT(slotNewPreviewRect(const QRect &)));

            mScanParams->connectDevice(mScanDevice);	// create scanning options

            mPreviewCanvas->setPreviewImage(mScanDevice->loadPreviewImage());
							// load saved preview image
            mPreviewCanvas->connectScanner(mScanDevice);
							// load its autosel options
        }
    }

    if (!haveConnection)				// no scanner device available,
    {							// or starting in gallery mode
        if (mScanParams!=NULL) mScanParams->connectDevice(NULL, gallery_mode);
    }

    emit signalScannerChanged(haveConnection);
    return (haveConnection);
}


void KookaView::slotAddDevice()
{
    AddDeviceDialog d(mMainWindow, i18n("Add Scan Device"));
    if (d.exec())
    {
	QByteArray dev = d.getDevice();
	QString dsc = d.getDescription();
	QByteArray type = d.getType();
	kDebug() << "dev" << dev << "type" << type << "desc" << dsc;

	ScanDevices::self()->addUserSpecifiedDevice(dev, dsc, type);
    }
}


QByteArray KookaView::userDeviceSelection(bool alwaysAsk)
{
   /* a list of backends the scan backend knows */
   QList<QByteArray> backends = ScanDevices::self()->allDevices();
   if (backends.count()==0)
   {
       if (KMessageBox::warningContinueCancel(mMainWindow, i18n("<qt>\
<p>No scanner devices are available.\
<p>If your scanner is a type that can be auto-detected by SANE, check that it \
is connected, switched on and configured correctly.\
<p>If the scanner cannot be auto-detected by SANE (this includes some network \
scanners), you need to specify the device to use.  Use the \"Add Scan Device\" \
option to enter the backend name and parameters, or see that dialog for more \
information."), QString::null, KGuiItem(i18n("Add Scan Device...")))!=KMessageBox::Continue) return ("");

       slotAddDevice();
       backends = ScanDevices::self()->allDevices();	// refresh the list
       if (backends.count()==0) return ("");		// give up this time
   }

   QByteArray selDevice;
   DeviceSelector ds(mMainWindow, backends,
                     (alwaysAsk ? KGuiItem() : KGuiItem(i18n("Gallery"), KIcon("image-x-generic"))));
   if (!alwaysAsk) selDevice = ds.getDeviceFromConfig();

   if (selDevice.isEmpty())
   {
       kDebug() << "no selDevice, starting selector";
       if (ds.exec()==QDialog::Accepted) selDevice = ds.getSelectedDevice();
       kDebug() << "selector returned device" << selDevice;
   }

   return (selDevice);
}


QString KookaView::scannerName() const
{
    if (mScanDevice->scannerBackendName().isEmpty()) return (i18n("Gallery"));
    if (!mScanDevice->isScannerConnected()) return (i18n("No scanner connected"));
    return (mScanDevice->scannerDescription());
}


bool KookaView::isScannerConnected() const
{
    return (mScanDevice->isScannerConnected());
}


void KookaView::slotSelectionChanged(const QRect &newSelection)
{
    emit signalRectangleChanged(newSelection.isValid());
}


//  The 'state' generated here is used by Kooka::slotUpdateViewActions().

void KookaView::updateSelectionState()
{
    KookaView::StateFlags state = 0;
    const FileTreeViewItem *ftvi = mGallery->galleryTree()->highlightedFileTreeViewItem();
    const KFileItem *fi = (ftvi!=NULL ? ftvi->fileItem() : NULL);
    const bool scanmode = (mCurrentTab==KookaView::TabScan);

    if (scanmode)					// Scan mode
    {
        if (mPreviewCanvas->getImageCanvas()->hasImage()) state |= KookaView::PreviewValid;
    }
    else						// Gallery/OCR mode
    {
        state |= KookaView::GalleryShown;
    }

    if (fi!=NULL && !fi->isNull())			// have a gallery selection
    {
        if (fi->isDir()) state |= KookaView::IsDirectory;
        else state |= KookaView::FileSelected;
    }

    if (ftvi!=NULL && ftvi->isRoot()) state |= KookaView::RootSelected;
    if (imageViewer()->hasImage()) state |= KookaView::ImageValid;

    //kDebug() << "state" << state;
    emit signalViewSelectionState(state);
}


void KookaView::slotGallerySelectionChanged()
{
    const FileTreeViewItem *ftvi = mGallery->galleryTree()->highlightedFileTreeViewItem();
    const KFileItem *fi = (ftvi!=NULL ? ftvi->fileItem() : NULL);
    const bool scanmode = (mCurrentTab==KookaView::TabScan);

    if (fi==NULL || fi->isNull())			// no gallery selection
    {
        if (!scanmode) emit signalChangeStatusbar(i18n("No selection"));
    }
    else						// have a gallery selection
    {
        if (!scanmode)
        {
            KLocalizedString str = fi->isDir() ? ki18n("Gallery folder %1") : ki18n("Gallery image %1");
            emit signalChangeStatusbar(str.subs(fi->url().pathOrUrl()).toString());
        }
    }

    updateSelectionState();
}


void KookaView::loadStartupImage()
{
    const KSharedConfig *konf = KGlobal::config().data();
    const KConfigGroup grp = konf->group(GROUP_STARTUP);
    bool wantReadOnStart = grp.readEntry(STARTUP_READ_IMAGE, true);

   if (wantReadOnStart)
   {
       QString startup = grp.readPathEntry(STARTUP_IMG_SELECTION, "");
       kDebug() << "load startup image" << startup;
       if (!startup.isEmpty()) gallery()->slotSelectImage(startup);
   }
   else kDebug() << "do not load startup image";
}


void KookaView::print()
{
// TODO: printing in KDE4
#ifndef KDE4
    /* For now, print a single file. Later, print multiple images to one page */

    KookaImage *img = gallery()->getCurrImage(true);	// load image if necessary
    if (img==NULL) return;

    KPrinter printer; // ( true, pMode );
    printer.setUsePrinterResolution(true);
    printer.addDialogPage( new ImgPrintDialog( img ));

    if( printer.setup( mMainWindow, i18n("Print %1").arg(img->localFileName().section('/', -1)) ))
    {
	KookaPrint kookaprint( &printer );
	kookaprint.printImage(img, 10);
    }
#endif
}


void KookaView::slotNewPreview(const QImage *newimg, const ImgScanInfo *info)
{
   if (newimg==NULL) return;

   kDebug() << "new preview image, size" << newimg->size()
		  << "res [" << info->getXResolution() << "x" << info->getYResolution() << "]";

   mPreviewCanvas->newImage(newimg);			// set new image and size
   updateSelectionState();
}


void KookaView::slotStartOcrSelection()
{
   emit signalChangeStatusbar(i18n("Starting OCR on selection"));
   startOCR(mImageCanvas->selectedImage());
   emit signalCleanStatusbar();
}


void KookaView::slotStartOcr()
{
   emit signalChangeStatusbar(i18n("Starting OCR on the image"));
   startOCR(*gallery()->getCurrImage(true));
   emit signalCleanStatusbar();
}


void KookaView::slotSetOcrSpellConfig(const QString &configFile)
{
    kDebug() << configFile;
    if (mOcrResEdit!=NULL) mOcrResEdit->setSpellCheckingConfigFileName(configFile);
}


void KookaView::slotOcrSpellCheck(bool interactive, bool background)
{
    if (!interactive && !background)			// not doing anything
    {
        emit signalChangeStatusbar(i18n("OCR finished"));
        return;
    }

    if (mOcrEngine==NULL || !mOcrEngine->engineValid() || mOcrResEdit==NULL)
    {
        KMessageBox::sorry(mMainWindow,
                           i18n("OCR has not been performed yet, or the engine has been changed."),
                           i18n("OCR Spell Check not possible"));
        return;
    }

    setCurrentIndex(KookaView::TabOcr);
    emit signalChangeStatusbar(i18n("OCR Spell Check"));
    if (background) mOcrResEdit->setCheckSpellingEnabled(true);
    if (interactive) mOcrResEdit->checkSpelling();
}


void KookaView::startOCR(const KookaImage img)
{
    if (img.isNull()) return;

    setCurrentIndex(KookaView::TabOcr);

    if (mOcrEngine!=NULL && !mOcrEngine->engineValid())	// engine already exists,
    {							// but needs to be changed?
        delete mOcrEngine;
        mOcrEngine = NULL;
    }

    if (mOcrEngine==NULL)
    {
        bool gotoPrefs = false;
        mOcrEngine = OcrEngine::createEngine(this, &gotoPrefs);
        if (mOcrEngine==NULL)
        {
            kDebug() << "Cannot create OCR engine!";
            if (gotoPrefs) emit signalOcrPrefs();
            return;
        }

        mOcrEngine->setImageCanvas(mImageCanvas);
        mOcrEngine->setTextDocument(mOcrResEdit->document());

        // Connections OcrEngine --> myself
        connect(mOcrEngine, SIGNAL(newOCRResultText()),
                SLOT(slotOcrResultAvailable()));
        connect(mOcrEngine, SIGNAL(setSpellCheckConfig(const QString &)),
                SLOT(slotSetOcrSpellConfig(const QString &)));
        connect(mOcrEngine, SIGNAL(startSpellCheck(bool,bool)),
                SLOT(slotOcrSpellCheck(bool,bool)));
	  
        // Connections OCR Results --> OCR Engine
        connect(mOcrResEdit, SIGNAL(highlightWord(const QRect &)),
                mOcrEngine, SLOT(slotHighlightWord(const QRect &)));
        connect(mOcrResEdit, SIGNAL(scrollToWord(const QRect &)),
                mOcrEngine, SLOT(slotScrollToWord(const QRect &)));

        // Connections OcrEngine --> OCR Results
        connect(mOcrEngine, SIGNAL(readOnlyEditor(bool)),
                mOcrResEdit, SLOT(slotSetReadOnly(bool)));
        connect(mOcrEngine, SIGNAL(selectWord(const QPoint &)),
                mOcrResEdit, SLOT(slotSelectWord(const QPoint &)));
    }

    mOcrEngine->setImage(img);
    mOcrEngine->startOCRVisible(this);
}


void KookaView::slotOcrResultAvailable()
{
    emit signalOcrResultAvailable(mOcrResEdit->document()->characterCount()>0);

}


void KookaView::slotScanStart( )
{
   kDebug() << "Scan starts";
   if( mScanParams )
   {
      mScanParams->setEnabled( false );
      KLed *led = mScanParams->operationLED();
      if( led )
      {
	 led->setColor( Qt::red );
	 led->setState( KLed::On );
         qApp->processEvents();
      }
   }
}


void KookaView::slotAcquireStart( )
{
   kDebug() << "Acquire starts";
   if( mScanParams )
   {
      KLed *led = mScanParams->operationLED();
      if( led )
      {
	 led->setColor( Qt::green );
         qApp->processEvents();
      }
   }
}


void KookaView::slotNewImageScanned(const QImage *img, const ImgScanInfo *info)
{
    if (mIsPhotoCopyMode) return;

    KookaImageMeta *meta = new KookaImageMeta;
    meta->setScanResolution(info->getXResolution(), info->getYResolution());
    gallery()->addImage(img, meta);
}


void KookaView::slotScanFinished(KScanDevice::Status stat)
{
    kDebug() << "Scan finished with status" << stat;

    if (stat!=KScanDevice::Ok && stat!=KScanDevice::Cancelled)
    {
        QString msg = i18n("<qt><p>"
                           "There was a problem during preview or scanning."
                           "<br>"
                           "Check that the scanner is still connected and switched on, "
                           "<br>"
                           "and that media is loaded if required."
                           "<p>"
                           "Trying to use scanner device: <b>%3</b><br>"
                           "libkscan reported error: <b>%2</b><br>"
                           "SANE reported error: <b>%1</b>",
                           mScanDevice->lastSaneErrorMessage(),
                           KScanDevice::statusMessage(stat),
                           mScanDevice->scannerBackendName().constData());
        KMessageBox::error(mMainWindow, msg);
    }

    if (mScanParams!=NULL)
    {
        mScanParams->setEnabled( true );
        KLed *led = mScanParams->operationLED();
        if (led!=NULL)
        {
            led->setColor(Qt::green);
            led->setState(KLed::Off);
        }
    }
}


void KookaView::closeScanDevice( )
{
    kDebug() << "Scanner Device closes down";
    if (mScanParams!=NULL)
    {
        delete mScanParams;
        mScanParams = NULL;
    }

    mScanDevice->closeDevice();
}


void KookaView::slotCreateNewImgFromSelection()
{
    emit signalChangeStatusbar(i18n("Create new image from selection"));
    QImage img = mImageCanvas->selectedImage();
    if (!img.isNull()) gallery()->addImage(&img);
    emit signalCleanStatusbar();
}


//  This and slotMirrorImage() below work even if the selected image is
//  not currently loaded, hence the getCurrImage(true) which automatically
//  loads it if necessary.

void KookaView::slotRotateImage(int angle)
{
    const KookaImage *img = gallery()->getCurrImage(true);
    if (img==NULL) return;

    bool doUpdate = true;

    QImage resImg;
    QMatrix m;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    switch (angle)
    {
case 90:
        emit signalChangeStatusbar(i18n("Rotate image 90 degrees"));
        m.rotate(+90);
        resImg = img->transformed(m);
        break;

case 270:
case -90:
        emit signalChangeStatusbar(i18n("Rotate image -90 degrees"));
        m.rotate(-90);
        resImg = img->transformed(m);
        break;

default:
        kDebug() << "Angle" << angle << "not supported!";
        doUpdate = false;
        break;
    }
    QApplication::restoreOverrideCursor();

    /* updateCurrImage does the status-bar cleanup */
    if (doUpdate) updateCurrImage(resImg);
    else emit signalCleanStatusbar();
}


void KookaView::slotMirrorImage(KookaView::MirrorType type)
{
    const KookaImage *img = gallery()->getCurrImage(true);
    if (img==NULL) return;

    bool doUpdate = true;
    QImage resImg;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    switch (type)
    {
case MirrorVertical:
        emit signalChangeStatusbar(i18n("Mirroring image vertically"));
        resImg = img->mirrored();
        break;

case MirrorHorizontal:
        emit signalChangeStatusbar(i18n("Mirroring image horizontally"));
        resImg = img->mirrored(true, false);
        break;

case MirrorBoth:
        emit signalChangeStatusbar(i18n("Mirroring image both directions"));
        resImg = img->mirrored(true, true);
        break;

default:
        kDebug() << "Mirror type" << type << "not supported!";
        doUpdate = false;
    }
    QApplication::restoreOverrideCursor();

    /* updateCurrImage does the status-bar cleanup */
    if (doUpdate) updateCurrImage(resImg);
    else emit signalCleanStatusbar();
}


void KookaView::slotSaveOcrResult()
{
    if (mOcrResEdit!=NULL) mOcrResEdit->slotSaveText();
}


void KookaView::slotScanParams()
{
    if (mScanDevice==NULL) return;			// must have a scanner device
    ScanParamsDialog d(this, mScanDevice);
    d.exec();
}


void KookaView::slotShowAImage(const KookaImage *img, bool isDir)
{
    if (mImageCanvas!=NULL)				// load into image viewer
    {
        mImageCanvas->newImage(img);
        mImageCanvas->setReadOnly(false);
    }

    if (mOcrEngine!=NULL) mOcrEngine->setImage(img!=NULL ? *img : KookaImage());
							// tell OCR about it
    KStatusBar *statBar = mMainWindow->statusBar();	// show status bar info
    if (mImageCanvas!=NULL) statBar->changeItem(mImageCanvas->imageInfoString(), KookaView::StatusImage);

    if (img!=NULL) emit signalChangeStatusbar(i18n("Loaded image %1", img->url().pathOrUrl()));
    else if (!isDir) emit signalChangeStatusbar(i18n("Unloaded image"));
    updateSelectionState();
}


void KookaView::slotUnloadAImage(const KookaImage *img)
{
    kDebug() << "Unloading Image";
    if (mImageCanvas!=NULL) mImageCanvas->newImage(NULL);
    updateSelectionState();
}


/* this slot is called when the user clicks on an image in the packager
 * and loading of the image starts
 */
void KookaView::slotStartLoading(const KUrl &url)
{
    emit signalChangeStatusbar(i18n("Loading %1",url.prettyUrl()));
}


void KookaView::updateCurrImage(const QImage &img)
{
    if (!mImageCanvas->readOnly())
    {
	emit signalChangeStatusbar(i18n("Updating image"));
	gallery()->slotCurrentImageChanged(&img);
	emit signalCleanStatusbar();
    }
    else
    {
	emit signalChangeStatusbar(i18n("Cannot update image, it is read only"));
    }
}


void KookaView::connectViewerAction(KAction *action, bool sepBefore)
{
    if (action==NULL) return;
    KMenu *popup = mImageCanvas->contextMenu();
    if (popup==NULL) return;

    if (sepBefore) popup->addSeparator();
    popup->addAction(action);
}


void KookaView::connectGalleryAction(KAction *action, bool sepBefore)
{
    if (action==NULL) return;
    KMenu *popup = gallery()->contextMenu();
    if (popup==NULL) return;

    if (sepBefore) popup->addSeparator();
    popup->addAction(action);
}


void KookaView::connectThumbnailAction(KAction *action)
{
    if (action==NULL) return;
    KMenu *popup = mThumbView->contextMenu();
    if (popup!=NULL) popup->addAction(action);
}


void KookaView::connectPreviewAction(KAction *action)
{
    if (action==NULL) return;
    KMenu *popup = mPreviewCanvas->getImageCanvas()->contextMenu();
    if (popup!=NULL) popup->addAction(action);
}


void KookaView::slotApplySettings()
{
   if (mThumbView!=NULL) mThumbView->readSettings();	// size and background
   if (mGallery!=NULL) mGallery->readSettings();	// layout and rename
}


// Starting a scan or preview, switch tab and tell the scan device

void KookaView::slotStartPreview()
{
    if (mScanParams==NULL) return;
    setCurrentIndex(KookaView::TabScan);
    qApp->processEvents();				// let the tab appear
    mScanParams->slotAcquirePreview();
}


void KookaView::slotStartFinalScan()
{
    if (mScanParams==NULL) return;
    setCurrentIndex(KookaView::TabScan);
    qApp->processEvents();				// let the tab appear
    mScanParams->slotStartScan();
}


/* Slot called to start copying to printer */
void KookaView::slotStartPhotoCopy( )
{
    kDebug();

#ifndef KDE4
    if ( mScanParams == 0 ) return;
    mIsPhotoCopyMode=true;
    mPhotoCopyPrinter = new KPrinter( true, QPrinter::HighResolution ); 
//    mPhotoCopyPrinter->removeStandardPage( KPrinter::CopiesPage );
    mPhotoCopyPrinter->setUsePrinterResolution(true);
    mPhotoCopyPrinter->setOption( OPT_SCALING, "scan");
    mPhotoCopyPrinter->setFullPage(true);
    slotPhotoCopyScan( KScanDevice::Ok );
#endif
}


void KookaView::slotPhotoCopyPrint(const QImage *img, const ImgScanInfo *info)
{
    kDebug();

#ifndef KDE4
    if ( ! mIsPhotoCopyMode ) return;
    KookaImage kooka_img=KookaImage( *img );
    KookaPrint kookaprint( mPhotoCopyPrinter );
    kookaprint.printImage( &kooka_img ,0 );
#endif

}


void KookaView::slotPhotoCopyScan(KScanDevice::Status status)
{
    kDebug();

#ifndef KDE4
    if ( ! mIsPhotoCopyMode ) return;

//    mPhotoCopyPrinter->addDialogPage( new PhotoCopyPrintDialogPage( mScanDevice ) );

    KScanOption res ( SANE_NAME_SCAN_RESOLUTION );
    mPhotoCopyPrinter->setOption( OPT_SCAN_RES, res.get() );
    mPhotoCopyPrinter->setMargins(QSize(0,0));
    kDebug() << "Resolution" << res.get();

//    mPhotoCopyPrinter->addDialogPage( new ImgPrintDialog( 0 ) );
    if( mPhotoCopyPrinter->setup( 0, "Photocopy" )) {
        Q_CHECK_PTR( mScanDevice );
        mScanParams->slotStartScan( );
    }
    else {
        mIsPhotoCopyMode=false;
        delete mPhotoCopyPrinter;
    }
#endif
}


ScanGallery *KookaView::gallery() const
{
    return (mGallery->galleryTree());
}


void KookaView::slotImageViewerAction(int act)
{
    if (mCurrentTab==KookaView::TabScan)		// Scan
    {
        mPreviewCanvas->getImageCanvas()->slotUserAction(act);
    }
    else						// Gallery or OCR
    {
        mImageCanvas->slotUserAction(act);
    }
}


void KookaView::showOpenWithMenu(KActionMenu *menu)
{
    FileTreeViewItem *curr = gallery()->highlightedFileTreeViewItem();
    QString mimeType = KMimeType::findByUrl(curr->url())->name();
    kDebug() << "Trying to open" << curr->url() << "which is" << mimeType;

    if (mOpenWithMapper==NULL)
    {
        mOpenWithMapper = new QSignalMapper(this);
        connect(mOpenWithMapper, SIGNAL(mapped(int)), SLOT(slotOpenWith(int)));
    }

    menu->menu()->clear();

    int i = 0;
    mOpenWithOffers = KMimeTypeTrader::self()->query(mimeType);
    for (KService::List::ConstIterator it = mOpenWithOffers.constBegin();
         it!=mOpenWithOffers.constEnd(); ++it, ++i)
    {
        KService::Ptr service = (*it);
        kDebug() << "> offer:" << (*it)->name();

        QString actionName((*it)->name().replace("&","&&"));
        KAction *act = new KAction(KIcon((*it)->icon()), actionName, this);
        connect(act, SIGNAL(triggered()), mOpenWithMapper, SLOT(map()));
        mOpenWithMapper->setMapping(act, i);
        menu->addAction(act);
    }

    menu->menu()->addSeparator();
    KAction *act = new KAction(i18n("Other..."), this);
    connect(act, SIGNAL(triggered()), mOpenWithMapper, SLOT(map()));
    mOpenWithMapper->setMapping(act, i);
    menu->addAction(act);
}


void KookaView::slotOpenWith(int idx)
{
    kDebug() << "idx" << idx;

    FileTreeViewItem *ftvi = gallery()->highlightedFileTreeViewItem();
    if (ftvi==NULL) return;
    KUrl::List urllist(ftvi->url());

    if (idx<mOpenWithOffers.count())			// application from the menu
    {
        KService::Ptr ptr = mOpenWithOffers[idx];
        KRun::run(*ptr, urllist, mMainWindow);
    }
    else						// last item = "Other..."
    {
        KRun::displayOpenWithDialog(urllist, mMainWindow);
    }
}
