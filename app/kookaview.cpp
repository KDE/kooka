/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include "kookaview.h"

#include <kio_version.h>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 69, 0)
#define HAVE_KIO_APPLICATIONLAUNCHERJOB
#endif

#include <qlabel.h>
#include <qlayout.h>
#include <qsplitter.h>
#include <qimage.h>
#include <qapplication.h>
#include <qicon.h>
#include <qaction.h>
#include <qmenu.h>
#include <qprintdialog.h>
#include <qprinter.h>
#include <qfiledialog.h>

#include <kapplicationtrader.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kfileitem.h>
#include <kmainwindow.h>

#ifdef HAVE_KIO_APPLICATIONLAUNCHERJOB
#include <kio/applicationlauncherjob.h>
#include <kio/jobuidelegatefactory.h>
#else
#include <krun.h>
#endif

#include "scandevices.h"
#include "kscandevice.h"
#include "deviceselector.h"
#include "adddevicedialog.h"
#include "previewer.h"
#include "imagecanvas.h"
#include "scanimage.h"

#include "kookapref.h"
#include "galleryhistory.h"
#include "thumbview.h"
#include "abstractocrengine.h"
#include "abstractdestination.h"
#include "pluginmanager.h"
#include "ocrresedit.h"
#include "scanpresetsdialog.h"
#include "kookagallery.h"
#include "kookascanparams.h"
#include "scangallery.h"
#include "imagetransform.h"
#include "statusbarmanager.h"
#include "kookasettings.h"
#include "imagefilter.h"
#include "recentsaver.h"
#include "kooka_logging.h"

#include "imgprintdialog.h"
#include "kookaprint.h"


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
    WidgetSite(QWidget *parent, QWidget *widget = nullptr);
    void setWidget(QWidget *widget);

private:
    static int sCount;
};

int WidgetSite::sCount = 0;

WidgetSite::WidgetSite(QWidget *parent, QWidget *widget)
    : QFrame(parent)
{
    QString name = QString("WidgetSite-#%1").arg(++sCount);
    setObjectName(name.toLocal8Bit());

    setFrameStyle(QFrame::Panel | QFrame::Raised);  // from "scanparams.cpp"
    setLineWidth(1);

    QGridLayout *lay = new QGridLayout(this);
    lay->setRowStretch(0, 1);
    lay->setColumnStretch(0, 1);

    if (widget == nullptr) {
        QLabel *l = new QLabel(name, this);
        l->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        widget = l;
    }

    //qCDebug(KOOKA_LOG) << name
    //         << "widget is a" << widget->metaObject()->className()
    //         << "parent is a" << widget->parent()->metaObject()->className();
    lay->addWidget(widget, 0, 0);
    widget->show();
}

