/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
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

#ifndef KOOKAVIEW_H
#define KOOKAVIEW_H

#include <qtabwidget.h>

#include <kservice.h>

#include "statusbarmanager.h"

#include "kscandevice.h"

class QSplitter;
class QSignalMapper;
class QUrl;

class KPrinter;
class QAction;
class KMainWindow;
class KActionMenu;

class OcrEngine;
class ThumbView;
class KookaImage;
class KookaGallery;
class OcrResEdit;
class ScanGallery;
class KookaScanParams;
class ImageMetaInfo;
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
class KookaView : public QTabWidget
{
    Q_OBJECT

public:
    enum MirrorType { MirrorVertical, MirrorHorizontal, MirrorBoth };
    enum TabPage { TabScan = 0, TabGallery = 1, TabOcr = 2, TabNone = 0xFF };

    /**
     * To avoid a proliferation of boolean parameters, these flags are
     * used to indicate the state of the gallery and image viewers.
     */
    enum StateFlag
    {
        GalleryShown = 0x001,				// in Gallery/OCR mode
        PreviewValid = 0x002,				// scan preview valid
        ImageValid   = 0x004,				// viewer image loaded
        IsDirectory  = 0x008,				// directory selected
        FileSelected = 0x010,				// 1 file selected
        ManySelected = 0x020,				// multiple selection
        RootSelected = 0x040,				// root is selected
        IsSubImage   = 0x080,				// a subimage selected
        HasSubImages = 0x100				// contains subimages
    };
    Q_DECLARE_FLAGS(StateFlags, StateFlag)

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
    void print();

    void loadStartupImage();
    ScanGallery *gallery() const;
    ImageCanvas *imageViewer() const;
    Previewer *previewer() const;

    bool isScannerConnected() const;
    QString scannerName() const;
    void closeScanDevice();

    void connectViewerAction(QAction *action, bool sepBefore = false);
    void connectGalleryAction(QAction *action, bool sepBefore = false);
    void connectThumbnailAction(QAction *action);
    void connectPreviewAction(QAction *action);

    void saveWindowSettings(KConfigGroup &grp);
    void applyWindowSettings(const KConfigGroup &grp);

public slots:
    void slotStartOcr();
    void slotStartOcrSelection();
    void slotSetOcrSpellConfig(const QString &configFile);
    void slotOcrSpellCheck(bool interactive = true, bool background = false);
    void slotSaveOcrResult();

    void slotStartPreview();
    void slotNewPreview(const QImage *newimg, const ImageMetaInfo *info);
    void slotStartFinalScan();
    void slotAutoSelect(bool on);

    void slotCreateNewImgFromSelection();
    void slotTransformImage();

    void slotScanParams();
    void slotApplySettings();

    /**
     * slot to select the scanner device. Does all the work with selection
     * of scanner, disconnection of the old device and connecting the new.
     */
    bool slotSelectDevice(const QByteArray &useDevice = "", bool alwaysAsk = true);
    void slotAddDevice();

    void slotScanStart(const ImageMetaInfo *info);
    void slotScanFinished(KScanDevice::Status stat);
    void slotAcquireStart();

    void showOpenWithMenu(KActionMenu *menu);

protected slots:
    void slotStartPhotoCopy();
    void slotPhotoCopyPrint(const QImage *img, const ImageMetaInfo *info);
    void slotPhotoCopyScan(KScanDevice::Status);

    void slotShowAImage(const KookaImage *img, bool isDir);
    void slotUnloadAImage(const KookaImage *img);

    /**
     * called from the scandevice if a new Image was successfully scanned.
     * Needs to convert the one-page-QImage to a KookaImage
     */
    void slotNewImageScanned(const QImage *img, const ImageMetaInfo *info);

    void slotSelectionChanged(const QRect &newSelection);
    void slotGallerySelectionChanged();

    void slotOcrResultAvailable();

    void slotTabChanged(int index);
    void slotImageViewerAction(int act);

signals:
    /**
     * Change the content of the statusbar
     */
    void changeStatus(const QString &text, StatusBarManager::Item item = StatusBarManager::Message);

    /**
     * Clean up the statusbar
     */
    void clearStatus(StatusBarManager::Item item = StatusBarManager::Message);

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

    void saveGalleryState(int index = -1) const;
    void restoreGalleryState(int index = -1);

    void updateSelectionState();

private slots:
    void slotStartLoading(const QUrl &url);
    void slotOpenWith(int idx);
    void slotPreviewDimsChanged(const QString &dims);

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
    KPrinter *mPhotoCopyPrinter;

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
