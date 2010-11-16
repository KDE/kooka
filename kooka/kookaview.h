/***************************************************** -*- mode:c++; -*- ***
                          kookaview.h  -  Main view
                             -------------------
    begin                : Sun Jan 16 2000
    copyright            : (C) 2000 by Klaas Freitag
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

#ifndef KOOKAVIEW_H
#define KOOKAVIEW_H

#include <ktabwidget.h>
#include <kmimetypetrader.h>

#include "libkscan/kscandevice.h"


class QPainter;
class QPixmap;
class QSplitter;
class QSignalMapper;

class KConfigGroup;
class KPrinter;
class KAction;
class KActionCollection;
class KMainWindow;
class KUrl;
class KActionMenu;

class OcrEngine;
class ThumbView;
class KookaImage;
class KookaGallery;
class OcrResEdit;
class ScanGallery;
class KookaScanParams;
class ScanParams;
class ImgScanInfo;
class Previewer;
class KScanDevice;
class ImageCanvas;

class WidgetSite;



/**
 * This is the main view class for Kooka.  Most of the non-menu,
 * non-toolbar, and non-statusbar (e.g., non frame) GUI code should go
 * here.
 *
 * @short Main view
 * @author Klaas Freitag <freitag@suse.de>
 * @version 0.1
 */
class KookaView : public KTabWidget
{
    Q_OBJECT

public:
    enum MirrorType { MirrorVertical, MirrorHorizontal, MirrorBoth};
    enum StatusBarIDs { StatusTemp, StatusImage };
    enum TabPage { TabScan = 0, TabGallery = 1, TabOcr = 2, TabNone = 0xFF };

    /**
     * To avoid a proliferation of boolean parameters, these flags are
     * used to indicate the state of the gallery and image viewers.
     */
    enum StateFlag
    {
        GalleryShown = 0x01,				// in Gallery/OCR mode
        PreviewValid = 0x02,				// scan preview valid
        ImageValid   = 0x04,				// viewer image loaded
        IsDirectory  = 0x08,				// directory selected
        FileSelected = 0x10,				// 1 file selected
        ManySelected = 0x20,				// multiple selection
        RootSelected = 0x40				// root is selected
    };
    Q_DECLARE_FLAGS(StateFlags, StateFlag);

    /**
     * Default constructor
     */
    KookaView(KMainWindow *parent, const QByteArray &deviceToUse);

    /**
     * Destructor
     */
    virtual ~KookaView();

    /**
     * Print this view to any medium -- paper or not
     */
    void print( );

    void loadStartupImage();
    ScanGallery *gallery() const;
    ImageCanvas *imageViewer() const	{ return (mImageCanvas); }

    bool isScannerConnected() const;
    QString scannerName() const;
    void closeScanDevice();

    void connectViewerAction(KAction *action, bool sepBefore = false);
    void connectGalleryAction(KAction *action, bool sepBefore = false);
    void connectThumbnailAction(KAction *action);
    void connectPreviewAction(KAction *action);

    void saveProperties(KConfigGroup &grp);
    void saveWindowSettings(KConfigGroup &grp);
    void applyWindowSettings(const KConfigGroup &grp);

public slots:
    void slotStartOcr();
    void slotStartOcrSelection();
    void slotSetOcrSpellConfig(const QString &configFile);
    void slotOcrSpellCheck(bool interactive = true, bool background = false);
    void slotSaveOcrResult();

    void slotStartPreview();
    void slotNewPreview(const QImage *newimg, const ImgScanInfo *info);
    void slotStartFinalScan();

    void slotCreateNewImgFromSelection();
    void slotRotateImage(int angle);
    void slotMirrorImage(KookaView::MirrorType type);

    void slotScanParams();
    void slotApplySettings();

    /**
     * slot to select the scanner device. Does all the work with selection
     * of scanner, disconnection of the old device and connecting the new.
     */
    bool slotSelectDevice(const QByteArray &useDevice = "", bool alwaysAsk = true);
    void slotAddDevice();

    void slotScanStart();
    void slotScanFinished( KScanDevice::Status stat );
    void slotAcquireStart();

    void showOpenWithMenu(KActionMenu *menu);

protected slots:
    void slotStartPhotoCopy();
    void slotPhotoCopyPrint(const QImage *img, const ImgScanInfo *info);
    void slotPhotoCopyScan( KScanDevice::Status );

    void slotShowAImage(const KookaImage *img, bool isDir);
    void slotUnloadAImage(const KookaImage *img);

    /**
     * called from the scandevice if a new Image was successfully scanned.
     * Needs to convert the one-page-QImage to a KookaImage
     */
    void slotNewImageScanned(const QImage *img, const ImgScanInfo *info);

    void slotSelectionChanged(const QRect &newSelection);
    void slotGallerySelectionChanged();

    void slotOcrResultAvailable();

    void slotTabChanged(int index);
    void slotImageViewerAction(int act);

signals:
    /**
     * Use this signal to change the content of the statusbar
     */
    void signalChangeStatusbar(const QString &text);

    /**
     * Use this signal to clean up the statusbar
     */
    void signalCleanStatusbar();

    /**
     * Use this signal to change the content of the caption
     */
    void signalChangeCaption(const QString &text);

    void signalViewSelectionState(KookaView::StateFlags state);
    void signalScannerChanged(bool haveConnection);
    void signalRectangleChanged(bool haveSelection);
    void signalOcrResultAvailable(bool haveText);
    void signalOcrPrefs();


private:
    void startOCR(const KookaImage img);

    QByteArray userDeviceSelection(bool alwaysAsk);

    void updateCurrImage(const QImage &img);
    void saveGalleryState(int index = -1) const;
    void restoreGalleryState(int index = -1);

    void updateSelectionState();

private slots:
    void slotStartLoading(const KUrl &url);
    void slotOpenWith(int idx);

private:
    KMainWindow *mMainWindow;

    ImageCanvas *mImageCanvas;
    ThumbView *mThumbView;
    Previewer *mPreviewCanvas;
    KookaGallery *mGallery;
    KookaScanParams *mScanParams;

    KScanDevice *mScanDevice;

    QImage *mOcrResultImg;
    OcrEngine *mOcrEngine;
    OcrResEdit *mOcrResEdit;

    bool mIsPhotoCopyMode;
    KPrinter* mPhotoCopyPrinter;

    KookaView::TabPage mCurrentTab;

    QSplitter *mScanPage;
    QSplitter *mScanSubSplitter;
    QSplitter *mGalleryPage;
    QSplitter *mGallerySubSplitter;
    QSplitter *mOcrPage;
    QSplitter *mOcrSubSplitter;

    WidgetSite *mParamsSite;

    WidgetSite *mScanGallerySite;
    WidgetSite *mGalleryGallerySite;
    WidgetSite *mOcrGallerySite;

    WidgetSite *mGalleryImgviewSite;
    WidgetSite *mOcrImgviewSite;

    KService::List mOpenWithOffers;
    QSignalMapper *mOpenWithMapper;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KookaView::StateFlags);


#endif							// KOOKAVIEW_H