void WidgetSite::setWidget(QWidget *widget)
{
    QGridLayout *lay = static_cast<QGridLayout *>(layout());

    QObjectList childs = children();
    for (QObjectList::iterator it = childs.begin(); it != childs.end(); ++it) {
        QObject *ch = (*it);
        if (ch->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(ch);
            w->hide();
            lay->removeWidget(w);
        }
    }

    //qCDebug(KOOKA_LOG) << objectName()
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
    WidgetSplitter(Qt::Orientation orientation, QWidget *parent = nullptr);
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
    : QTabWidget(parent)
{
    setObjectName("KookaView");

    mMainWindow = parent;
    mScanParams = nullptr;
    mCurrentTab = KookaView::TabNone;

    /** Image Viewer **/
    mImageCanvas = new ImageCanvas(this);
    mImageCanvas->setMinimumSize(100, 200);
    QMenu *ctxtmenu = mImageCanvas->contextMenu();
    if (ctxtmenu != nullptr) {
        ctxtmenu->addSection(i18n("Image View"));
    }

    // Connections ImageCanvas --> myself
    connect(mImageCanvas, QOverload<const QRect &>::of(&ImageCanvas::newRect), this, &KookaView::slotSelectionChanged);

    /** Thumbnail View **/
    mThumbView = new ThumbView(this);

    /** Scan Gallery **/
    mGallery = new KookaGallery(this);
    ScanGallery *packager = mGallery->galleryTree();

    // Connections ScanGallery --> myself
    connect(packager, &ScanGallery::itemHighlighted, this, &KookaView::slotGallerySelectionChanged);
    connect(packager, &ScanGallery::showImage, this, &KookaView::slotShowImage);
    connect(packager, &ScanGallery::aboutToShowImage, this, &KookaView::slotStartLoading);
    connect(packager, &ScanGallery::unloadImage, this, &KookaView::slotUnloadImage);

    // Connections ScanGallery --> ThumbView
    connect(packager, &ScanGallery::itemHighlighted, mThumbView, &ThumbView::slotHighlightItem);
    connect(packager, &ScanGallery::imageChanged, mThumbView, &ThumbView::slotImageChanged);
    connect(packager, &ScanGallery::fileRenamed, mThumbView, &ThumbView::slotImageRenamed);

    // Connections ThumbView --> ScanGallery
    connect(mThumbView, &ThumbView::itemHighlighted, packager, &ScanGallery::slotHighlightItem);
    connect(mThumbView, &ThumbView::itemActivated, packager, &ScanGallery::slotActivateItem);

    GalleryHistory *recentFolder = mGallery->galleryRecent();

    // Connections ScanGallery <--> recent folder history
    connect(packager, &ScanGallery::galleryPathChanged, recentFolder, &GalleryHistory::slotPathChanged);
    connect(packager, &ScanGallery::galleryDirectoryRemoved, recentFolder, &GalleryHistory::slotPathRemoved);
    connect(recentFolder, &GalleryHistory::pathSelected, packager, &ScanGallery::slotSelectDirectory);

    /** Scanner Settings **/

    if (!deviceToUse.isEmpty() && deviceToUse != "gallery") {
        ScanDevices::self()->addUserSpecifiedDevice(deviceToUse,
                                                    i18n("User specified"),
                                                    i18n("on command line"),
                                                    "",
                                                    true);
    }

    /** Scanner Device **/
    mScanDevice = new KScanDevice(this);

    // Connections KScanDevice --> myself
    connect(mScanDevice, &KScanDevice::sigNewImage, this, &KookaView::slotNewImageScanned);
    connect(mScanDevice, &KScanDevice::sigNewPreview, this, &KookaView::slotNewPreview);
    connect(mScanDevice, &KScanDevice::sigScanStart, this, &KookaView::slotScanStart);
    connect(mScanDevice, &KScanDevice::sigScanFinished, this, &KookaView::slotScanFinished);

    /** Scan Preview **/
    mPreviewCanvas = new Previewer(this);
    mPreviewCanvas->setMinimumSize(100, 100);
    ctxtmenu = mPreviewCanvas->getImageCanvas()->contextMenu();
    if (ctxtmenu != nullptr) {
        ctxtmenu->addSection(i18n("Scan Preview"));
    }

    // Connections Previewer --> myself
    connect(mPreviewCanvas, &Previewer::previewDimsChanged, this, &KookaView::slotPreviewDimsChanged);

    /** Ocr Result Text **/
    mOcrResEdit  = new OcrResEdit(this);
    mOcrResEdit->setAcceptRichText(false);
    mOcrResEdit->setWordWrapMode(QTextOption::NoWrap);
    mOcrResEdit->setPlaceholderText(i18n("OCR results will appear here"));
    connect(mOcrResEdit, &OcrResEdit::spellCheckStatus, this, [this](const QString &msg){ changeStatus(msg); });

    /** Tabs **/
    // TODO: not sure which tab position is best, make this configurable
    setTabPosition(QTabWidget::West);
    setTabsClosable(false);

    mScanPage = new WidgetSplitter(Qt::Horizontal, this);
    addTab(mScanPage, QIcon::fromTheme("scan"), i18n("Scan"));

    mGalleryPage = new WidgetSplitter(Qt::Horizontal, this);
    addTab(mGalleryPage, QIcon::fromTheme("image-x-generic"), i18n("Gallery"));

    mOcrPage = new WidgetSplitter(Qt::Vertical, this);
    addTab(mOcrPage, QIcon::fromTheme("ocr"), i18n("OCR"));

    connect(this, &QTabWidget::currentChanged, this, &KookaView::slotTabChanged);

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
    mScanSubSplitter->addWidget(mScanGallerySite);          // TL
    mScanSubSplitter->addWidget(mParamsSite);               // BL
    mScanPage->addWidget(new WidgetSite(this, mPreviewCanvas));     // R

    // "Gallery" page: gallery left, viewer top right, thumbnails bottom right
    mGalleryPage->addWidget(mGalleryGallerySite);           // L
    mGallerySubSplitter = new WidgetSplitter(Qt::Vertical, mGalleryPage);
    mGallerySubSplitter->addWidget(mGalleryImgviewSite);        // TR
    mGallerySubSplitter->addWidget(new WidgetSite(this, mThumbView));   // BR

    // "OCR" page: gallery top left, viewer top right, results bottom
    mOcrSubSplitter = new WidgetSplitter(Qt::Horizontal, mOcrPage);
    mOcrSubSplitter->addWidget(mOcrGallerySite);            // TL
    mOcrSubSplitter->addWidget(mOcrImgviewSite);            // TR
    mOcrPage->addWidget(new WidgetSite(this, mOcrResEdit));     // B

    if (slotSelectDevice(deviceToUse, false)) {
        slotTabChanged(KookaView::TabScan);
    } else {
        setCurrentIndex(KookaView::TabGallery);
    }
}

KookaView::~KookaView()
{
    delete mScanDevice;
}

// this gets called via Kooka::closeEvent() at shutdown
void KookaView::saveWindowSettings(KConfigGroup &grp)
{
    qCDebug(KOOKA_LOG) << "to group" << grp.name();
    KookaSettings::setLayoutTabIndex(currentIndex());
    KookaSettings::setLayoutScan1(mScanPage->saveState().toBase64());
    KookaSettings::setLayoutScan2(mScanSubSplitter->saveState().toBase64());
    KookaSettings::setLayoutGallery1(mGalleryPage->saveState().toBase64());
    KookaSettings::setLayoutGallery2(mGallerySubSplitter->saveState().toBase64());
    KookaSettings::setLayoutOcr1(mOcrPage->saveState().toBase64());
    KookaSettings::setLayoutOcr2(mOcrSubSplitter->saveState().toBase64());

    saveGalleryState();					// for the current tab
    KookaSettings::self()->save();
}

// this gets called by Kooka::applyMainWindowSettings() at startup
void KookaView::applyWindowSettings(const KConfigGroup &grp)
{
    qCDebug(KOOKA_LOG) << "from group" << grp.name();

    QString set = KookaSettings::layoutScan1();
    if (!set.isEmpty()) mScanPage->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));
    set = KookaSettings::layoutScan2();
    if (!set.isEmpty()) mScanSubSplitter->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));

    set = KookaSettings::layoutGallery1();
    if (!set.isEmpty()) mGalleryPage->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));
    set = KookaSettings::layoutGallery2();
    if (!set.isEmpty()) mGallerySubSplitter->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));

    set = KookaSettings::layoutOcr1();
    if (!set.isEmpty()) mOcrPage->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));
    set = KookaSettings::layoutOcr2();
    if (!set.isEmpty()) mOcrSubSplitter->restoreState(QByteArray::fromBase64(set.toLocal8Bit()));
}

