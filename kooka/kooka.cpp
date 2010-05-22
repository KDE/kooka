/**************************************************************************
			kooka.cpp  -  Main program class
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

#include "kooka.h"
#include "kooka.moc"

#include <qevent.h>
#include <qsignalmapper.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#ifndef KDE4
#include <kprinter.h>
#endif
#include <kstatusbar.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <ktoggleaction.h>
#include <kstandardaction.h>
#include <kactioncollection.h>
#include <kmenu.h>
#include <kxmlguiwindow.h>

#include "libkscan/scanglobal.h"
#include "libkscan/imagecanvas.h"

#include "scangallery.h"
#include "kookapref.h"
#include "kookaview.h"

#include <kvbox.h>
#include <qlabel.h>




#define DOCK_SIZES "DockSizes"


Kooka::Kooka(const QByteArray &deviceToUse)
    : KXmlGuiWindow(NULL),
#ifndef KDE4
      m_printer(0),
#endif
      m_prefDialogIndex(0)
{
    setObjectName("Kooka");

    // Set the status bar up first, so that this item is leftmost
    KStatusBar *statBar = statusBar();
    statBar->insertItem(i18n("Ready"), KookaView::StatusTemp, 1);
    statBar->setItemAlignment(KookaView::StatusTemp, Qt::AlignLeft);
    statBar->show();

    /* Start to create the main view framework */
    m_view = new KookaView(this, deviceToUse);
    setCentralWidget(m_view);

    setAcceptDrops(false); // Waba: Not (yet?) supported

    readProperties(KGlobal::config()->group(autoSaveGroup()));

    // then, setup our actions
    setupActions();

    setupGUI(KXmlGuiWindow::Default, "kookaui.rc");
    setAutoSaveSettings();				// default group, do save

    // allow the view to change the statusbar and caption
    connect(m_view, SIGNAL(signalChangeStatusbar(const QString&)),
            SLOT(changeStatusbar(const QString&)));
    connect(m_view, SIGNAL(signalCleanStatusbar(void)),
            SLOT(cleanStatusbar()));
    connect(m_view, SIGNAL(signalChangeCaption(const QString&)),
            SLOT(changeCaption(const QString&)));
    connect(m_view, SIGNAL(signalScannerChanged(bool)),
            SLOT(slotUpdateScannerActions(bool)));
    connect(m_view, SIGNAL(signalRectangleChanged(bool)),
            SLOT(slotUpdateRectangleActions(bool)));
    connect(m_view, SIGNAL(signalViewSelectionState(KookaView::StateFlags)),
            SLOT(slotUpdateViewActions(KookaView::StateFlags)));
    connect(m_view, SIGNAL(signalOcrResultAvailable(bool)),
            SLOT(slotUpdateOcrResultActions(bool)));
    connect(m_view, SIGNAL(signalOcrPrefs()),
            SLOT(optionsOcrPreferences()));

    connect(m_view->imageViewer(), SIGNAL(imageReadOnly(bool)),
            SLOT(slotUpdateReadOnlyActions(bool)));

    changeCaption( i18n( "KDE Scanning" ));

    slotUpdateScannerActions(m_view->isScannerConnected());
    slotUpdateRectangleActions(false);
    slotUpdateViewActions(KookaView::GalleryShown|KookaView::IsDirectory|KookaView::RootSelected);
    slotUpdateOcrResultActions(false);
    slotUpdateReadOnlyActions(true);
}


Kooka::~Kooka()
{
    m_view->closeScanDevice();
    delete m_view;					// ensure its config saved
#ifndef KDE4
    delete m_printer;
#endif
}


void Kooka::startup()
{
    if (m_view==NULL) return;

    kDebug();
    m_view->gallery()->openRoots();
    m_view->loadStartupImage();
}


