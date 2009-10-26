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

#include "libkscan/kscanoption.h"
// TODO: inline functions into .cpp, then these includes not needed
#include "libkscan/img_canvas.h"

class QPainter;
class QPixmap;
class QSplitter;

class KConfigGroup;
class KPrinter;
class KAction;
class KActionCollection;
class K3FileTreeViewItem;
class KMainWindow;
class KUrl;

class OcrEngine;
class ThumbView;
class KookaImage;
class KookaGallery;
class OcrResEdit;
class ScanPackager;
class ScanParams;
class ImgScanInfo;
class Previewer;
class KScanDevice;

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
    enum TabPage { TabScan = 0, TabGallery = 1, TabOcr = 2 };

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
    ScanPackager *gallery() const;
    ImageCanvas *getImageViewer() const	{ return (img_canvas); }

    bool scannerConnected() const { return (haveConnection); }
    QString scannerName() const;
    void closeScanDevice();

    bool galleryRootSelected() const;

    void connectViewerAction(KAction *action);
    void connectGalleryAction(KAction *action);
    void connectThumbnailAction(KAction *action);

    void saveProperties(KConfigGroup &grp);
    void saveWindowSettings(KConfigGroup &grp);
    void applyWindowSettings(const KConfigGroup &grp);


public slots:
    void slotShowPreview()  {  }
    void slotShowPackager() {  }
    void slotNewPreview(const QImage *newimg, const ImgScanInfo *info);

    void slotStartOcr();
    void slotStartOcrSelection();
    void slotOcrSpellCheck();
    void slotSaveOcrResult();

    void slotStartPreview();
    void slotStartFinalScan();

    void slotCreateNewImgFromSelection();

    void slotRotateImage(int angle);

    void slotMirrorImage(KookaView::MirrorType type);

// TODO: just make these a slot connection Kooka --> ImageCanvas
    void slotIVScaleToWidth()
        { if( img_canvas ) img_canvas->handlePopup(ImageCanvas::ID_FIT_WIDTH );}
    void slotIVScaleToHeight()
        { if( img_canvas ) img_canvas->handlePopup(ImageCanvas::ID_FIT_HEIGHT );}
    void slotIVScaleOriginal()
        { if( img_canvas ) img_canvas->handlePopup(ImageCanvas::ID_ORIG_SIZE ); }
    void slotIVShowZoomDialog( )
        { if( img_canvas ) img_canvas->handlePopup(ImageCanvas::ID_POP_ZOOM ); }

    void slotOpenCurrInGraphApp();


    void slotScanParams();

    void slotOCRResultImage( const QPixmap& );

     void slotApplySettings();

    /**
     * slot that show the image viewer
     */
    void slotStartLoading(const KUrl &url);
    /**
     * starts ocr on the image the parameter is pointing to
     **/
    void startOCR(const KookaImage *img);


    /**
     * slot to select the scanner device. Does all the work with selection
     * of scanner, disconnection of the old device and connecting the new.
     */
    bool slotSelectDevice(const QByteArray &useDevice = "", bool alwaysAsk = true);
    void slotAddDevice();

    void slotScanStart();
    void slotScanFinished( KScanStat stat );
    void slotAcquireStart();


protected slots:

    void slotStartPhotoCopy();
    void slotPhotoCopyPrint(const QImage *img, const ImgScanInfo *info);
    void slotPhotoCopyScan( KScanStat );

    void slotShowAImage(const KookaImage *img);
    void slotUnloadAImage(const KookaImage *img);

    /**
     * called from the scandevice if a new Image was successfully scanned.
     * Needs to convert the one-page-QImage to a KookaImage
     */
    void slotNewImageScanned(const QImage *img, const ImgScanInfo *info);

    /**
     * called if an viewer image was set to read only or back to read write state.
     */
    void slotViewerReadOnly(bool ro);

    void slotSelectionChanged(QRect newSelection);
    void slotGallerySelectionChanged();
    void slotLoadedImageChanged(const KookaImage *img, bool isDir);
    void slotOcrResultText(const QString &text);

    void slotTabChanged(int index);

signals:
    /**
     * Use this signal to change the content of the statusbar
     */
    void signalChangeStatusbar(const QString& text);

    /**
     * Use this signal to clean up the statusbar
     */
    void signalCleanStatusbar();

    /**
     * Use this signal to change the content of the caption
     */
    void signalChangeCaption(const QString &text);

    void signalScannerChanged(bool haveConnection);
    void signalRectangleChanged(bool haveSelection);
    void signalGallerySelectionChanged(bool isDir, int howmanySelected);
    void signalLoadedImageChanged(bool isLoaded, bool isDir);
    void signalOcrResultAvailable(bool haveText);
    void signalOcrPrefs();

private:
    QByteArray userDeviceSelection(bool alwaysAsk);

    void updateCurrImage( QImage& ) ;

    KMainWindow *m_mainWindow;

    ImageCanvas  *img_canvas;
    ThumbView    *m_thumbview;

    Previewer    *preview_canvas;
    KookaGallery *m_gallery;

    ScanParams   *mScanParams;
    bool haveConnection;

    KScanDevice  *sane;

    QByteArray     connectedDevice;

    QImage       *m_ocrResultImg;
    int          image_pool_id;
    int 	 preview_id;

    OcrEngine *ocrFabric;
    OcrResEdit  *m_ocrResEdit;

    bool        isPhotoCopyMode;
    KPrinter*   photoCopyPrinter;

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
};

#endif							// KOOKAVIEW_H
