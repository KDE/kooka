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

#include "kooka.h"

#include <qevent.h>
#include <qaction.h>
#include <qicon.h>
#include <qmenu.h>
#include <qlabel.h>
#include <qmimedata.h>

#include <klocalizedstring.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kstandardaction.h>
#include <kactioncollection.h>
#include <kxmlguiwindow.h>

#include "scanglobal.h"
#include "imagecanvas.h"
#include "previewer.h"

#include "scangallery.h"
#include "kookapref.h"
#include "kookasettings.h"
#include "kookaview.h"
#include "imagetransform.h"
#include "kooka_logging.h"


Kooka::Kooka(const QByteArray &deviceToUse)
    : KXmlGuiWindow(nullptr),
      m_prefDialogIndex(0)
{
    setObjectName("Kooka");

    // Set up the status bar
    StatusBarManager *sbm = new StatusBarManager(this);

    /* Start to create the main view framework */
    m_view = new KookaView(this, deviceToUse);
    setCentralWidget(m_view);

    setAcceptDrops(false); // Waba: Not (yet?) supported

    readProperties(KSharedConfig::openConfig()->group(autoSaveGroup()));

    // then, setup our actions
    setupActions();

    setupGUI(KXmlGuiWindow::Default, "kookaui.rc");
    setAutoSaveSettings();              // default group, do save

    // Allow the view to change the status bar and caption
    connect(m_view, &KookaView::changeStatus, sbm, &StatusBarManager::setStatus);
    connect(m_view, &KookaView::clearStatus, sbm, &StatusBarManager::clearStatus);
    connect(m_view, &KookaView::signalChangeCaption, this, QOverload<const QString &>::of(&KMainWindow::setCaption));
    connect(m_view, &KookaView::signalScannerChanged, this, &Kooka::slotUpdateScannerActions);
    connect(m_view, &KookaView::signalRectangleChanged, this, &Kooka::slotUpdateRectangleActions);
    connect(m_view, &KookaView::signalViewSelectionState, this, &Kooka::slotUpdateViewActions);
    connect(m_view, &KookaView::signalOcrResultAvailable, this, &Kooka::slotUpdateOcrResultActions);
    connect(m_view, &KookaView::signalOcrPrefs, this, &Kooka::optionsOcrPreferences);

    connect(m_view->imageViewer(), &ImageCanvas::imageReadOnly, this, &Kooka::slotUpdateReadOnlyActions);

    connect(m_view->previewer(), &Previewer::autoSelectStateChanged, this, &Kooka::slotUpdateAutoSelectActions);
    connect(m_view->previewer(), &Previewer::previewFileSizeChanged, sbm, &StatusBarManager::setFileSize);

    setCaption(i18n("KDE Scanning"));

    slotUpdateScannerActions(m_view->isScannerConnected());
    slotUpdateRectangleActions(false);
    slotUpdateViewActions(KookaView::GalleryShown | KookaView::IsDirectory | KookaView::RootSelected);
    slotUpdateOcrResultActions(false);
    slotUpdateReadOnlyActions(true);
}

Kooka::~Kooka()
{
    m_view->closeScanDevice();
    delete m_view;                  // ensure its config saved
#ifndef KDE4
    delete m_printer;
#endif
}

void Kooka::startup()
{
    if (m_view == nullptr) return;

    qCDebug(KOOKA_LOG);
    m_view->gallery()->openRoots();
    m_view->loadStartupImage();
}