// TODO: use KStandardShortcut wherever available
void Kooka::setupActions()
{
    printImageAction = KStandardAction::print(this, SLOT(filePrint()), actionCollection());

    KStandardAction::quit(this, SLOT(close()), actionCollection());
    KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());

    // Image Viewer

    QSignalMapper *mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(int)), m_view, SLOT(slotImageViewerAction(int)));

    scaleToWidthAction =  new KAction(KIcon("zoom-fit-width"), i18n("Scale to Width"), this);
    scaleToWidthAction->setShortcut(Qt::CTRL+Qt::Key_I);
    connect(scaleToWidthAction, SIGNAL(triggered()), mapper, SLOT(map()));
    actionCollection()->addAction("scaleToWidth", scaleToWidthAction);
    m_view->connectViewerAction(scaleToWidthAction);
    m_view->connectPreviewAction(scaleToWidthAction);
    mapper->setMapping(scaleToWidthAction, ImageCanvas::UserActionFitWidth);

    scaleToHeightAction = new KAction(KIcon("zoom-fit-height"), i18n("Scale to Height"), this);
    scaleToHeightAction->setShortcut(Qt::CTRL+Qt::Key_H);
    connect(scaleToHeightAction, SIGNAL(triggered()), mapper, SLOT(map()));
    actionCollection()->addAction("scaleToHeight", scaleToHeightAction);
    m_view->connectViewerAction(scaleToHeightAction);
    m_view->connectPreviewAction(scaleToHeightAction);
    mapper->setMapping(scaleToHeightAction, ImageCanvas::UserActionFitHeight);

    scaleToOriginalAction = new KAction(KIcon("zoom-original"), i18n("Original Size"), this);
    scaleToOriginalAction->setShortcut(Qt::CTRL+Qt::Key_1);
    connect(scaleToOriginalAction, SIGNAL(triggered()), mapper, SLOT(map()));
    actionCollection()->addAction("scaleOriginal", scaleToOriginalAction);
    m_view->connectViewerAction(scaleToOriginalAction);
    mapper->setMapping(scaleToOriginalAction, ImageCanvas::UserActionOrigSize);

    scaleToZoomAction = new KAction(KIcon("page-zoom"), i18n("Set Zoom..."), this);
    scaleToZoomAction->setShortcut(KStandardShortcut::shortcut(KStandardShortcut::Zoom));
    connect(scaleToZoomAction, SIGNAL(triggered()), mapper, SLOT(map()));
    actionCollection()->addAction("showZoomDialog", scaleToZoomAction);
    m_view->connectViewerAction(scaleToZoomAction);
    mapper->setMapping(scaleToZoomAction, ImageCanvas::UserActionZoom);

    keepZoomAction = new KToggleAction(KIcon("lockzoom"), i18n("Keep Zoom Setting"), this);
    keepZoomAction->setShortcut(Qt::CTRL+Qt::Key_Z);
    connect(keepZoomAction, SIGNAL(toggled(bool)), m_view->imageViewer(), SLOT(setKeepZoom(bool)));
    actionCollection()->addAction("keepZoom", keepZoomAction);
    m_view->connectViewerAction(keepZoomAction);

    // Thumb view and gallery actions

    newFromSelectionAction = new KAction(KIcon("transform-crop"), i18n("New Image From Selection"), this);
    newFromSelectionAction->setShortcut(Qt::CTRL+Qt::Key_N);
    connect(newFromSelectionAction, SIGNAL(triggered()), m_view, SLOT(slotCreateNewImgFromSelection()));
    actionCollection()->addAction("createFromSelection", newFromSelectionAction);

    mirrorVerticallyAction = new KAction(KIcon("object-flip-vertical"), i18n("Mirror Vertically"), this);
    mirrorVerticallyAction->setShortcut(Qt::CTRL+Qt::Key_V);
    connect(mirrorVerticallyAction, SIGNAL(triggered()), SLOT(slotMirrorVertical()));
    actionCollection()->addAction("mirrorVertical", mirrorVerticallyAction);
    m_view->connectViewerAction(mirrorVerticallyAction, true);

    mirrorHorizontallyAction = new KAction(KIcon("object-flip-horizontal"), i18n("Mirror Horizontally"), this);
    mirrorHorizontallyAction->setShortcut(Qt::CTRL+Qt::Key_M);
    connect(mirrorHorizontallyAction, SIGNAL(triggered()), SLOT(slotMirrorHorizontal()));
    actionCollection()->addAction("mirrorHorizontal", mirrorHorizontallyAction);
    m_view->connectViewerAction(mirrorHorizontallyAction);

    // Standard KDE has icons for 'object-rotate-right' and 'object-rotate-left',
    // but not for rotate by 180 degrees.  The 3 used here are copies of the 22x22
    // icons from the old kdeclassic theme.
    rotateAcwAction = new KAction(KIcon("rotate-acw"), i18n("Rotate Counter-Clockwise"), this);
    rotateAcwAction->setShortcut(Qt::CTRL+Qt::Key_7);
    connect(rotateAcwAction, SIGNAL(triggered()), SLOT(slotRotateCounterClockWise()));
    actionCollection()->addAction("rotateCounterClockwise", rotateAcwAction);
    m_view->connectViewerAction(rotateAcwAction, true);

    rotateCwAction = new KAction(KIcon("rotate-cw"), i18n("Rotate Clockwise"), this);
    rotateCwAction->setShortcut(Qt::CTRL+Qt::Key_9);
    connect(rotateCwAction, SIGNAL(triggered()), SLOT(slotRotateClockWise()));
    actionCollection()->addAction("rotateClockwise", rotateCwAction);
    m_view->connectViewerAction(rotateCwAction);

    rotate180Action = new KAction(KIcon("rotate-180"), i18n("Rotate 180 Degrees"), this);
    rotate180Action->setShortcut(Qt::CTRL+Qt::Key_8);
    connect(rotate180Action, SIGNAL(triggered()), SLOT(slotRotate180()));
    actionCollection()->addAction("upsitedown", rotate180Action);
    m_view->connectViewerAction(rotate180Action);

    // Gallery actions

    createFolderAction = new KAction(KIcon("folder-new"), i18n("Create Folder..."), this);
    connect(createFolderAction, SIGNAL(triggered()), m_view->gallery(), SLOT(slotCreateFolder()));
    actionCollection()->addAction("foldernew", createFolderAction);
    m_view->connectGalleryAction(createFolderAction);

    openWithMenu = new KActionMenu(KIcon("document-open"), i18n("Open With"), this);
    connect(openWithMenu->menu(), SIGNAL(aboutToShow()), SLOT(slotOpenWithMenu()));
    actionCollection()->addAction("openWidth", openWithMenu);
    m_view->connectGalleryAction(openWithMenu, true);
    m_view->connectThumbnailAction(openWithMenu);

    saveImageAction = new KAction(KIcon("document-save"), i18n("Save Image..."), this);
    saveImageAction->setShortcut(KStandardShortcut::save());
    connect(saveImageAction, SIGNAL(triggered()), m_view->gallery(), SLOT(slotExportFile()));
    actionCollection()->addAction("saveImage", saveImageAction);
    m_view->connectGalleryAction(saveImageAction);
    m_view->connectThumbnailAction(saveImageAction);

    importImageAction = new KAction(KIcon("document-import"), i18n("Import Image..."), this);
    connect(importImageAction, SIGNAL(triggered()), m_view->gallery(), SLOT(slotImportFile()));
    actionCollection()->addAction("importImage", importImageAction);
    //m_view->connectGalleryAction( importImageAction );

    deleteImageAction = new KAction(KIcon("edit-delete"), i18n("Delete Image"), this);
    // TODO: can get standard accel?
    deleteImageAction->setShortcut(Qt::SHIFT+Qt::Key_Delete);
    connect(deleteImageAction, SIGNAL(triggered()), m_view->gallery(), SLOT(slotDeleteItems()));
    actionCollection()->addAction("deleteImage", deleteImageAction);
    m_view->connectGalleryAction(deleteImageAction);
    m_view->connectThumbnailAction(deleteImageAction);

    renameImageAction = new KAction(KIcon("edit-rename"), i18n("Rename Image"), this);
    renameImageAction->setShortcut(Qt::Key_F2);
    connect(renameImageAction, SIGNAL(triggered()), m_view->gallery(), SLOT(slotRenameItems()));
    actionCollection()->addAction("renameImage", renameImageAction);
    m_view->connectGalleryAction(renameImageAction);

    unloadImageAction = new KAction(KIcon("document-close"), i18n("Unload Image"), this);
    unloadImageAction->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_U);
    connect(unloadImageAction, SIGNAL(triggered()), m_view->gallery(), SLOT(slotUnloadItems()));
    actionCollection()->addAction("unloadImage", unloadImageAction);
    m_view->connectGalleryAction(unloadImageAction);
    m_view->connectThumbnailAction(unloadImageAction);

    propsImageAction = new KAction(KIcon("document-properties"), i18n("Properties..."), this);
    propsImageAction->setShortcut(Qt::ALT+Qt::Key_Return);
    connect(propsImageAction, SIGNAL(triggered()), m_view->gallery(), SLOT(slotItemProperties()));
    actionCollection()->addAction("propsImage", propsImageAction);
    m_view->connectGalleryAction(propsImageAction, true);
    m_view->connectThumbnailAction(propsImageAction);

    // "Settings" menu

    selectDeviceAction = new KAction(KIcon("scanselect"), i18n("Select Scan Device..."), this);
    connect(selectDeviceAction, SIGNAL(triggered()), m_view, SLOT(slotSelectDevice()));
    actionCollection()->addAction("selectsource", selectDeviceAction);

    addDeviceAction = new KAction(KIcon("scanadd"), i18n("Add Scan Device..."), this);
    connect(addDeviceAction, SIGNAL(triggered()), m_view, SLOT(slotAddDevice()));
    actionCollection()->addAction("addsource", addDeviceAction);

    // Scanning functions

    previewAction = new KAction(KIcon("preview"), i18n("Preview"), this);
    previewAction->setShortcut(Qt::Key_F3);
    connect(previewAction, SIGNAL(triggered()), m_view, SLOT(slotStartPreview()));
    actionCollection()->addAction("startPreview", previewAction);

    scanAction = new KAction(KIcon("scan"), i18n("Start Scan"), this);
    scanAction->setShortcut(Qt::Key_F4);
    connect(scanAction, SIGNAL(triggered()), m_view, SLOT(slotStartFinalScan()));
    actionCollection()->addAction("startScan", scanAction);

    photocopyAction = new KAction(KIcon("photocopy"), i18n("Photocopy..."), this);
    photocopyAction->setShortcut(Qt::CTRL+Qt::Key_F);
    connect(photocopyAction, SIGNAL(triggered()), m_view, SLOT(slotStartPhotoCopy()));
    actionCollection()->addAction("startPhotoCopy", photocopyAction);

    paramsAction = new KAction(KIcon("bookmark-new"), i18n("Scan Parameters..."), this);
    paramsAction->setShortcut(Qt::CTRL+Qt::SHIFT+Qt::Key_S);
    connect(paramsAction, SIGNAL(triggered()), m_view, SLOT(slotScanParams()));
    actionCollection()->addAction("scanparam", paramsAction);

    // OCR functions

    ocrAction = new KAction(KIcon("ocr"), i18n("OCR Image..."), this);
    connect(ocrAction, SIGNAL(triggered()), m_view, SLOT(slotStartOcr()));
    actionCollection()->addAction("ocrImage", ocrAction);

    ocrSelectAction = new KAction(KIcon("ocr-select"), i18n("OCR Selection..."), this);
    connect(ocrSelectAction, SIGNAL(triggered()), m_view, SLOT(slotStartOcrSelection()));
    actionCollection()->addAction("ocrImageSelect", ocrSelectAction);

    m_saveOCRTextAction = new KAction(KIcon("document-save-as"), i18n("Save OCR Result Text..."), this);
    m_saveOCRTextAction->setShortcut(Qt::CTRL+Qt::Key_U);
    connect(m_saveOCRTextAction, SIGNAL(triggered()), m_view, SLOT(slotSaveOcrResult()));
    actionCollection()->addAction("saveOCRResult", m_saveOCRTextAction);

    ocrSpellAction = new KAction(KIcon("tools-check-spelling"), i18n("Spell Check OCR Result..."), this);
    connect(ocrSpellAction, SIGNAL(triggered()), m_view, SLOT(slotOcrSpellCheck()));
    actionCollection()->addAction("ocrSpellCheck", ocrSpellAction);
}


