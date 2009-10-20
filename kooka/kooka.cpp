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

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#ifndef KDE4
#include <kprinter.h>
#endif
#include <kstatusbar.h>
#include <kurl.h>
//#include <kedittoolbar.h>
#include <kmessagebox.h>
//#include <kstandardaccel.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <ktoggleaction.h>
#include <kstandardaction.h>
#include <kactioncollection.h>
#include <kmenu.h>
#include <kxmlguiwindow.h>

#include "scanpackager.h"
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

    setAutoSaveSettings();				// default group, do save
//    readDockConfig(KGlobal::config().data(), DOCK_SIZES);
    readProperties(KGlobal::config()->group(autoSaveGroup()));

    // then, setup our actions
    setupActions();

    setupGUI(KXmlGuiWindow::Default, "kookaui.rc");

    // allow the view to change the statusbar and caption
    connect(m_view, SIGNAL(signalChangeStatusbar(const QString&)),
            this,   SLOT(changeStatusbar(const QString&)));
    connect(m_view, SIGNAL(signalCleanStatusbar(void)),
            this, SLOT(cleanStatusbar()));
    connect(m_view, SIGNAL(signalChangeCaption(const QString&)),
            this,   SLOT(changeCaption(const QString&)));
    connect(m_view, SIGNAL(signalScannerChanged(bool)),
            this,   SLOT(slotUpdateScannerActions(bool)));
    connect(m_view,SIGNAL(signalRectangleChanged(bool)),
            this,   SLOT(slotUpdateRectangleActions(bool)));
    connect(m_view,SIGNAL(signalGallerySelectionChanged(bool,int)),
            this,   SLOT(slotUpdateGalleryActions(bool,int)));
    connect(m_view,SIGNAL(signalLoadedImageChanged(bool,bool)),
            this,   SLOT(slotUpdateLoadedActions(bool,bool)));
    connect(m_view,SIGNAL(signalOcrResultAvailable(bool)),
            this,   SLOT(slotUpdateOcrResultActions(bool)));
    connect(m_view,SIGNAL(signalOcrPrefs()),
            this,   SLOT(optionsOcrPreferences()));

    changeCaption( i18n( "KDE Scanning" ));

    slotUpdateScannerActions(m_view->scannerConnected());
    slotUpdateRectangleActions(false);
    slotUpdateGalleryActions(true,0);
    slotUpdateOcrResultActions(false);
}



Kooka::~Kooka()
{
    m_view->closeScanDevice();
//    writeDockConfig(KGlobal::config().data(), DOCK_SIZES);
    delete m_view;					// ensure its config saved
#ifndef KDE4
    delete m_printer;
#endif
}


void Kooka::startup()
{
    kDebug();
    if (m_view!=NULL) m_view->loadStartupImage();
}


// TODO: use KStandardShortcut wherever available
void Kooka::setupActions()
{
    printImageAction = KStandardAction::print(this, SLOT(filePrint()), actionCollection());

    KStandardAction::quit(this, SLOT(close()), actionCollection());
    //KStandardAction::keyBindings(guiFactory(),SLOT(configureShortcuts()),actionCollection());
    //KStandardAction::configureToolbars(this,SLOT(optionsConfigureToolbars()),actionCollection());
    KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());

//    m_view->createDockMenu(actionCollection(), this, "settings_show_docks");

    // Image Viewer

    scaleToWidthAction =  new KAction(KIcon("zoom-fit-width"), i18n("Scale to Width"), this);
    scaleToWidthAction->setShortcut(Qt::CTRL+Qt::Key_I);
    connect(scaleToWidthAction, SIGNAL(triggered()), m_view, SLOT(slotIVScaleToWidth()));
    actionCollection()->addAction("scaleToWidth", scaleToWidthAction);
    m_view->connectViewerAction(scaleToWidthAction);

    scaleToHeightAction = new KAction(KIcon("zoom-fit-height"), i18n("Scale to Height"), this);
    scaleToHeightAction->setShortcut(Qt::CTRL+Qt::Key_H);
    connect(scaleToHeightAction, SIGNAL(triggered()), m_view, SLOT(slotIVScaleToHeight()));
    actionCollection()->addAction("scaleToheight", scaleToHeightAction);
    m_view->connectViewerAction(scaleToHeightAction);

    scaleToOriginalAction = new KAction(KIcon("zoom-original"), i18n("Original Size"), this);
    scaleToOriginalAction->setShortcut(Qt::CTRL+Qt::Key_1);
    connect(scaleToOriginalAction, SIGNAL(triggered()), m_view, SLOT(slotIVScaleOriginal()));
    actionCollection()->addAction("scaleOriginal", scaleToOriginalAction);
    m_view->connectViewerAction(scaleToOriginalAction);

    KToggleAction *tact = new KToggleAction(KIcon("lockzoom"), i18n("Keep Zoom Setting"), this);
    tact->setShortcut(Qt::CTRL+Qt::Key_Z);