// TODO: use KStandardShortcut wherever available
void Kooka::setupActions()
{
    printImageAction = KStandardAction::print(this, &Kooka::filePrint, actionCollection());

    KStandardAction::quit(this, &QWidget::close, actionCollection());
    KStandardAction::preferences(this, &Kooka::optionsPreferences, actionCollection());

    // Image Viewer

    scaleToWidthAction =  new QAction(QIcon::fromTheme("zoom-fit-width"), i18n("Scale to Width"), this);
    actionCollection()->addAction("scaleToWidth", scaleToWidthAction);
    actionCollection()->setDefaultShortcut(scaleToWidthAction, Qt::CTRL + Qt::Key_I);
    connect(scaleToWidthAction, &QAction::triggered, [=]() { m_view->imageViewerAction(ImageCanvas::UserActionFitWidth); });
    m_view->connectViewerAction(scaleToWidthAction);
    m_view->connectPreviewAction(scaleToWidthAction);

    scaleToHeightAction = new QAction(QIcon::fromTheme("zoom-fit-height"), i18n("Scale to Height"), this);
    actionCollection()->addAction("scaleToHeight", scaleToHeightAction);
    actionCollection()->setDefaultShortcut(scaleToHeightAction, Qt::CTRL + Qt::Key_H);
    connect(scaleToHeightAction, &QAction::triggered, [=]() { m_view->imageViewerAction(ImageCanvas::UserActionFitHeight); });
    m_view->connectViewerAction(scaleToHeightAction);
    m_view->connectPreviewAction(scaleToHeightAction);

    scaleToOriginalAction = new QAction(QIcon::fromTheme("zoom-original"), i18n("Original Size"), this);
    actionCollection()->addAction("scaleOriginal", scaleToOriginalAction);
    actionCollection()->setDefaultShortcut(scaleToOriginalAction, Qt::CTRL + Qt::Key_1);
    connect(scaleToOriginalAction, &QAction::triggered, [=]() { m_view->imageViewerAction(ImageCanvas::UserActionOrigSize); });
    m_view->connectViewerAction(scaleToOriginalAction);

    scaleToZoomAction = new QAction(QIcon::fromTheme("page-zoom"), i18n("Set Zoom..."), this);
    actionCollection()->addAction("showZoomDialog", scaleToZoomAction);
    // No shortcut.  There wasn't a standard shortcut for "Zoom" in KDE4,
    // and there is no KStandardShortcut::Zoom in KF5.
    connect(scaleToZoomAction, &QAction::triggered, [=]() { m_view->imageViewerAction(ImageCanvas::UserActionZoom); });
    m_view->connectViewerAction(scaleToZoomAction);

    keepZoomAction = new KToggleAction(QIcon::fromTheme("lockzoom"), i18n("Keep Zoom Setting"), this);
    actionCollection()->addAction("keepZoom", keepZoomAction);
    actionCollection()->setDefaultShortcut(keepZoomAction, Qt::CTRL + Qt::Key_Z);
    connect(keepZoomAction, &KToggleAction::toggled, m_view->imageViewer(), &ImageCanvas::setKeepZoom);
    m_view->connectViewerAction(keepZoomAction);

    // Thumb view and gallery actions

    newFromSelectionAction = new QAction(QIcon::fromTheme("transform-crop"), i18n("New Image From Selection"), this);
    actionCollection()->addAction("createFromSelection", newFromSelectionAction);
    actionCollection()->setDefaultShortcut(newFromSelectionAction, Qt::CTRL + Qt::Key_N);
    connect(newFromSelectionAction, &QAction::triggered, m_view, &KookaView::slotCreateNewImgFromSelection);

    mirrorVerticallyAction = new QAction(QIcon::fromTheme("object-flip-vertical"), i18n("Mirror Vertically"), this);
    mirrorVerticallyAction->setData(ImageTransform::MirrorVertical);
    actionCollection()->addAction("mirrorVertical", mirrorVerticallyAction);
    actionCollection()->setDefaultShortcut(mirrorVerticallyAction, Qt::CTRL + Qt::Key_V);
    connect(mirrorVerticallyAction, &QAction::triggered, m_view, &KookaView::slotTransformImage);
    m_view->connectViewerAction(mirrorVerticallyAction, true);

    mirrorHorizontallyAction = new QAction(QIcon::fromTheme("object-flip-horizontal"), i18n("Mirror Horizontally"), this);
    mirrorHorizontallyAction->setData(ImageTransform::MirrorHorizontal);
    actionCollection()->addAction("mirrorHorizontal", mirrorHorizontallyAction);
    actionCollection()->setDefaultShortcut(mirrorHorizontallyAction, Qt::CTRL + Qt::Key_M);
    connect(mirrorHorizontallyAction, &QAction::triggered, m_view, &KookaView::slotTransformImage);
    m_view->connectViewerAction(mirrorHorizontallyAction);

    // Standard KDE has icons for 'object-rotate-right' and 'object-rotate-left',
    // but not for rotate by 180 degrees.  The 3 used here are copies of the 22x22
    // icons from the old kdeclassic theme.
    rotateAcwAction = new QAction(QIcon::fromTheme("rotate-acw"), i18n("Rotate Counter-Clockwise"), this);
    rotateAcwAction->setData(ImageTransform::Rotate270);
    actionCollection()->addAction("rotateCounterClockwise", rotateAcwAction);
    actionCollection()->setDefaultShortcut(rotateAcwAction, Qt::CTRL + Qt::Key_7);
    connect(rotateAcwAction, &QAction::triggered, m_view, &KookaView::slotTransformImage);
    m_view->connectViewerAction(rotateAcwAction, true);

    rotateCwAction = new QAction(QIcon::fromTheme("rotate-cw"), i18n("Rotate Clockwise"), this);
    rotateCwAction->setData(ImageTransform::Rotate90);
    actionCollection()->addAction("rotateClockwise", rotateCwAction);
    actionCollection()->setDefaultShortcut(rotateCwAction, Qt::CTRL + Qt::Key_9);
    connect(rotateCwAction, &QAction::triggered, m_view, &KookaView::slotTransformImage);
    m_view->connectViewerAction(rotateCwAction);

    rotate180Action = new QAction(QIcon::fromTheme("rotate-180"), i18n("Rotate 180 Degrees"), this);
    rotate180Action->setData(ImageTransform::Rotate180);
    actionCollection()->addAction("upsitedown", rotate180Action);
    actionCollection()->setDefaultShortcut(rotate180Action, Qt::CTRL + Qt::Key_8);
    connect(rotate180Action, &QAction::triggered, m_view, &KookaView::slotTransformImage);
    m_view->connectViewerAction(rotate180Action);

    // Gallery actions

    createFolderAction = new QAction(QIcon::fromTheme("folder-new"), i18n("New Folder..."), this);
    actionCollection()->addAction("foldernew", createFolderAction);
    connect(createFolderAction, &QAction::triggered, m_view->gallery(), &ScanGallery::slotCreateFolder);
    m_view->connectGalleryAction(createFolderAction);

    openWithMenu = new KActionMenu(QIcon::fromTheme("document-open"), i18n("Open With"), this);
    actionCollection()->addAction("openWidth", openWithMenu);
    connect(openWithMenu->menu(), &QMenu::aboutToShow, this, &Kooka::slotOpenWithMenu);
    m_view->connectGalleryAction(openWithMenu, true);
    m_view->connectThumbnailAction(openWithMenu);

    saveImageAction = new QAction(QIcon::fromTheme("document-save"), i18n("Save Image..."), this);
    actionCollection()->addAction("saveImage", saveImageAction);
    actionCollection()->setDefaultShortcuts(saveImageAction, KStandardShortcut::save());
    connect(saveImageAction, &QAction::triggered, m_view->gallery(), &ScanGallery::slotExportFile);
    m_view->connectGalleryAction(saveImageAction);
    m_view->connectThumbnailAction(saveImageAction);

    importImageAction = new QAction(QIcon::fromTheme("document-import"), i18n("Import Image..."), this);
    actionCollection()->addAction("importImage", importImageAction);
    connect(importImageAction, &QAction::triggered, m_view->gallery(), &ScanGallery::slotImportFile);
    m_view->connectGalleryAction(importImageAction);

    deleteImageAction = new QAction(QIcon::fromTheme("edit-delete"), i18n("Delete Image"), this);
    actionCollection()->addAction("deleteImage", deleteImageAction);
    actionCollection()->setDefaultShortcut(deleteImageAction, Qt::SHIFT + Qt::Key_Delete);
    connect(deleteImageAction, &QAction::triggered, m_view->gallery(), &ScanGallery::slotDeleteItems);
    m_view->connectGalleryAction(deleteImageAction);
    m_view->connectThumbnailAction(deleteImageAction);

    renameImageAction = new QAction(QIcon::fromTheme("edit-rename"), i18n("Rename Image"), this);
    actionCollection()->addAction("renameImage", renameImageAction);
    actionCollection()->setDefaultShortcut(renameImageAction, Qt::Key_F2);
    connect(renameImageAction, &QAction::triggered, m_view->gallery(), &ScanGallery::slotRenameItems);
    m_view->connectGalleryAction(renameImageAction);

    unloadImageAction = new QAction(QIcon::fromTheme("document-close"), i18n("Unload Image"), this);
    actionCollection()->addAction("unloadImage", unloadImageAction);
    actionCollection()->setDefaultShortcut(unloadImageAction, Qt::CTRL + Qt::SHIFT + Qt::Key_U);
    connect(unloadImageAction, &QAction::triggered, m_view->gallery(), &ScanGallery::slotUnloadItems);
    m_view->connectGalleryAction(unloadImageAction);
    m_view->connectThumbnailAction(unloadImageAction);

    propsImageAction = new QAction(QIcon::fromTheme("document-properties"), i18n("Properties..."), this);
    actionCollection()->addAction("propsImage", propsImageAction);
    actionCollection()->setDefaultShortcut(propsImageAction, Qt::ALT + Qt::Key_Return);
    connect(propsImageAction, &QAction::triggered, m_view->gallery(), &ScanGallery::slotItemProperties);
    m_view->connectGalleryAction(propsImageAction, true);
    m_view->connectThumbnailAction(propsImageAction);

    // "Settings" menu

    selectDeviceAction = new QAction(QIcon::fromTheme("scanselect"), i18n("Select Scan Device..."), this);
    actionCollection()->addAction("selectsource", selectDeviceAction);
    connect(selectDeviceAction, &QAction::triggered, m_view, [this](){ m_view->slotSelectDevice(); });

    addDeviceAction = new QAction(QIcon::fromTheme("scanadd"), i18n("Add Scan Device..."), this);
    actionCollection()->addAction("addsource", addDeviceAction);
    connect(addDeviceAction, &QAction::triggered, m_view, &KookaView::slotAddDevice);

    // Scanning functions

    previewAction = new QAction(QIcon::fromTheme("preview"), i18n("Preview"), this);
    actionCollection()->addAction("startPreview", previewAction);
    actionCollection()->setDefaultShortcut(previewAction, Qt::Key_F3);
    connect(previewAction, &QAction::triggered, m_view, &KookaView::slotStartPreview);

    scanAction = new QAction(QIcon::fromTheme("scan"), i18n("Start Scan"), this);
    actionCollection()->addAction("startScan", scanAction);
    actionCollection()->setDefaultShortcut(scanAction, Qt::Key_F4);
    connect(scanAction, &QAction::triggered, m_view, &KookaView::slotStartFinalScan);

    paramsAction = new QAction(QIcon::fromTheme("bookmark-new"), i18n("Scan Presets..."), this);
    actionCollection()->addAction("scanparam", paramsAction);
    actionCollection()->setDefaultShortcut(paramsAction, Qt::CTRL + Qt::SHIFT + Qt::Key_S);
    connect(paramsAction, &QAction::triggered, m_view, &KookaView::slotScanParams);

    autoselAction = new KToggleAction(QIcon::fromTheme("autoselect"), i18n("Auto Select"), this);
    actionCollection()->addAction("autoselect", autoselAction);
    actionCollection()->setDefaultShortcut(autoselAction, Qt::CTRL + Qt::Key_A);
    connect(autoselAction, &QAction::toggled, m_view, &KookaView::slotAutoSelect);
    m_view->connectPreviewAction(autoselAction);

    // OCR functions

    ocrAction = new QAction(QIcon::fromTheme("ocr"), i18n("OCR Image..."), this);
    actionCollection()->addAction("ocrImage", ocrAction);
    connect(ocrAction, &QAction::triggered, m_view, &KookaView::slotStartOcr);

    ocrSelectAction = new QAction(QIcon::fromTheme("ocr-select"), i18n("OCR Selection..."), this);
    actionCollection()->addAction("ocrImageSelect", ocrSelectAction);
    connect(ocrSelectAction, &QAction::triggered, m_view, &KookaView::slotStartOcrSelection);

    m_saveOCRTextAction = new QAction(QIcon::fromTheme("document-save-as"), i18n("Save OCR Result Text..."), this);
    actionCollection()->addAction("saveOCRResult", m_saveOCRTextAction);
    actionCollection()->setDefaultShortcut(m_saveOCRTextAction, Qt::CTRL + Qt::Key_U);
    connect(m_saveOCRTextAction, &QAction::triggered, m_view, &KookaView::slotSaveOcrResult);

    ocrSpellAction = new QAction(QIcon::fromTheme("tools-check-spelling"), i18n("Spell Check OCR Result..."), this);
    actionCollection()->addAction("ocrSpellCheck", ocrSpellAction);
    connect(ocrSpellAction, &QAction::triggered, m_view, [this](){ m_view->slotOcrSpellCheck(); });
}