void Kooka::closeEvent(QCloseEvent *ev)
{
    KConfigGroup grp = KGlobal::config()->group(autoSaveGroup());
    saveProperties(grp);

    if (autoSaveSettings())
    {
        saveAutoSaveSettings();
        m_view->saveWindowSettings(grp);
    }
}


void Kooka::saveProperties(KConfigGroup &grp)
{
    kDebug() << "to group" << grp.name();

    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored

    //if (!m_view->currentURL().isNull())
    //     config->writePathEntry("lastURL", m_view->currentURL());
    grp.writeEntry(PREFERENCE_DIA_TAB, m_prefDialogIndex);
    m_view->saveProperties(grp);
}


void Kooka::applyMainWindowSettings(const KConfigGroup &grp, bool forceGlobal)
{
    kDebug() << "from group" << grp.name();

    KXmlGuiWindow::applyMainWindowSettings(grp, forceGlobal);
    m_view->applyWindowSettings(grp);
}


void Kooka::readProperties(const KConfigGroup &grp)
{
    kDebug() << "to group" << grp.name();

    // the 'config' object points to the session managed
    // config file.  this function is automatically called whenever
    // the app is being restored.  read in here whatever you wrote
    // in 'saveProperties'
    m_prefDialogIndex = grp.readEntry(PREFERENCE_DIA_TAB, 0);
    // QString url = grp.readPathEntry("lastURL");
}