void KookaView::saveGalleryState(int index) const
{
    if (index == -1) index = currentIndex();
    gallery()->saveHeaderState(index);
}

void KookaView::restoreGalleryState(int index)
{
    if (index == -1) index = currentIndex();
    gallery()->restoreHeaderState(index);
}

void KookaView::slotTabChanged(int index)
{
    //qCDebug(KOOKA_LOG) << index;
    if (mCurrentTab != KookaView::TabNone) {
        saveGalleryState(mCurrentTab);
    }
    // save state of previous tab
    switch (index) {
    case KookaView::TabScan:                // Scan
        mScanGallerySite->setWidget(mGallery);
        mGalleryImgviewSite->setWidget(mImageCanvas);   // somewhere to park it
        emit clearStatus(StatusBarManager::ImageDims);
        break;

    case KookaView::TabGallery:             // Gallery
        mGalleryGallerySite->setWidget(mGallery);
        mGalleryImgviewSite->setWidget(mImageCanvas);
        emit clearStatus(StatusBarManager::PreviewDims);
        break;

    case KookaView::TabOcr:                 // OCR
        mOcrGallerySite->setWidget(mGallery);
        mOcrImgviewSite->setWidget(mImageCanvas);
        emit clearStatus(StatusBarManager::PreviewDims);
        break;
    }

    restoreGalleryState(index);             // restore state of new tab
    mCurrentTab = static_cast<KookaView::TabPage>(index);
    // note for next tab change
    updateSelectionState();             // update image action states
}