//    connect(tact, SIGNAL(toggled(bool)), m_view->getImageViewer(), SLOT(setKeepZoom(bool)));
    actionCollection()->addAction("keepZoom", tact);
    m_view->connectViewerAction(tact);

    // Thumb view and gallery actions

    KAction *act = new KAction(KIcon("page-zoom"), i18n("Set Zoom..."), this);
    connect(act, SIGNAL(triggered()), m_view, SLOT(slotIVShowZoomDialog()));
    actionCollection()->addAction("showZoomDialog", act);
    m_view->connectViewerAction(act);

    newFromSelectionAction = new KAction(KIcon("crop"), i18n("New Image From Selection"), this);
    newFromSelectionAction->setShortcut(Qt::CTRL+Qt::Key_N);
    connect(newFromSelectionAction, SIGNAL(triggered()), m_view, SLOT(slotCreateNewImgFromSelection()));
    actionCollection()->addAction("createFromSelection", newFromSelectionAction);

    mirrorVerticallyAction = new KAction(KIcon("object-flip-vertical"), i18n("Mirror Vertically"), this);
    mirrorVerticallyAction->setShortcut(Qt::CTRL+Qt::Key_V);
    connect(mirrorVerticallyAction, SIGNAL(triggered()), SLOT(slotMirrorVertical()));
    actionCollection()->addAction("mirrorVertical", mirrorVerticallyAction);

    mirrorHorizontallyAction = new KAction(KIcon("object-flip-horizontal"), i18n("Mirror Horizontally"), this);
    mirrorHorizontallyAction->setShortcut(Qt::CTRL+Qt::Key_M);
    connect(mirrorHorizontallyAction, SIGNAL(triggered()), SLOT(slotMirrorHorizontal()));
    actionCollection()->addAction("mirrorHorizontal", mirrorHorizontallyAction);

    // Standard KDE has icons for 'object-rotate-right' and 'object-rotate-left',
    // but not for rotate by 180 degrees.  The 3 used here are copies of the 22x22
    // icons from the old kdeclassic theme.
    rotateCwAction = new KAction(KIcon("rotate-cw"), i18n("Rotate Clockwise"), this);
    rotateCwAction->setShortcut(Qt::CTRL+Qt::Key_9);
    connect(rotateCwAction, SIGNAL(triggered()), SLOT(slotRotateClockWise()));
    actionCollection()->addAction("rotateClockwise", rotateCwAction);
    m_view->connectViewerAction(rotateCwAction);

    rotateAcwAction = new KAction(KIcon("rotate-acw"), i18n("Rotate Counter-Clockwise"), this);
    rotateAcwAction->setShortcut(Qt::CTRL+Qt::Key_7);
    connect(rotateAcwAction, SIGNAL(triggered()), SLOT(slotRotateCounterClockWise()));
    actionCollection()->addAction("rotateCounterClockwise", rotateAcwAction);
    m_view->connectViewerAction(rotateAcwAction);

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
    m_view->connectGalleryAction(openWithMenu);
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
    m_view->connectGalleryAction(propsImageAction);
    m_view->connectThumbnailAction(propsImageAction);

    // "Settings" menu

    act = new KAction(KIcon("scanner"), i18n("Select Scan Device..."), this);
    connect(act, SIGNAL(triggered()), m_view, SLOT(slotSelectDevice()));
    actionCollection()->addAction("selectsource", act);

    act = new KAction(KIcon("list-add"), i18n("Add Scan Device..."), this);
    connect(act, SIGNAL(triggered()), m_view, SLOT(slotAddDevice()));
    actionCollection()->addAction("addsource", act);

    // Scanning functions

    previewAction = new KAction(KIcon("preview"), i18n("Preview"), this);
    previewAction->setShortcut(Qt::Key_F3);
    connect(previewAction, SIGNAL(triggered()), m_view, SLOT(slotStartPreview()));
    actionCollection()->addAction("startPreview", previewAction);

    scanAction = new KAction(KIcon("scanner"), i18n("Start Scan"), this);
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
    KXmlGuiWindow::closeEvent(ev);
}