void Kooka::dragEnterEvent(QDragEnterEvent *ev)
{
    KUrl::List uriList = KUrl::List::fromMimeData(ev->mimeData());
    if (!uriList.isEmpty()) ev->accept();		// accept URI drops only
}


void Kooka::filePrint()
{
    // this slot is called whenever the File->Print menu is selected,
    // the Print shortcut is pressed (usually CTRL+P) or the Print toolbar
    // button is clicked
    m_view->print();

}


// KXmlGuiWindow does this automatically now
//
//void Kooka::optionsConfigureToolbars()
//{
//    saveMainWindowSettings(KGlobal::config()->group(autoSaveGroup()));
//    KEditToolBar dlg(factory());			// use the standard toolbar editor
//    connect(&dlg, SIGNAL(newToolBarConfig()), SLOT(newToolbarConfig()));
//    dlg.exec();
//}

//void Kooka::newToolbarConfig()
//{
//    // OK/Apply pressed in the toolbar editor
//    applyMainWindowSettings(KGlobal::config()->group(autoSaveGroup()));
//}


void Kooka::optionsPreferences()
{
    KookaPref dlg;

#ifndef KDE4
    dlg.showPageIndex(m_prefDialogIndex);
#endif
    connect(&dlg, SIGNAL(dataSaved()), m_view, SLOT(slotApplySettings()));
#ifndef KDE4
    if (dlg.exec()) m_prefDialogIndex = dlg.currentPageIndex();
#else
    dlg.exec();
#endif

}


