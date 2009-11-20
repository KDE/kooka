/**************************************************************************
              kookaview.cpp  -  kookas visible stuff
                             -------------------
    begin                : ?
    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de

    $Id$
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
#include <k3dockwidget.h>
#include <kmenu.h>
#include <ktabwidget.h>
#include <kfileitem.h>

#include "libkscan/scanparams.h"
#include "libkscan/kscandevice.h"
#include "libkscan/imgscaninfo.h"
#include "libkscan/devselector.h"
#include "libkscan/adddevice.h"
#include "libkscan/previewer.h"

#include "imgsaver.h"
#include "kookapref.h"
#include "imagenamecombo.h"
#include "thumbview.h"
#include "kookaimage.h"
#include "kookaimagemeta.h"
#include "ocrresedit.h"
#include "scanparamsdialog.h"
#include "kookagallery.h"
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

    m_mainWindow = parent;
    m_ocrResultImg = NULL;
    mScanParams = NULL;
    ocrFabric = NULL;
    mPreviousTab = -1;

    isPhotoCopyMode = false;

    /** Image Viewer **/
    img_canvas  = new ImageCanvas(this);
    img_canvas->setMinimumSize(100,200);
    img_canvas->enableContextMenu(true);

    // Connections ImageCanvas --> myself
    connect(img_canvas, SIGNAL(imageReadOnly(bool)), SLOT(slotViewerReadOnly(bool)));
    connect(img_canvas, SIGNAL(newRect(QRect)), SLOT(slotSelectionChanged(QRect)));
   
    KMenu *ctxtmenu = img_canvas->contextMenu();
    if (ctxtmenu!=NULL) ctxtmenu->addTitle(i18n("Image View"));

    /** Thumbnail View **/
    m_thumbview = new ThumbView(this);

    /** Scan Gallery **/
    m_gallery = new KookaGallery(this);
    ScanGallery *packager = m_gallery->galleryTree();

    // Connections ScanGallery --> ThumbView
    connect(packager, SIGNAL(showItem(const KFileItem *)),
            m_thumbview, SLOT(slotSelectImage(const KFileItem *)));
    connect(packager, SIGNAL(imageChanged(const KFileItem *)),
            m_thumbview, SLOT(slotImageChanged(const KFileItem *)));
    connect(packager, SIGNAL(fileRenamed(const KFileItem *,const QString &)),
            m_thumbview, SLOT(slotImageRenamed(const KFileItem *,const QString &)));

    // Connections ThumbView --> ScanGallery
    connect(m_thumbview, SIGNAL(selectFromThumbnail(const KUrl &)),
            packager, SLOT(slotSelectImage(const KUrl &)));

    // Connections ScanGallery --> myself
    connect(packager, SIGNAL(itemSelectionChanged()), SLOT(slotGallerySelectionChanged()));
    connect(packager, SIGNAL(showImage(const KookaImage *,bool)), SLOT(slotLoadedImageChanged(const KookaImage *,bool)));

    ImageNameCombo *recentFolder = m_gallery->galleryRecent();

    // Connections ScanGallery <--> recent folder history
    connect(packager, SIGNAL(galleryPathChanged(FileTreeBranch *,const QString &)),
            recentFolder, SLOT(slotPathChanged(FileTreeBranch *,const QString &)));
    connect(packager, SIGNAL(galleryDirectoryRemoved(FileTreeBranch *,const QString &)),
            recentFolder, SLOT(slotPathRemoved(FileTreeBranch *,const QString &)));
    connect(recentFolder, SIGNAL(pathSelected(const QString &,const QString &)),
            packager, SLOT(slotSelectDirectory(const QString &,const QString &)));

    /** Scanner Settings **/
    sane = new KScanDevice(this);
    Q_CHECK_PTR(sane);
    if (!deviceToUse.isEmpty() && deviceToUse!="gallery")
        sane->addUserSpecifiedDevice(deviceToUse, "on command line", true);

    /* select the scan device, either user or from config, this creates and assembles
     * the complete scanner options dialog
     * mScanParams must be zero for that */

    /** Scan Preview **/
    preview_canvas = new Previewer(this);
    preview_canvas->setMinimumSize(100,100);
    /* since the mScanParams will be created in slSelectDevice, do the
     * connections later
     */

    /** Ocr Result Text **/
    m_ocrResEdit  = new OcrResEdit(this);
    m_ocrResEdit->setTextFormat(Qt::PlainText);
    m_ocrResEdit->setWordWrap(Q3TextEdit::NoWrap);

    // Connections KScanDevice --> myself
    connect(sane, SIGNAL(sigScanFinished(KScanStat)), SLOT(slotScanFinished(KScanStat)));
    connect(sane, SIGNAL(sigScanFinished(KScanStat)), SLOT(slotPhotoCopyScan(KScanStat)));
    /* New image created after scanning now call save dialog*/
    connect(sane, SIGNAL(sigNewImage(const QImage *, const ImgScanInfo *)), SLOT(slotNewImageScanned(const QImage *,const ImgScanInfo *)));
    /* New image created after scanning now print it*/    
    connect(sane, SIGNAL(sigNewImage(const QImage *,const ImgScanInfo *)), SLOT(slotPhotoCopyPrint(const QImage *,const ImgScanInfo *)));
    /* New preview image */
    connect(sane, SIGNAL(sigNewPreview(const QImage *,const ImgScanInfo *)), SLOT(slotNewPreview(const QImage *,const ImgScanInfo *)));

    connect(sane, SIGNAL(sigScanStart()), SLOT(slotScanStart()));
    connect(sane, SIGNAL(sigAcquireStart()), SLOT(slotAcquireStart()));

    // Connections ScanGallery --> myself
    connect(packager, SIGNAL(showImage(const KookaImage *, bool)), SLOT(slotShowAImage(const KookaImage *)));
    connect(packager, SIGNAL(aboutToShowImage(const KUrl &)), SLOT(slotStartLoading(const KUrl &)));
    connect(packager, SIGNAL(unloadImage(const KookaImage *)), SLOT(slotUnloadAImage(const KookaImage *)));

    packager->openRoots();

    /** Status Bar **/
    KStatusBar *statBar = m_mainWindow->statusBar();
    statBar->insertPermanentFixedItem((img_canvas->imageInfoString(2000, 2000, 48)+"--"),
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
    mScanPage->addWidget(new WidgetSite(this, preview_canvas));		// R

    // "Gallery" page: gallery left, viewer top right, thumbnails bottom right
    mGalleryPage->addWidget(mGalleryGallerySite);			// L
    mGallerySubSplitter = new WidgetSplitter(Qt::Vertical, mGalleryPage);
    mGallerySubSplitter->addWidget(mGalleryImgviewSite);		// TR
    mGallerySubSplitter->addWidget(new WidgetSite(this, m_thumbview));	// BR

    // "OCR" page: gallery top left, viewer top right, results bottom
    mOcrSubSplitter = new WidgetSplitter(Qt::Horizontal, mOcrPage);
    mOcrSubSplitter->addWidget(mOcrGallerySite);			// TL
    mOcrSubSplitter->addWidget(mOcrImgviewSite);			// TR
    mOcrPage->addWidget(new WidgetSite(this, m_ocrResEdit));		// B

    if (slotSelectDevice(deviceToUse, false)) slotTabChanged(KookaView::TabScan);
    else setCurrentIndex(KookaView::TabGallery);
}