bool KookaView::slotSelectDevice(const QByteArray &useDevice, bool alwaysAsk)
{
    qCDebug(KOOKA_LOG) << "device" << useDevice << "ask" << alwaysAsk;

    bool haveConnection = false;
    bool gallery_mode = (useDevice == "gallery");
    QByteArray selDevice;

    /* in case useDevice is the term 'gallery', the user does not want to
     * connect to a scanner, but only work in gallery mode. Otherwise, try
     * to read the device to use from config or from a user dialog */
    if (!gallery_mode) {
        selDevice =  useDevice;
        if (selDevice.isEmpty()) {
            selDevice = userDeviceSelection(alwaysAsk);
        }

        if (selDevice.isEmpty()) {          // dialogue cancelled
            if (mScanParams != nullptr) {
                return (false);    // have setup, do nothing
            }
            gallery_mode = true;
        }
    }

    if (mScanParams != nullptr) {
        closeScanDevice();    // remove existing GUI object
    }

    mScanParams = new KookaScanParams(this);        // and create a new one
    connect(mScanParams, &KookaScanParams::actionSelectScanner, this, [this](){ slotSelectDevice(); });
    connect(mScanParams, &KookaScanParams::actionAddScanner, this, &KookaView::slotAddDevice);

    mParamsSite->setWidget(mScanParams);

    if (!selDevice.isEmpty()) {             // connect to the selected scanner
        while (!haveConnection) {
            qCDebug(KOOKA_LOG) << "Opening device" << selDevice;
            KScanDevice::Status stat = mScanDevice->openDevice(selDevice);
            if (stat == KScanDevice::Ok) {
                haveConnection = true;
            } else {
                QString msg = xi18nc("@info",
                                     "There was a problem opening the scanner device."
                                     "<nl/><nl/>"
                                     "Check that the scanner is connected and switched on, "
                                     "and that SANE support for it is correctly configured."
                                     "<nl/><nl/>"
                                     "Trying to use scanner device: <icode>%3</icode><nl/>"
                                     "libkookascan reported error: <message>%2</message><nl/>"
                                     "SANE reported error: <message>%1</message>",
                                     mScanDevice->lastSaneErrorMessage(),
                                     KScanDevice::statusMessage(stat),
                                     selDevice.constData());

                int tryAgain = KMessageBox::warningContinueCancel(mMainWindow, msg, QString(),
                               KGuiItem("Retry"));
                if (tryAgain == KMessageBox::Cancel) {
                    break;
                }
            }
        }

        if (haveConnection) {           // scanner connected OK
            QSize s = mScanDevice->getMaxScanSize();    // fix for 160148
            mPreviewCanvas->setScannerBedSize(s.width(), s.height());

            // Connections ScanParams --> Previewer
            connect(mScanParams, &ScanParams::scanResolutionChanged, mPreviewCanvas, &Previewer::slotNewScanResolutions);
            connect(mScanParams, &ScanParams::scanModeChanged, mPreviewCanvas, &Previewer::slotNewScanMode);
            connect(mScanParams, &ScanParams::newCustomScanSize, mPreviewCanvas, &Previewer::slotNewCustomScanSize);

            // Connections Previewer --> ScanParams
            connect(mPreviewCanvas, &Previewer::newPreviewRect, mScanParams, &ScanParams::slotNewPreviewRect);

            mScanParams->connectDevice(mScanDevice);    // create scanning options

            // Load the saved preview image, if there is one
            ScanImage *previewImage = new ScanImage(mScanDevice->loadPreviewImage());
            mPreviewCanvas->setPreviewImage(ScanImage::Ptr(previewImage));
            // Load auto-selection options for the selected scanner
            mPreviewCanvas->connectScanner(mScanDevice);
        }
    }

    if (!haveConnection) {              // no scanner device available,
        // or starting in gallery mode
        if (mScanParams != nullptr) {
            mScanParams->connectDevice(nullptr, gallery_mode);
        }
    }

    emit signalScannerChanged(haveConnection);
    return (haveConnection);
}

void KookaView::slotAddDevice()
{
    AddDeviceDialog d(mMainWindow, i18n("Add Scan Device"));
    if (!d.exec()) return;

    QByteArray dev = d.getDevice();
    QString manuf = d.getManufacturer();
    QString dsc = d.getDescription();
    QByteArray type = d.getType();
    // TODO: no need to log, the function called below does that
    qCDebug(KOOKA_LOG) << "dev" << dev << "manuf" << manuf << "type" << type << "desc" << dsc;

    ScanDevices::self()->addUserSpecifiedDevice(dev, manuf, dsc, type);
}

QByteArray KookaView::userDeviceSelection(bool alwaysAsk)
{
    /* a list of backends the scan backend knows */
    QList<QByteArray> backends = ScanDevices::self()->allDevices();
    if (backends.count() == 0) {
        if (KMessageBox::warningContinueCancel(mMainWindow,
                                               xi18nc("@info",
                                                      "No scanner devices are available."
                                                      "<nl/><nl/>"
                                                      "If your scanner is a type that can be auto-detected by SANE, "
                                                      "check that it is connected, switched on and configured correctly."
                                                      "<nl/><nl/>"
                                                      "If the scanner cannot be auto-detected by SANE (this includes some network scanners), "
                                                      "you need to specify the device to use. "
                                                      "Use the <interface>Add Scan Device</interface> option to enter the backend name and parameters, "
                                                      "or see that dialog for more information."),
                                               QString(),
                                               KGuiItem(i18n("Add Scan Device..."))) != KMessageBox::Continue)
        {
            return ("");
        }

        slotAddDevice();
        backends = ScanDevices::self()->allDevices();    // refresh the list
        if (backends.count() == 0) {
            return ("");    // give up this time
        }
    }

    QByteArray selDevice;
    DeviceSelector ds(mMainWindow, backends,
                      (alwaysAsk ? KGuiItem() : KGuiItem(i18n("Gallery"), QIcon::fromTheme("image-x-generic"))));
    if (!alwaysAsk) {
        selDevice = ds.getDeviceFromConfig();
    }

    if (selDevice.isEmpty()) {
        qCDebug(KOOKA_LOG) << "no selDevice, starting selector";
        if (ds.exec() == QDialog::Accepted) {
            selDevice = ds.getSelectedDevice();
        }
        qCDebug(KOOKA_LOG) << "selector returned device" << selDevice;
    }

    return (selDevice);
}