void Kooka::optionsOcrPreferences()
{
    m_prefDialogIndex = 4;				// go to OCR page
    optionsPreferences();
}


void Kooka::changeStatusbar(const QString& text)
{
    // display the text on the statusbar
    statusBar()->changeItem(text, KookaView::StatusTemp);
}

void Kooka::changeCaption(const QString& text)
{
    // display the text on the caption
    setCaption(text);
}

void Kooka::slotMirrorVertical( void )
{
   m_view->slotMirrorImage( KookaView::MirrorVertical );
}

void Kooka::slotMirrorHorizontal( void )
{
    m_view->slotMirrorImage( KookaView::MirrorHorizontal );
}

void Kooka::slotRotateClockWise( void )
{
   m_view->slotRotateImage( 90 );
}

void Kooka::slotRotateCounterClockWise( void )
{
   m_view->slotRotateImage( -90 );
}

void Kooka::slotRotate180( void )
{
    m_view->slotMirrorImage( KookaView::MirrorBoth );
    //m_view->slRotateImage( 180 );
}

//void Kooka::slEnableWarnings( )
//{
//    KMessageBox::information(this,i18n("All messages and warnings will now be shown."));
//    KMessageBox::enableAllMessages();
//    FormatDialog::forgetRemembered();
//    kapp->config()->reparseConfiguration();
//}


void Kooka::slotUpdateScannerActions(bool haveConnection)
{
    kDebug() << "haveConnection" << haveConnection;

    scanAction->setEnabled(haveConnection);
    previewAction->setEnabled(haveConnection);
    photocopyAction->setEnabled(haveConnection);
    paramsAction->setEnabled(haveConnection);

    if (!ScanGlobal::self()->available())
    {
        selectDeviceAction->setEnabled(false);
        addDeviceAction->setEnabled(false);
    }

    setCaption(m_view->scannerName());
}