void Kooka::saveProperties(KConfigGroup &grp)
{
    kDebug();

    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored

    //if (!m_view->currentURL().isNull())
    //     config->writePathEntry("lastURL", m_view->currentURL());
    grp.writeEntry(PREFERENCE_DIA_TAB, m_prefDialogIndex);
    m_view->saveProperties(grp);
}


void Kooka::readProperties(const KConfigGroup &grp)
{
    kDebug();

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

    setCaption(m_view->scannerName());
}


void Kooka::slotUpdateRectangleActions(bool haveSelection)
{
    kDebug() << "haveSelection" << haveSelection;

    ocrSelectAction->setEnabled(haveSelection);
    newFromSelectionAction->setEnabled(haveSelection);
}


void Kooka::slotUpdateGalleryActions(bool isDir,int howmanySelected)
{
    kDebug() << "isdir" << isDir << "howmany" << howmanySelected;

    const bool singleImage = howmanySelected==1 && !isDir;

    ocrAction->setEnabled(singleImage);

    scaleToWidthAction->setEnabled(singleImage);
    scaleToHeightAction->setEnabled(singleImage);
    scaleToOriginalAction->setEnabled(singleImage);
    mirrorVerticallyAction->setEnabled(singleImage);
    mirrorHorizontallyAction->setEnabled(singleImage);
    rotateCwAction->setEnabled(singleImage);
    rotateAcwAction->setEnabled(singleImage);
    rotate180Action->setEnabled(singleImage);

    if (howmanySelected==0) slotUpdateRectangleActions(false);

    createFolderAction->setEnabled(isDir);
    importImageAction->setEnabled(isDir);

    saveImageAction->setEnabled(singleImage);
    printImageAction->setEnabled(singleImage);

    if (isDir)
    {
        unloadImageAction->setText(i18n("Unload Folder"));
        unloadImageAction->setEnabled(true);

        deleteImageAction->setText(i18n("Delete Folder"));
        renameImageAction->setText(i18n("Rename Folder"));
        bool rs = m_view->galleryRootSelected();
        deleteImageAction->setEnabled(!rs);
        renameImageAction->setEnabled(!rs);
        propsImageAction->setEnabled(!rs);
    }
    else
    {
        unloadImageAction->setText(i18n("Unload Image"));
        unloadImageAction->setEnabled(singleImage &&
                                      m_view->gallery()->getCurrImage()!=NULL);

        deleteImageAction->setText(i18n("Delete Image"));
        renameImageAction->setText(i18n("Rename Image"));
        deleteImageAction->setEnabled(singleImage);
        renameImageAction->setEnabled(singleImage);
        propsImageAction->setEnabled(singleImage);
    }
}


void Kooka::slotUpdateLoadedActions(bool isLoaded,bool isDir)
{
    kDebug() << "loaded" << isLoaded << "isDir" << isDir;
    unloadImageAction->setEnabled(isLoaded || isDir);
}


void Kooka::slotUpdateOcrResultActions(bool haveText)
{
    kDebug() << "haveText" << haveText;
    m_saveOCRTextAction->setEnabled(haveText);
    ocrSpellAction->setEnabled(haveText);
}


void Kooka::slotOpenWithMenu()
{
    m_view->gallery()->showOpenWithMenu(openWithMenu);
}