void Kooka::closeEvent(QCloseEvent *ev)
{
    KConfigGroup grp = KSharedConfig::openConfig()->group(autoSaveGroup());
    saveProperties(grp);
    if (autoSaveSettings())
    {
        saveAutoSaveSettings();
        m_view->saveWindowSettings(grp);
    }
}

void Kooka::saveProperties(KConfigGroup &grp)
{
    qCDebug(KOOKA_LOG) << "to group" << grp.name();

    // The group object points to the session managed
    // config file.  Anything you write here will be available
    // later when this app is restored.

    KookaSettings::setPreferencesTab(m_prefDialogIndex);
    //FIXME crash KookaSettings::setStartupSelectedImage(m_view->gallery()->currentImageFileName());
    KookaSettings::self()->save();
}

void Kooka::applyMainWindowSettings(const KConfigGroup &grp)
{
    qCDebug(KOOKA_LOG) << "from group" << grp.name();
    KXmlGuiWindow::applyMainWindowSettings(grp);
    m_view->applyWindowSettings(grp);
}

void Kooka::readProperties(const KConfigGroup &grp)
{
    qCDebug(KOOKA_LOG) << "from group" << grp.name();

    // The group object points to the session managed
    // config file.  This function is automatically called whenever
    // the app is being restored.  Read in here whatever you wrote
    // in 'saveProperties'.

    m_prefDialogIndex = KookaSettings::preferencesTab();
}