QString KookaView::scannerName() const
{
    if (mScanDevice->scannerBackendName().isEmpty()) {
        return (i18n("Gallery"));
    }
    if (!mScanDevice->isScannerConnected()) {
        return (i18n("No scanner connected"));
    }
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
    KookaView::StateFlags state = KookaView::StateFlags();
    const FileTreeViewItem *ftvi = mGallery->galleryTree()->highlightedFileTreeViewItem();
    const KFileItem *fi = (ftvi != nullptr ? ftvi->fileItem() : nullptr);
    const bool scanmode = (mCurrentTab == KookaView::TabScan);

    if (scanmode)					// Scan mode
    {
        if (mPreviewCanvas->getImageCanvas()->hasImage()) state |= KookaView::PreviewValid;
    }
    else						// Gallery/OCR mode
    {
        state |= KookaView::GalleryShown;
    }

    if (fi != nullptr && !fi->isNull())			// have a gallery selection
    {
        if (fi->isDir()) state |= KookaView::IsDirectory;
        else
        {
            state |= KookaView::FileSelected;
            if (fi->url().hasFragment()) state |= KookaView::IsSubImage;
            if (ftvi->childCount()>0) state |= KookaView::HasSubImages;
        }
    }

    if (ftvi != nullptr && ftvi->isRoot()) state |= KookaView::RootSelected;
    if (imageViewer()->hasImage()) state |= KookaView::ImageValid;

    //qCDebug(KOOKA_LOG) << "state" << state;
    emit signalViewSelectionState(state);
}

void KookaView::slotGallerySelectionChanged()
{
    const FileTreeViewItem *ftvi = mGallery->galleryTree()->highlightedFileTreeViewItem();
    const KFileItem *fi = (ftvi != nullptr ? ftvi->fileItem() : nullptr);
    const bool scanmode = (mCurrentTab == KookaView::TabScan);

    if (fi == nullptr || fi->isNull()) {       // no gallery selection
        if (!scanmode) {
            emit changeStatus(i18n("No selection"));
        }
    } else {                    // have a gallery selection
        if (!scanmode) {
            KLocalizedString str = fi->isDir() ? ki18n("Gallery folder %1") : ki18n("Gallery image %1");
            emit changeStatus(str.subs(fi->url().url(QUrl::PreferLocalFile)).toString());
        }
    }

    updateSelectionState();
}

void KookaView::loadStartupImage()
{
    const bool wantReadOnStart = KookaSettings::startupReadImage();
    if (wantReadOnStart) {
        QString startup = KookaSettings::startupSelectedImage();
        qCDebug(KOOKA_LOG) << "load startup image" << startup;
        if (!startup.isEmpty()) gallery()->slotSelectImage(QUrl::fromLocalFile(startup));
    }
}

void KookaView::print()
{
    ScanImage::Ptr img = gallery()->getCurrImage(true);
    if (img.isNull()) return;				// load image if necessary

    // create a KookaPrint (subclass of a QPrinter)
    KookaPrint printer;
    printer.setImage(img.data());

    QPrintDialog d(&printer, this);
    d.setWindowTitle(i18nc("@title:window", "Print Image"));
    d.setOptions(QAbstractPrintDialog::PrintToFile|QAbstractPrintDialog::PrintShowPageSize);

    // TODO (investigate): even with the options set as above, the options below still
    // appear in the print dialogue.  Is this as intended by Qt?
    //     d.setOption(QAbstractPrintDialog::PrintSelection, false);
    //     d.setOption(QAbstractPrintDialog::PrintPageRange, false);

    ImgPrintDialog imgTab(img, &printer);		// create tab for our options
    d.setOptionTabs(QList<QWidget *>() << &imgTab);	// add tab to print dialogue

    if (!d.exec()) return;				// open the dialogue
    QString msg = imgTab.checkValid();			// check that settings are valid
    if (!msg.isEmpty())					// if not, display error message
    {
        KMessageBox::error(this,
                           i18nc("@info", "Invalid print options were specified:\n\n%1", msg),
                           i18nc("@title:window", "Cannot Print"));
        return;
    }

    imgTab.updatePrintParameters();			// set final printer options
    printer.printImage();				// print the image
}


void KookaView::slotNewPreview(ScanImage::Ptr newimg)
{
    if (newimg.isNull()) return;

    mPreviewCanvas->newImage(newimg);			// set new image and size
    updateSelectionState();
}


void KookaView::slotStartOcrSelection()
{
    emit changeStatus(i18n("Starting OCR on selection"));
    startOCR(mImageCanvas->selectedImage());
}

void KookaView::slotStartOcr()
{
    emit changeStatus(i18n("Starting OCR on the image"));
    startOCR(gallery()->getCurrImage(true));
}

void KookaView::slotStartOcrFile()
{
    RecentSaver saver("ocrFile");
    QUrl url = QFileDialog::getOpenFileUrl(this, i18n("OCR File"),
                                           saver.recentUrl(),
                                           ImageFilter::qtFilterString(ImageFilter::Reading, ImageFilter::AllImages|ImageFilter::AllFiles));
    if (!url.isValid()) return;
    saver.save(url);

    ScanImage::Ptr img(new ScanImage(url));
    if (!img->isFileBound())
    {
        KMessageBox::error(mMainWindow,
                           xi18nc("@info", "Cannot load <filename>%1</filename> for OCR:<nl/>%2",
                                  url.toDisplayString(), img->errorString()),
                           i18n("Cannot Read OCR File"));
        return;
    }

    emit changeStatus(xi18nc("@info", "Starting OCR on file <filename>%1</filename>", url.url(QUrl::PreferLocalFile)));
    startOCR(img);
}