KookaView::~KookaView()
{
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
    kDebug() << "to group" << grp.name();
    grp.writePathEntry(STARTUP_IMG_SELECTION, gallery()->getCurrImageFileName(true));
}


void KookaView::slotTabChanged(int index)
{
    kDebug() << index;
    if (mPreviousTab!=-1) saveGalleryState(mPreviousTab);
							// save state of previous tab
    switch (index)
    {
case 0:							// Scan
        mScanGallerySite->setWidget(m_gallery);
        mGalleryImgviewSite->setWidget(img_canvas);	// somewhere to park it
        break;

case 1:							// Gallery
        mGalleryGallerySite->setWidget(m_gallery);
        mGalleryImgviewSite->setWidget(img_canvas);
        break;

case 2:							// OCR
        mOcrGallerySite->setWidget(m_gallery);
        mOcrImgviewSite->setWidget(img_canvas);
        break;
    }

    restoreGalleryState(index);				// restore state of new tab
    mPreviousTab = index;				// note for next tab change
}


void KookaView::slotViewerReadOnly( bool )
{
    /* retrieve actions that could change the image */
}


bool KookaView::slotSelectDevice(const QByteArray &useDevice, bool alwaysAsk)
{
    kDebug() << "use device" << useDevice << "ask" << alwaysAsk;
    haveConnection = false;
    connectedDevice = "";

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

    // Allow reopening
    //if( connectedDevice == selDevice ) {
    //  kdDebug( 28000) << "Device " << selDevice << " is already selected!" << endl;
    //return( true );
    //}

    if (mScanParams!=NULL) closeScanDevice();		// remove existing GUI object

    mScanParams = new ScanParams(this);			// and create a new one
    Q_CHECK_PTR(mScanParams);
    mParamsSite->setWidget(mScanParams);

    if (!selDevice.isEmpty())				// connect to the selected scanner
    {
        while (!haveConnection)
        {
            kDebug() << "Opening device" << selDevice;
            if (sane->openDevice(selDevice)==KSCAN_OK) haveConnection = true;
            else
            {
                QString msg = i18n("<qt><p>\
There was a problem opening the scanner device. \
Check that the scanner is connected and switched on, and that SANE support for it \
is correctly configured.\
<p>\
Trying to use scanner device: <b>%2</b>\
<br>\
The error reported was: <b>%1</b>", sane->lastErrorMessage(), selDevice.data());

                int tryAgain = KMessageBox::warningContinueCancel(m_mainWindow, msg, QString::null,
                                                       KGuiItem("Retry"));
                if (tryAgain==KMessageBox::Cancel) break;
            }
        }

        if (haveConnection)				// scanner connected OK
        {
            QSize s = sane->getMaxScanSize();		// fix for 160148
            kDebug() << "scanner max size" << s;
            preview_canvas->setScannerBedSize(s.width(),s.height());

            // Connections ScanParams --> Previewer
            connect(mScanParams,SIGNAL(scanResolutionChanged(int,int)),
                    preview_canvas,SLOT(slotNewScanResolutions(int,int)));
            connect(mScanParams,SIGNAL(scanModeChanged(int)),
                    preview_canvas,SLOT(slotNewScanMode(int)));
            connect(mScanParams,SIGNAL(newCustomScanSize(QRect)),
                    preview_canvas,SLOT(slotNewCustomScanSize(QRect)));

            // Connections Previewer --> ScanParams
            connect(preview_canvas,SIGNAL(newPreviewRect(QRect)),
                    mScanParams,SLOT(slotNewPreviewRect(QRect)));

            mScanParams->connectDevice(sane);
            connectedDevice = selDevice;

            preview_canvas->setPreviewImage(sane->loadPreviewImage());
							// load saved preview image
            preview_canvas->connectScanner(sane);	// load its autosel options
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
    AddDeviceDialog d(m_mainWindow, i18n("Add Scan Device"));
    if (d.exec())
    {
	QString dev = d.getDevice();
	QString dsc = d.getDescription();
	kDebug() << "dev" << dev << "desc" << dsc;

	sane->addUserSpecifiedDevice(dev,dsc);
    }
}


QByteArray KookaView::userDeviceSelection(bool alwaysAsk)
{
   /* Human readable scanner descriptions */
   QStringList hrbackends;

   /* a list of backends the scan backend knows */
   QList<QByteArray> backends = sane->getDevices();

   if (backends.count()==0)
   {
       if (KMessageBox::warningContinueCancel(m_mainWindow, i18n("<qt>\
<p>No scanner devices are available.\
<p>If your scanner is a type that can be auto-detected by SANE, check that it \
is connected, switched on and configured correctly.\
<p>If the scanner cannot be auto-detected by SANE (this includes some network \
scanners), you need to specify the device to use.  Use the \"Add Scan Device\" \
option to enter the backend name and parameters, or see that dialogue for more \
information."), QString::null, KGuiItem(i18n("Add Scan Device...")))!=KMessageBox::Continue) return ("");

       slotAddDevice();
       backends = sane->getDevices();			// refresh the list
       if (backends.count()==0) return ("");		// give up this time
   }

   QByteArray selDevice;
   for (QList<QByteArray>::const_iterator it = backends.constBegin();
        it!=backends.constEnd(); ++it)
   {
       QByteArray backend = (*it);
       kDebug() << "Found backend" << backend << "=" << sane->getScannerName(backend);
       hrbackends.append(sane->getScannerName(backend));
   }

   /* allow the user to select one */
   DeviceSelector ds(m_mainWindow, backends, hrbackends);
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
    if (connectedDevice=="") return (i18n("Gallery"));
    if (!haveConnection) return (i18n("No scanner connected"));
    return (sane->getScannerName(connectedDevice));
}


void KookaView::slotSelectionChanged(QRect newSelection)
{
    emit signalRectangleChanged(newSelection.isValid());
}


void KookaView::slotGallerySelectionChanged()
{
    const KFileItem *fi = m_gallery->galleryTree()->currentFileItem();

    if (fi==NULL || fi->isNull())
    {
        emit signalChangeStatusbar(i18n("No selection"));
	emit signalGallerySelectionChanged(false,0);
    }
    else
    {
        emit signalChangeStatusbar(i18n("Gallery %1 %2",
                                        (fi->isDir() ? i18n("folder") : i18n("image")),
                                        fi->url().pathOrUrl()));
	//emit signalGallerySelectionChanged(fi->isDir(), gallery()->selectedItems().count());
        // TODO: can there be more than one selected?
	emit signalGallerySelectionChanged(fi->isDir(), 1);
        if (m_thumbview!=NULL) m_thumbview->slotSelectImage(fi);
    }
}


void KookaView::slotLoadedImageChanged(const KookaImage *img,bool isDir)
{
    if (!isDir && img==NULL) emit signalChangeStatusbar(i18n("Image unloaded"));
    emit signalLoadedImageChanged(img!=NULL, isDir);
}


bool KookaView::galleryRootSelected() const
{
    const FileTreeViewItem *curr = m_gallery->galleryTree()->currentFileTreeViewItem();
    return (curr==NULL ? false : curr->isRoot());
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

    if( printer.setup( m_mainWindow, i18n("Print %1").arg(img->localFileName().section('/', -1)) ))
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

   preview_canvas->newImage(newimg);			// set new image and size
}


void KookaView::slotStartOcrSelection()
{
   emit signalChangeStatusbar(i18n("Starting OCR on selection"));

   KookaImage img;
   if (img_canvas->selectedImage(&img)) startOCR(&img);

   emit signalCleanStatusbar();
}


void KookaView::slotStartOcr()
{
   emit signalChangeStatusbar(i18n("Starting OCR on the image"));

   const KookaImage *img = gallery()->getCurrImage(true);
   startOCR(img);

   emit signalCleanStatusbar();
}


void KookaView::slotOcrSpellCheck()
{
    kDebug();

    emit signalChangeStatusbar(i18n("OCR Spell Check"));

    setCurrentIndex(KookaView::TabOcr);

    if (ocrFabric==NULL || !ocrFabric->engineValid())
    {
        KMessageBox::sorry(m_mainWindow,
                           i18n("OCR has not been performed yet, or the engine has been changed"),
                           i18n("OCR Spell Check not possible"));
        return;
    }

    ocrFabric->performSpellCheck();
    emit signalCleanStatusbar();
}


void KookaView::startOCR(const KookaImage *img)
{
    if (img==NULL || img->isNull()) return;

    setCurrentIndex(KookaView::TabOcr);

    if (ocrFabric!=NULL && !ocrFabric->engineValid())	// exists, but needs to be changed?
    {
        delete ocrFabric;
        ocrFabric = NULL;
    }

    if (ocrFabric==NULL)
    {
        bool gotoPrefs = false;
        ocrFabric = OcrEngine::createEngine(this, &gotoPrefs);
        if (ocrFabric==NULL)
        {
            kDebug() << "Cannot create OCR engine!";
            if (gotoPrefs) emit signalOcrPrefs();
            return;
        }

        ocrFabric->setImageCanvas( img_canvas );

        // Connections OcrEngine --> myself
        connect( ocrFabric, SIGNAL( newOCRResultText( const QString& )),
                 SLOT(slotOcrResultText( const QString& )));
//        connect( ocrFabric, SIGNAL( newOCRResultText( const QString& )),
//                 m_dockOCRText, SLOT( show() ));
	  
        // Connections OcrEngine --> ImageCanvas
        connect( ocrFabric, SIGNAL( repaintOCRResImage( )),
                 img_canvas, SLOT(repaint()));

        // Connections OcrEngine --> OCR Results
        connect( ocrFabric, SIGNAL( clearOCRResultText()),
                 m_ocrResEdit, SLOT(clear()));

        connect( ocrFabric,    SIGNAL( updateWord(int, const QString&, const QString& )),
                 m_ocrResEdit, SLOT( slUpdateOCRResult( int, const QString&, const QString& )));

        connect( ocrFabric,    SIGNAL( ignoreWord(int, const ocrWord&)),
                 m_ocrResEdit, SLOT( slIgnoreWrongWord( int, const ocrWord& )));

        connect( ocrFabric, SIGNAL( markWordWrong(int, const ocrWord& )),
                 m_ocrResEdit, SLOT( slMarkWordWrong( int, const ocrWord& )));

        connect( ocrFabric,    SIGNAL( readOnlyEditor( bool )),
                 m_ocrResEdit, SLOT( setReadOnly( bool )));

        connect( ocrFabric,    SIGNAL( selectWord( int, const ocrWord& )),
                 m_ocrResEdit, SLOT( slSelectWord( int, const ocrWord& )));
    }

    ocrFabric->setImage(img);
    ocrFabric->startOCRVisible(this);
}


void KookaView::slotOcrResultText(const QString &text)
{
    m_ocrResEdit->setText(text);
    emit signalOcrResultAvailable(!text.isEmpty());
}


void KookaView::slotOCRResultImage(const QPixmap &pix)
{
    kDebug();
    if (img_canvas==NULL) return;

    if (m_ocrResultImg!=NULL)
    {
        img_canvas->newImage(NULL);
        delete m_ocrResultImg;
    }

    m_ocrResultImg = new QImage();
    *m_ocrResultImg = pix.toImage();
    img_canvas->newImage(m_ocrResultImg);
    img_canvas->setReadOnly(true);			// OCR result images should be read only
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
    if (isPhotoCopyMode) return;

    KookaImageMeta *meta = new KookaImageMeta;
    meta->setScanResolution(info->getXResolution(), info->getYResolution());
    gallery()->addImage(img, meta);
}


void KookaView::slotScanFinished( KScanStat stat )
{
    kDebug() << "Scan finished with status" << stat;

    if (stat!=KSCAN_OK && stat!=KSCAN_CANCELLED)
    {
        QString msg = i18n("<qt><p>"
                           "There was a problem during preview or scanning."
                           "<br>"
                           "Check that the scanner is still connected and switched on, "
                           "and that media is loaded if required."
                           "<p>"
                           "Trying to use scanner device: <b>%3</b><br>"
                           "SANE reported error: <b>%1</b><br>"
                           "libkscan reported error: <b>%2</b>",
                           sane->lastErrorMessage(),
                           KScanOption::errorMessage(stat),
                           connectedDevice.data());
        KMessageBox::error(m_mainWindow, msg);
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

    sane->slotCloseDevice();
}


void KookaView::slotCreateNewImgFromSelection()
{
   if( img_canvas->rootImage() )
   {
      emit( signalChangeStatusbar( i18n("Create new image from selection" )));
      QImage img;
      if( img_canvas->selectedImage( &img ) )
      {
	 gallery()->addImage( &img );
      }
      emit( signalCleanStatusbar( ));
   }

}


void KookaView::slotRotateImage(int angle)
{
    const KookaImage *img = gallery()->getCurrImage();
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
    const QImage *img = img_canvas->rootImage();
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
    if (m_ocrResEdit!=NULL) m_ocrResEdit->slotSaveText();
}


void KookaView::slotScanParams()
{
    if (sane==NULL) return;				// must have a scanner device
    ScanParamsDialog d(this, sane);
    d.exec();
}


void KookaView::slotShowAImage(const KookaImage *img)
{
    if (img_canvas!=NULL)
    {
        img_canvas->newImage(img);
        img_canvas->setReadOnly(false);
    }

    if (ocrFabric) ocrFabric->setImage(img);		/* tell ocr about it */

    KStatusBar *statBar = m_mainWindow->statusBar();	/* Status Bar */
    if (img_canvas!=NULL) statBar->changeItem(img_canvas->imageInfoString(), KookaView::StatusImage);

    if (img!=NULL) emit signalChangeStatusbar(i18n("Showing image %1", img->url().pathOrUrl()));
}


void KookaView::slotUnloadAImage(const KookaImage *img)
{
    kDebug() << "Unloading Image";
    if (img_canvas!=NULL) img_canvas->newImage(NULL);
}


/* this slot is called when the user clicks on an image in the packager
 * and loading of the image starts
 */
void KookaView::slotStartLoading(const KUrl &url)
{
    emit signalChangeStatusbar(i18n("Loading %1",url.prettyUrl()));
}


void KookaView::updateCurrImage( QImage& img )
{
    if( ! img_canvas->readOnly() )
    {
	emit( signalChangeStatusbar( i18n("Storing image changes" )));
	gallery()->slotCurrentImageChanged( &img );
	emit( signalCleanStatusbar());
    }
    else
    {
	emit( signalChangeStatusbar( i18n("Can not save image, it is write protected!")));
	kDebug() << "Image is write protected, not saving!";
    }
}


void KookaView::slotOpenCurrInGraphApp()
{
    const KFileItem *fi = m_gallery->galleryTree()->currentFileItem();
    if (fi==NULL || fi->isNull()) return;

    KUrl::List urllist(fi->url());
    KRun::displayOpenWithDialog(urllist, m_mainWindow);
}


void KookaView::connectViewerAction(KAction *action)
{
    if (action==NULL) return;
    KMenu *popup = img_canvas->contextMenu();
    if (popup!=NULL) popup->addAction(action);
}


void KookaView::connectGalleryAction(KAction *action)
{
    if (action==NULL) return;
    KMenu *popup = gallery()->contextMenu();
    if (popup!=NULL) popup->addAction(action);
}


void KookaView::connectThumbnailAction(KAction *action)
{
    if (action==NULL) return;
    // TODO: fix this, maybe need the thumbview to keep a list of actions
    //KMenu *popup = m_thumbview->contextMenu();
    //if (popup!=NULL) popup->addAction(action);
}


void KookaView::slotApplySettings()
{
   if (m_thumbview!=NULL) m_thumbview->readSettings();	// size and background
   if (m_gallery!=NULL) m_gallery->readSettings();	// layout and rename
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
    isPhotoCopyMode=true;
    photoCopyPrinter = new KPrinter( true, QPrinter::HighResolution ); 
//    photoCopyPrinter->removeStandardPage( KPrinter::CopiesPage );
    photoCopyPrinter->setUsePrinterResolution(true);
    photoCopyPrinter->setOption( OPT_SCALING, "scan");
    photoCopyPrinter->setFullPage(true);
    slotPhotoCopyScan( KSCAN_OK );
#endif
}


void KookaView::slotPhotoCopyPrint(const QImage *img, const ImgScanInfo *info)
{
    kDebug();

#ifndef KDE4
    if ( ! isPhotoCopyMode ) return;
    KookaImage kooka_img=KookaImage( *img );
    KookaPrint kookaprint( photoCopyPrinter );
    kookaprint.printImage( &kooka_img ,0 );
#endif

}


void KookaView::slotPhotoCopyScan(KScanStat status)
{
    kDebug();

#ifndef KDE4
    if ( ! isPhotoCopyMode ) return;

//    photoCopyPrinter->addDialogPage( new PhotoCopyPrintDialogPage( sane ) );

    KScanOption res ( SANE_NAME_SCAN_RESOLUTION );
    photoCopyPrinter->setOption( OPT_SCAN_RES, res.get() );
    photoCopyPrinter->setMargins(QSize(0,0));
    kDebug() << "Resolution" << res.get();

//    photoCopyPrinter->addDialogPage( new ImgPrintDialog( 0 ) );
    if( photoCopyPrinter->setup( 0, "Photocopy" )) {
        Q_CHECK_PTR( sane );
        mScanParams->slotStartScan( );
    }
    else {
        isPhotoCopyMode=false;
        delete photoCopyPrinter;
    }
#endif
}


ScanGallery *KookaView::gallery() const
{
    return (m_gallery->galleryTree());
}