void Kooka::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasUrls()) ev->accept();	// accept URI drops only
}

void Kooka::filePrint()
{
    // this slot is called whenever the File->Print menu is selected,
    // the Print shortcut is pressed (usually CTRL+P) or the Print toolbar
    // button is clicked
    m_view->print();
}


void Kooka::optionsPreferences()
{
    KookaPref dlg;

    dlg.showPageIndex(m_prefDialogIndex);
    connect(&dlg, &KookaPref::dataSaved, m_view, &KookaView::slotApplySettings);
    if (dlg.exec()) m_prefDialogIndex = dlg.currentPageIndex();
}


void Kooka::optionsOcrPreferences()
{
    m_prefDialogIndex = 4;              // go to OCR page
    optionsPreferences();
}

void Kooka::slotUpdateScannerActions(bool haveConnection)
{
    qCDebug(KOOKA_LOG) << "haveConnection" << haveConnection;

    scanAction->setEnabled(haveConnection);
    previewAction->setEnabled(haveConnection);
    paramsAction->setEnabled(haveConnection);

    if (!ScanGlobal::self()->available()) {
        selectDeviceAction->setEnabled(false);
        addDeviceAction->setEnabled(false);
    }

    setCaption(m_view->scannerName());
}

void Kooka::slotUpdateRectangleActions(bool haveSelection)
{
    qCDebug(KOOKA_LOG) << "haveSelection" << haveSelection;

    ocrSelectAction->setEnabled(haveSelection);
    newFromSelectionAction->setEnabled(haveSelection);
}