void KookaView::slotSetOcrSpellConfig(const QString &configFile)
{
#ifndef KF5
    qCDebug(KOOKA_LOG) << configFile;
    if (mOcrResEdit!=nullptr) mOcrResEdit->setSpellCheckingConfigFileName(configFile);
#endif
}

void KookaView::slotOcrSpellCheck(bool interactive, bool background)
{
    if (!interactive && !background)			// not doing anything
    {
        emit changeStatus(i18n("OCR finished"));
        return;
    }

    if (mOcrResEdit->document()->isEmpty())
    {
        KMessageBox::error(mMainWindow,
                           i18n("There is no OCR result text to spell check."),
                           i18n("OCR Spell Check not possible"));
        return;
    }

    setCurrentIndex(KookaView::TabOcr);
    emit changeStatus(i18n("OCR Spell Check"));
    if (background) mOcrResEdit->setCheckSpellingEnabled(true);
    if (interactive) mOcrResEdit->checkSpelling();
}


void KookaView::startOCR(ScanImage::Ptr img)
{
    if (img.isNull()) return;				// no image to OCR

    setCurrentIndex(KookaView::TabOcr);

    const QString engineName = KookaSettings::ocrEngineName();
    if (engineName.isEmpty())
    {
        int result = KMessageBox::warningContinueCancel(mMainWindow,
                                                        i18n("No OCR engine is configured.\n"
                                                             "Please select and configure one in order to perform OCR."),
                                                        i18n("OCR Not Configured"),
                                                        KGuiItem(i18n("Configure OCR...")));
        if (result==KMessageBox::Continue) emit signalOcrPrefs();
        return;
    }

    AbstractOcrEngine *engine = qobject_cast<AbstractOcrEngine *>(PluginManager::self()->loadPlugin(PluginManager::OcrPlugin, engineName));
    if (engine==nullptr)
    {
        KMessageBox::error(mMainWindow, i18n("Cannot load OCR plugin '%1'", engineName));
        return;
    }

    // We don't know whether the plugin object has been used before.
    // So disconnect all of its existing signals so that they do not
    // get double connected.
    engine->disconnect();

    // Connections OCR Engine --> myself
    connect(engine, &AbstractOcrEngine::newOCRResultText, this, &KookaView::slotOcrResultAvailable);
    connect(engine, &AbstractOcrEngine::setSpellCheckConfig, this, &KookaView::slotSetOcrSpellConfig);
    connect(engine, &AbstractOcrEngine::startSpellCheck, this, &KookaView::slotOcrSpellCheck);
    connect(engine, &AbstractOcrEngine::openOcrPrefs, this, &KookaView::signalOcrPrefs);

    // Connections OCR Results --> OCR Engine
    connect(mOcrResEdit, &OcrResEdit::highlightWord, engine, &AbstractOcrEngine::slotHighlightWord);
    connect(mOcrResEdit, &OcrResEdit::scrollToWord, engine, &AbstractOcrEngine::slotScrollToWord);

    // Connections OCR Engine --> OCR Results
    connect(engine, &AbstractOcrEngine::readOnlyEditor, mOcrResEdit, &OcrResEdit::slotSetReadOnly);
    connect(engine, &AbstractOcrEngine::selectWord, mOcrResEdit, &OcrResEdit::slotSelectWord);

    engine->setImageCanvas(mImageCanvas);
    engine->setTextDocument(mOcrResEdit->document());
    engine->setImage(img);
    engine->openOcrDialogue(this);
}


void KookaView::slotOcrResultAvailable()
{
    emit signalOcrResultAvailable(mOcrResEdit->document()->characterCount()>0);
    updateSelectionState();
}


void KookaView::slotScanStart(ScanImage::ImageType type)
{
    // The scan parameters GUI must exist here, in order to locate the
    // destination plugin which is loaded and managed by KookaScanParams.
    AbstractDestination *dest = (mScanParams!=nullptr) ? mScanParams->destinationPlugin() : nullptr;
    if (dest==nullptr)
    {
        qCWarning(KOOKA_LOG) << "No destination plugin";
        mScanDevice->slotStopScanning();		// no point starting scan
        return;
    }

    // Allow the plugin to find the scan gallery, if it needs to send
    // the scan output to there.
    dest->setScanGallery(gallery());

    // Tell the plugin that a scan is starting.
    if (type!=ScanImage::Preview)
    {
        if (!dest->scanStarting(type))			// get ready to save
        {						// user cancelled file prompt
            mScanDevice->slotStopScanning();		// abort the scan now
            return;
        }
    }

    // Set the destination string displayed in the "Scan in Progress" dialogue
    if (type!=ScanImage::Preview)
    {
        mScanParams->setScanDestination(dest->scanDestinationString());
    }
}