void Kooka::slotUpdateRectangleActions(bool haveSelection)
{
    kDebug() << "haveSelection" << haveSelection;

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
//  | GalleryShown && FileSelected                                     |
//  +========================+==================+======================+
//  | create folder          | directory        | directory            |
//  +------------------------+------------------+----------------------+
//  | IsDirectory                                                      |
//  +========================+==================+======================+
//  | import                 | no               | directory            |
//  +------------------------+------------------+----------------------+
//  | GalleryShown && IsDirectory                                      |
//  +========================+==================+======================+
//  | OCR/save/print         | 1                | 1                    |
//  +------------------------+------------------+----------------------+
//  | FileSelected                                                     |
//  +========================+==================+======================+
//  | unload (dir)           | yes              | yes                  |
//  | unload (not dir)       | image loaded     | image loaded         |
//  +------------------------+------------------+----------------------+
//  | IsDirectory | ((FileSelected | ManySelected) & ImageValid)       |
//  +========================+==================+======================+
//  | delete (dir)           | 1 not root       | 1 not root           |
//  | delete (not dir)       | >1               | >1                   |
//  +------------------------+------------------+----------------------+
//  | (IsDirectory & !RootSelected) | (FileSelected | ManySelected)    |
//  +========================+==================+======================+
//  | rename (dir)           | not root         | not root             |
//  | rename (not dir)       | 1 image          | 1 image              |
//  +------------------------+------------------+----------------------+
//  | (IsDirectory & !RootSelected) | FileSelected                     |
//  +========================+==================+======================+
//  | props (dir)            | yes              | yes                  |
//  | props (not dir)        | 1 image          | 1 image              |
//  +------------------------+------------------+----------------------+
//  | IsDirectory | FileSelected                                       |
//  +==================================================================+
//
//  It is assumed that only one directory may be selected at a time in
//  the gallery, but multiple files/images may be.  Currently, though,
//  the Kooka gallery allows single selection only.
//
//  The source of the 'state' is KookaView::updateSelectionState().

void Kooka::slotUpdateViewActions(KookaView::StateFlags state)
{
    kDebug() << "state" << hex << state;

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

    e = (state & KookaView::GalleryShown) && (state & KookaView::FileSelected);
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

    e = (state & KookaView::FileSelected);
    ocrAction->setEnabled(e);
    saveImageAction->setEnabled(e);
    printImageAction->setEnabled(e);

    if (state & KookaView::IsDirectory)
    {
        unloadImageAction->setText(i18n("Unload Folder"));
        deleteImageAction->setText(i18n("Delete Folder"));
        renameImageAction->setText(i18n("Rename Folder"));

        unloadImageAction->setEnabled(true);

        e = !(state & KookaView::RootSelected);
        deleteImageAction->setEnabled(e);
        renameImageAction->setEnabled(e);

        propsImageAction->setEnabled(true);
    }
    else
    {
        unloadImageAction->setText(i18n("Unload Image"));
        deleteImageAction->setText(i18n("Delete Image"));
        renameImageAction->setText(i18n("Rename Image"));

        e = (state & (KookaView::FileSelected|KookaView::ManySelected)) &&
            (state & KookaView::ImageValid);
        unloadImageAction->setEnabled(e);

        e = (state & (KookaView::FileSelected|KookaView::ManySelected));
        deleteImageAction->setEnabled(e);

        e = (state & KookaView::FileSelected);
        renameImageAction->setEnabled(e);
        propsImageAction->setEnabled(e);
    }

    if (!(state & (KookaView::IsDirectory|KookaView::FileSelected|KookaView::ManySelected)))
        slotUpdateRectangleActions(false);
}


void Kooka::slotUpdateOcrResultActions(bool haveText)
{
    kDebug() << "haveText" << haveText;
    m_saveOCRTextAction->setEnabled(haveText);
    ocrSpellAction->setEnabled(haveText);
}


void Kooka::slotUpdateReadOnlyActions(bool ro)
{
    bool enable = (m_imageChangeAllowed && !ro);	// also check gallery state

    mirrorVerticallyAction->setEnabled(enable);
    mirrorHorizontallyAction->setEnabled(enable);
    rotateCwAction->setEnabled(enable);
    rotateAcwAction->setEnabled(enable);
    rotate180Action->setEnabled(enable);
}


void Kooka::slotOpenWithMenu()
{
    m_view->showOpenWithMenu(openWithMenu);
}