//  This is the most complex action update, depending on the operation
//  mode (i.e. the tab selected), the gallery selection, and whether the
//  (two) image viewers have an image loaded.  The actions controlled
//  here and the conditions for enabling them are:
//
//  +========================+==================+======================+
//  | Action/Operation       | Scan mode        | Gallery/OCR mode     |
//  +========================+==================+======================+
//  | scale width/height     | preview valid    | image loaded         |
//  +------------------------+------------------+----------------------+
//  | (!GalleryShown && PreviewValid) || (GalleryShown && ImageValid)  |
//  +========================+==================+======================+
//  | scale original/zoom    | no               | image loaded         |
//  +------------------------+------------------+----------------------+
//  | GalleryShown && ImageValid                                       |
//  +========================+==================+======================+
//  | keep zoom              | no               | yes                  |
//  +------------------------+------------------+----------------------+
//  | GalleryShown                                                     |
//  +========================+==================+======================+
//  | mirror/rotate          | no               | 1 image              |
//  +------------------------+------------------+----------------------+
//  | GalleryShown && FileSelected && !IsSubImage && !HasSubImages     |
//  +========================+==================+======================+
//  | create folder          | directory        | directory            |
//  +------------------------+------------------+----------------------+
//  | IsDirectory                                                      |
//  +========================+==================+======================+
//  | import                 | no               | directory            |
//  +------------------------+------------------+----------------------+
//  | GalleryShown && IsDirectory                                      |
//  +========================+==================+======================+
//  | OCR/print              | 1                | 1                    |
//  +------------------------+------------------+----------------------+
//  | FileSelected && !HasSubImages                                    |
//  +========================+==================+======================+
//  | Save                   | 1                | 1                    |
//  +------------------------+------------------+----------------------+
//  | FileSelected && !IsSubImage                                      |
//  +========================+==================+======================+
//  | unload (dir)           | yes              | yes                  |
//  | unload (not dir)       | image loaded     | image loaded         |
//  +------------------------+------------------+----------------------+
//  | IsDirectory | HasSubImages |                                     |
//  |                    ((FileSelected | ManySelected) & ImageValid)  |
//  +========================+==================+======================+
//  | delete (dir)           | 1 not root       | 1 not root           |
//  | delete (not dir)       | >1               | >1                   |
//  +------------------------+------------------+----------------------+
//  | (IsDirectory & !RootSelected) | ((FileSelected | ManySelected)   |
//  |                                                   & !IsSubImage  |
//  +========================+==================+======================+
//  | rename (dir)           | not root         | not root             |
//  | rename (not dir)       | 1 image          | 1 image              |
//  +------------------------+------------------+----------------------+
//  | (IsDirectory & !RootSelected) | (FileSelected & !IsSubImage)     |
//  +========================+==================+======================+
//  | props (dir)            | yes              | yes                  |
//  | props (not dir)        | 1 image          | 1 image              |
//  +------------------------+------------------+----------------------+
//  | IsDirectory | FileSelected                                       |
//  +========================+==================+======================+
//  | Open with              | yes              | not subimage         |
//  +------------------------+------------------+----------------------+
//  | !IsSubImage                                                      |
//  +==================================================================+
//
//  It is assumed that only one directory may be selected at a time in
//  the gallery, but multiple files/images may be.  Currently, though,
//  the Kooka gallery allows single selection only.
//
//  The source of the 'state' is KookaView::updateSelectionState().