void KookaView::slotNewImageScanned(ScanImage::Ptr img)
{
    AbstractDestination *dest = (mScanParams!=nullptr) ? mScanParams->destinationPlugin() : nullptr;
    if (dest!=nullptr) dest->imageScanned(img);
    else qCWarning(KOOKA_LOG) << "No destination plugin";
}


void KookaView::slotScanFinished(KScanDevice::Status stat)
{
    qCDebug(KOOKA_LOG) << "Scan finished with status" << stat;

    if (stat!=KScanDevice::Ok && stat!=KScanDevice::Cancelled && stat!=KScanDevice::AdfEmpty)
    {
        QString msg = xi18nc("@info",
                             "There was a problem during preview or scanning."
                             "<nl/>"
                             "Check that the scanner is still connected and switched on, "
                             "<nl/>"
                             "and that media is loaded if required."
                             "<nl/><nl/>"
                             "Trying to use scanner device: <icode>%3</icode><nl/>"
                             "libkookascan reported error: <message>%2</message><nl/>"
                             "SANE reported error: <message>%1</message>",
                             mScanDevice->lastSaneErrorMessage(),
                             KScanDevice::statusMessage(stat),
                             mScanDevice->scannerBackendName().constData());
        KMessageBox::error(mMainWindow, msg);
    }
}


void KookaView::closeScanDevice()
{
    qCDebug(KOOKA_LOG);

    if (mScanParams!=nullptr)
    {
        // First save the current destination plugin settings
        mScanParams->saveDestinationSettings();
        // Then ensure that the current destination plugin is unloaded
        PluginManager::self()->loadPlugin(PluginManager::DestinationPlugin, QString());
        // Then the scan parameters GUI can be destroyed
        delete mScanParams;
        mScanParams = nullptr;
    }

    mScanDevice->closeDevice();
}


void KookaView::slotCreateNewImgFromSelection()
{
    ScanImage::Ptr img = mImageCanvas->selectedImage();
    if (img.isNull()) return;				// no selection

    emit changeStatus(i18n("Create new image from selection"));
    gallery()->addImage(img);
    emit clearStatus();
}


void KookaView::slotTransformImage()
{
    if (imageViewer()->isReadOnly()) {
        emit changeStatus(i18n("Cannot transform, image is read-only"));
        return;
    }

    // Get operation code from action that was triggered
    QAction *act = static_cast<QAction *>(sender());
    if (act==nullptr) return;
    ImageTransform::Operation op = static_cast<ImageTransform::Operation>(act->data().toInt());

    // This may appear to be a waste, since we are immediately unloading the image.
    // But we need a copy of the image anyway, so there will still be a load needed.
    ScanImage::Ptr loadedImage = gallery()->getCurrImage(true);
    if (loadedImage.isNull()) return;

    const QImage img = *loadedImage.data();		// get a copy of the image

    QString imageFile = gallery()->currentImageFileName();
    if (imageFile.isEmpty()) return;			// get file to save back to
    gallery()->slotUnloadItems();			// unload original from memory

    ImageTransform *transformer = new ImageTransform(img, op, imageFile, this);
    connect(transformer, &ImageTransform::done, gallery(), &ScanGallery::slotUpdatedItem);
    connect(transformer, &ImageTransform::statusMessage, this, [this](const QString &msg){ changeStatus(msg); });
    connect(transformer, &QThread::finished, transformer, &QObject::deleteLater);
    transformer->start();
}


void KookaView::slotSaveOcrResult()
{
    if (mOcrResEdit != nullptr) {
        mOcrResEdit->slotSaveText();
    }
}


void KookaView::slotScanParams()
{
    if (mScanDevice == nullptr) return;			// must have a scanner device
    ScanPresetsDialog d(mScanDevice, this);
    d.exec();
}


void KookaView::slotShowImage(ScanImage::Ptr img, bool isDir)
{
    if (mImageCanvas!=nullptr)				// load into image viewer
    {
        mImageCanvas->newImage(img);
        mImageCanvas->setReadOnly(false);
        emit changeStatus(mImageCanvas->imageInfoString(), StatusBarManager::ImageDims);
    }

    if (!img.isNull()) emit changeStatus(i18n("Loaded image %1", img->url().url(QUrl::PreferLocalFile)));
    else if (!isDir) emit changeStatus(i18n("Unloaded image"));
    updateSelectionState();
}


void KookaView::slotUnloadImage()
{
    if (mImageCanvas!=nullptr) mImageCanvas->newImage(nullptr);
    updateSelectionState();
}


/* this slot is called when the user clicks on an image in the packager
 * and loading of the image starts
 */
void KookaView::slotStartLoading(const QUrl &url)
{
    emit changeStatus(i18n("Loading %1...", url.url(QUrl::PreferLocalFile)));
}

void KookaView::connectViewerAction(QAction *action, bool sepBefore)
{
    if (action == nullptr) {
        return;
    }
    QMenu *popup = mImageCanvas->contextMenu();
    if (popup == nullptr) {
        return;
    }

    if (sepBefore) {
        popup->addSeparator();
    }
    popup->addAction(action);
}

void KookaView::connectGalleryAction(QAction *action, bool sepBefore)
{
    if (action == nullptr) {
        return;
    }
    QMenu *popup = gallery()->contextMenu();
    if (popup == nullptr) {
        return;
    }

    if (sepBefore) {
        popup->addSeparator();
    }
    popup->addAction(action);
}

void KookaView::connectThumbnailAction(QAction *action)
{
    if (action == nullptr) {
        return;
    }
    QMenu *popup = mThumbView->contextMenu();
    if (popup != nullptr) {
        popup->addAction(action);
    }
}

void KookaView::connectPreviewAction(QAction *action)
{
    if (action == nullptr) {
        return;
    }
    QMenu *popup = mPreviewCanvas->getImageCanvas()->contextMenu();
    if (popup != nullptr) {
        popup->addAction(action);
    }
}

void KookaView::slotApplySettings()
{
    if (mThumbView != nullptr) {
        mThumbView->readSettings();    // size and background
    }
    if (mGallery != nullptr) {
        mGallery->readSettings();    // layout and rename
    }
}

// Starting a scan or preview, switch tab and tell the scan device

void KookaView::slotStartPreview()
{
    if (mScanParams == nullptr) {
        return;
    }
    setCurrentIndex(KookaView::TabScan);
    qApp->processEvents();              // let the tab appear
    mScanParams->slotAcquirePreview();
}

void KookaView::slotStartFinalScan()
{
    if (mScanParams == nullptr) {
        return;
    }
    setCurrentIndex(KookaView::TabScan);
    qApp->processEvents();              // let the tab appear
    mScanParams->slotStartScan();
}

void KookaView::slotAutoSelect(bool on)
{
    if (mPreviewCanvas == nullptr) {
        return;
    }
    setCurrentIndex(KookaView::TabScan);
    qApp->processEvents();              // let the tab appear
    mPreviewCanvas->slotAutoSelToggled(on);
}


ScanGallery *KookaView::gallery() const
{
    return (mGallery->galleryTree());
}

ImageCanvas *KookaView::imageViewer() const
{
    return (mImageCanvas);
}

Previewer *KookaView::previewer() const
{
    return (mPreviewCanvas);
}

void KookaView::imageViewerAction(ImageCanvas::UserAction act)
{
    if (mCurrentTab == KookaView::TabScan) {		// Scan
        mPreviewCanvas->getImageCanvas()->performUserAction(act);
    } else {						// Gallery or OCR
        mImageCanvas->performUserAction(act);
    }
}

void KookaView::showOpenWithMenu(KActionMenu *menu)
{
    FileTreeViewItem *curr = gallery()->highlightedFileTreeViewItem();
    QString mimeType = curr->fileItem()->mimetype();
    //qCDebug(KOOKA_LOG) << "Trying to open" << curr->url() << "which is" << mimeType;

    menu->menu()->clear();

    int i = 0;
    mOpenWithOffers = KApplicationTrader::queryByMimeType(mimeType);
    for (const KService::Ptr &service : qAsConst(mOpenWithOffers))
    {
        //qCDebug(KOOKA_LOG) << "> offer:" << service->name();
        QString actionName(service->name().replace("&", "&&"));
        QAction *act = new QAction(QIcon::fromTheme(service->icon()), actionName, this);
        connect(act, &QAction::triggered, this, [=]() { slotOpenWith(i); });
        menu->addAction(act);
    }

    menu->menu()->addSeparator();
    QAction *act = new QAction(i18n("Other..."), this);
    connect(act, &QAction::triggered, this, [=]() { slotOpenWith(-1); });
    menu->addAction(act);
}

void KookaView::slotOpenWith(int idx)
{
    FileTreeViewItem *ftvi = gallery()->highlightedFileTreeViewItem();
    if (ftvi == nullptr) return;

    QList<QUrl> urllist;
    urllist.append(ftvi->url());

#ifdef HAVE_KIO_APPLICATIONLAUNCHERJOB
    KIO::ApplicationLauncherJob *job;
#endif
    if (idx!=-1)			 		// application from the menu
    {
        Q_ASSERT(idx<mOpenWithOffers.count());
        KService::Ptr ptr = mOpenWithOffers[idx];
#ifdef HAVE_KIO_APPLICATIONLAUNCHERJOB
        job = new KIO::ApplicationLauncherJob(ptr, mMainWindow);
#else
        KRun::runService(*ptr, urllist, mMainWindow);
#endif
    }
    else    						// last item = "Other..."
    {
#ifdef HAVE_KIO_APPLICATIONLAUNCHERJOB
        job = new KIO::ApplicationLauncherJob(mMainWindow);
#else
        KRun::displayOpenWithDialog(urllist, mMainWindow);
#endif
    }
#ifdef HAVE_KIO_APPLICATIONLAUNCHERJOB
    job->setUrls(urllist);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    job->start();
#endif
}


void KookaView::slotPreviewDimsChanged(const QString &dims)
{
    emit changeStatus(dims, StatusBarManager::PreviewDims);
}