void Kooka::slotUpdateViewActions(KookaView::StateFlags state)
{
    qCDebug(KOOKA_LOG) << "state" << Qt::hex << state;

    bool e;

    e = (!(state & KookaView::GalleryShown) && (state & KookaView::PreviewValid)) ||
        ((state & KookaView::GalleryShown) && (state & KookaView::ImageValid));
    scaleToWidthAction->setEnabled(e);
    scaleToHeightAction->setEnabled(e);

    e = (state & KookaView::GalleryShown) && (state & KookaView::ImageValid);
    scaleToOriginalAction->setEnabled(e);
    scaleToZoomAction->setEnabled(e);

    e = (state & KookaView::GalleryShown);
    keepZoomAction->setEnabled(e);

    e = (state & KookaView::GalleryShown) && (state & KookaView::FileSelected) && !(state & (KookaView::IsSubImage|KookaView::HasSubImages));
    m_imageChangeAllowed = e;
    mirrorVerticallyAction->setEnabled(e);
    mirrorHorizontallyAction->setEnabled(e);
    rotateCwAction->setEnabled(e);
    rotateAcwAction->setEnabled(e);
    rotate180Action->setEnabled(e);

    e = (state & KookaView::IsDirectory);
    createFolderAction->setEnabled(e);

    e = (state & KookaView::GalleryShown) && (state & KookaView::IsDirectory);
    importImageAction->setEnabled(e);

    e = (state & KookaView::FileSelected) && !(state & KookaView::HasSubImages);
    ocrAction->setEnabled(e);
    printImageAction->setEnabled(e);

    // TODO: allow this for sub-images (page extraction)
    e = (state & KookaView::FileSelected) && !(state & KookaView::IsSubImage);
    saveImageAction->setEnabled(e);

    if (state & KookaView::IsDirectory) {
        unloadImageAction->setText(i18n("Unload Folder"));
        deleteImageAction->setText(i18n("Delete Folder"));
        renameImageAction->setText(i18n("Rename Folder"));

        unloadImageAction->setEnabled(true);

        e = !(state & KookaView::RootSelected);
        deleteImageAction->setEnabled(e);
        renameImageAction->setEnabled(e);

        propsImageAction->setEnabled(true);
    } else {
        unloadImageAction->setText(i18n("Unload Image"));
        deleteImageAction->setText(i18n("Delete Image"));
        renameImageAction->setText(i18n("Rename Image"));

        e = ((state & (KookaView::FileSelected | KookaView::ManySelected)) &&
            (state & KookaView::ImageValid)) || (state & KookaView::HasSubImages);
        unloadImageAction->setEnabled(e);

        e = (state & (KookaView::FileSelected | KookaView::ManySelected)) && !(state & KookaView::IsSubImage);
        deleteImageAction->setEnabled(e);

        e = (state & KookaView::FileSelected) && !(state & KookaView::IsSubImage);
        renameImageAction->setEnabled(e);
        propsImageAction->setEnabled(e);
    }

    // TODO: allow this for sub-images
    openWithMenu->setEnabled(!(state & KookaView::IsSubImage));

    if (!(state & (KookaView::IsDirectory | KookaView::FileSelected | KookaView::ManySelected))) {
        slotUpdateRectangleActions(false);
    }
}

void Kooka::slotUpdateOcrResultActions(bool haveText)
{
    qCDebug(KOOKA_LOG) << "haveText" << haveText;
    m_saveOCRTextAction->setEnabled(haveText);
    ocrSpellAction->setEnabled(haveText);
}

void Kooka::slotUpdateReadOnlyActions(bool ro)
{
    bool enable = (m_imageChangeAllowed && !ro);    // also check gallery state

    mirrorVerticallyAction->setEnabled(enable);
    mirrorHorizontallyAction->setEnabled(enable);
    rotateCwAction->setEnabled(enable);
    rotateAcwAction->setEnabled(enable);
    rotate180Action->setEnabled(enable);
}

void Kooka::slotUpdateAutoSelectActions(bool isAvailable, bool isOn)
{
    qCDebug(KOOKA_LOG) << "available" << isAvailable << "on" << isOn;

    autoselAction->setEnabled(isAvailable);
    autoselAction->setChecked(isOn);
}

void Kooka::slotOpenWithMenu()
{
    m_view->showOpenWithMenu(openWithMenu);
}
