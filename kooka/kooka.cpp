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

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kprinter.h>
#include <kstatusbar.h>
#include <kurl.h>
#include <kedittoolbar.h>
#include <kmessagebox.h>
#include <kstdaccel.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kurldrag.h>
#include <kpopupmenu.h>

#include "kookapref.h"
#include "kookaview.h"

#include "kooka.h"
#include "kooka.moc"


#define DOCK_SIZES "DockSizes"


Kooka::Kooka( const QCString& deviceToUse)
   : KParts::DockMainWindow( 0, "Kooka" ),
      m_printer(0),
      m_prefDialogIndex(0)
{
    /* Start to create the main view framework */
    m_view = new KookaView( this, deviceToUse);

    /* Call createGUI on the ocr-result view */
    setXMLFile( "kookaui.rc", true );

    setAcceptDrops(false); // Waba: Not (yet?) supported
    KConfig *konf = KGlobal::config ();
    readDockConfig ( konf, DOCK_SIZES );

    // then, setup our actions
    setupActions();

    createGUI(0L); // m_view->ocrResultPart());
    // and a status bar
    statusBar()->insertItem(i18n("Ready"),KookaView::StatusTemp,1);
    statusBar()->setItemAlignment(KookaView::StatusTemp,Qt::AlignLeft);
    statusBar()->show();

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

    setAutoSaveSettings(  QString::fromLatin1("General Options"),
                          true );

    slotUpdateScannerActions(m_view->scannerConnected());
    slotUpdateRectangleActions(false);
    slotUpdateGalleryActions(true,0);
    slotUpdateOcrResultActions(false);
}


void Kooka::createMyGUI( KParts::Part *part )
{
    kdDebug(28000) << "Part changed, Creating gui" << endl;
    createGUI(part);
}


Kooka::~Kooka()
{
    KConfig *konf = KGlobal::config();
    m_view->slCloseScanDevice();
    writeDockConfig (konf,DOCK_SIZES);
    delete m_view;					// ensure its config saved
    delete m_printer;
}


void Kooka::startup()
{
    kdDebug(28000) << k_funcinfo << endl;
    if (m_view!=NULL) m_view->loadStartupImage();
}


void Kooka::setupActions()
{
    printImageAction = KStdAction::print(this, SLOT(filePrint()), actionCollection());

    KStdAction::quit(this,SLOT(close()),actionCollection());
    KStdAction::keyBindings(guiFactory(),SLOT(configureShortcuts()),actionCollection());
    KStdAction::configureToolbars(this,SLOT(optionsConfigureToolbars()),actionCollection());
    KStdAction::preferences(this,SLOT(optionsPreferences()),actionCollection());

    m_view->createDockMenu(actionCollection(), this, "settings_show_docks" );

    /* Image Viewer action Toolbar - Scaling etc. */

    scaleToWidthAction =  new KAction(i18n("Scale to Width"), "scaletowidth", CTRL+Key_I,
				      m_view, SLOT( slIVScaleToWidth()),
				      actionCollection(), "scaleToWidth" );
    m_view->connectViewerAction( scaleToWidthAction );

    scaleToHeightAction = new KAction(i18n("Scale to Height"), "scaletoheight", CTRL+Key_H,
				      m_view, SLOT( slIVScaleToHeight()),
				      actionCollection(), "scaleToHeight" );
    m_view->connectViewerAction( scaleToHeightAction );

    scaleToOriginalAction = new KAction(i18n("Original Size"), "scaleorig", CTRL+Key_1,
					m_view, SLOT( slIVScaleOriginal()),
					actionCollection(), "scaleOriginal" );
    m_view->connectViewerAction( scaleToOriginalAction );

#ifdef QICONSET_HONOUR_ON_OFF
    /* The Toggleaction does not seem to handle the on/off icon from QIconSet */
    QIconSet lockSet;
    lockSet.setPixmap(BarIcon("lock")  , QIconSet::Automatic, QIconSet::Normal, QIconSet::On );
    lockSet.setPixmap(BarIcon("unlock"), QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
    KAction *act = new KToggleAction ( i18n("Keep Zoom Setting"), lockSet, CTRL+Key_Z,
				       actionCollection(), "keepZoom" );
#else
    KAction *act = new KToggleAction( i18n("Keep Zoom Setting"), "lockzoom", CTRL+Key_Z,
				      actionCollection(), "keepZoom" );
#endif

    connect( act, SIGNAL( toggled( bool ) ), m_view->getImageViewer(),
             SLOT(setKeepZoom(bool)));

    m_view->connectViewerAction( act );

    /* thumbview and gallery actions */
    act = new KAction(i18n("Set Zoom..."), "viewmag", 0,
		       m_view, SLOT( slIVShowZoomDialog()),
		       actionCollection(), "showZoomDialog" );
    m_view->connectViewerAction( act );

    newFromSelectionAction = new KAction(i18n("New Image From Selection"), "crop", CTRL+Key_N,
					 m_view, SLOT( slCreateNewImgFromSelection() ),
					 actionCollection(), "createFromSelection" );

    mirrorVerticallyAction = new KAction(i18n("Mirror Vertically"), "mirror-vert", CTRL+Key_V,
					 this, SLOT( slMirrorVertical() ),
					 actionCollection(), "mirrorVertical" );

    mirrorHorizontallyAction = new KAction(i18n("Mirror Horizontally"), "mirror-horiz", CTRL+Key_M,
					   this, SLOT( slMirrorHorizontal() ),
					   actionCollection(), "mirrorHorizontal" );

    rotateCwAction = new KAction(i18n("Rotate Clockwise"), "rotate_cw", CTRL+Key_9,
				 this, SLOT( slRotateClockWise() ),
				 actionCollection(), "rotateClockwise" );
    m_view->connectViewerAction( rotateCwAction );

    rotateAcwAction = new KAction(i18n("Rotate Counter-Clockwise"), "rotate_ccw", CTRL+Key_7,
				  this, SLOT( slRotateCounterClockWise() ),
				  actionCollection(), "rotateCounterClockwise" );
    m_view->connectViewerAction( rotateAcwAction );

    rotate180Action = new KAction(i18n("Rotate 180 Degrees"), "rotate", CTRL+Key_8,
				  this, SLOT( slRotate180() ),
				  actionCollection(), "upsitedown" );
    m_view->connectViewerAction( rotate180Action );

    /* Gallery actions */
    createFolderAction = new KAction(i18n("Create Folder..."), "folder_new", 0,
				     m_view->gallery(), SLOT( slotCreateFolder() ),
				     actionCollection(), "foldernew" );
    m_view->connectGalleryAction( createFolderAction );

    //openWithAction = new KAction(i18n("Open In Graphical Application..."), "fileopen", KStdAccel::open(),
    //				 m_view, SLOT( slOpenCurrInGraphApp() ),
    //				 actionCollection(), "openInGraphApp" );
    openWithMenu = new KActionMenu(i18n("Open With"),"fileopen",
                                   actionCollection(),"openWith");
    connect(openWithMenu->popupMenu(),SIGNAL(aboutToShow()),SLOT(slotOpenWithMenu()));
    m_view->connectGalleryAction( openWithMenu );

    saveImageAction = new KAction(i18n("Save Image..."), "filesave", KStdAccel::save(),
				  m_view->gallery(), SLOT( slotExportFile() ),
				  actionCollection(), "saveImage" );
    m_view->connectGalleryAction( saveImageAction );

    importImageAction = new KAction(i18n("Import Image..."), "fileimport", 0,
				    m_view->gallery(), SLOT( slotImportFile() ),
				    actionCollection(), "importImage" );
    //m_view->connectGalleryAction( importImageAction );

    deleteImageAction = new KAction(i18n("Delete Image"), "editdelete", SHIFT+Key_Delete,
				    m_view->gallery(), SLOT( slotDeleteItems() ),
				    actionCollection(), "deleteImage" );
    m_view->connectGalleryAction( deleteImageAction );

    renameImageAction = new KAction(i18n("Rename Image"), "edittool", Key_F2,
				    m_view->gallery(), SLOT( slotRenameItems() ),
				    actionCollection(), "renameImage" );
    m_view->connectGalleryAction( renameImageAction );

    unloadImageAction = new KAction(i18n("Unload Image"), "fileclose", CTRL+SHIFT+Key_U,
				    m_view->gallery(), SLOT( slotUnloadItems() ),
				    actionCollection(), "unloadImage" );
    m_view->connectGalleryAction( unloadImageAction );

    propsImageAction = new KAction(i18n("Properties..."), 0,
				    m_view->gallery(), SLOT( slotItemProperties() ),
				    actionCollection(), "propsImage" );
    m_view->connectGalleryAction( propsImageAction );

    // "Settings" menu

    (void) new KAction(i18n("Select Scan Device..."), "scanner", 0,
		       m_view, SLOT( slSelectDevice()),
		       actionCollection(), "selectsource" );

    (void) new KAction(i18n("Add Scan Device..."), "add", 0,
		       m_view, SLOT( slAddDevice()),
		       actionCollection(), "addsource" );

//    (void) new KAction( i18n("Enable All Warnings && Messages"), 0,
//			this,  SLOT(slEnableWarnings()),
//			actionCollection(), "enable_msgs");


    // Scanning functions

    scanAction = new KAction(i18n("Preview"), "preview", Key_F3,
		       m_view, SLOT( slStartPreview()),
		       actionCollection(), "startPreview" );

    previewAction = new KAction(i18n("Start Scan"), "scanner", Key_F4,
		       m_view, SLOT( slStartFinalScan()),
		       actionCollection(), "startScan" );

    paramsAction = new KAction(i18n("Scan Parameters..."), "bookmark_add", CTRL+SHIFT+Key_S,
                               m_view, SLOT(slScanParams()),
                               actionCollection(), "scanparam" );

    // OCR functions

    ocrAction = new KAction(i18n("OCR Image..."), "ocr", 0,
			    m_view, SLOT(doOCR()),
			    actionCollection(), "ocrImage" );

    ocrSelectAction = new KAction(i18n("OCR Selection..."), "ocr-select", 0,
				  m_view, SLOT(doOCRonSelection()),
				  actionCollection(), "ocrImageSelect" );

    m_saveOCRTextAction = new KAction( i18n("Save OCR Result Text..."), "filesaveas", CTRL+Key_U,
                                       m_view, SLOT(slSaveOCRResult()),
                                       actionCollection(), "saveOCRResult");

    ocrSpellAction = new KAction(i18n("Spell Check OCR Result..."), "spellcheck", 0,
				  m_view, SLOT(slOcrSpellCheck()),
				  actionCollection(), "ocrSpellCheck" );
}


void Kooka::saveProperties(KConfig *config)
{
    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored

   //if (!m_view->currentURL().isNull())
   //     config->writePathEntry("lastURL", m_view->currentURL());
   kdDebug(28000) << "In kooka's saveProperties !" << endl;
   config->setGroup( KOOKA_STATE_GROUP );
   config->writeEntry( PREFERENCE_DIA_TAB, m_prefDialogIndex );
   m_view->saveProperties( config );
}

void Kooka::readProperties(KConfig *config)
{
   (void) config;
    // the 'config' object points to the session managed
    // config file.  this function is automatically called whenever
    // the app is being restored.  read in here whatever you wrote
    // in 'saveProperties'
   config->setGroup( KOOKA_STATE_GROUP );
   m_prefDialogIndex = config->readNumEntry( PREFERENCE_DIA_TAB, 0 );
   // QString url = config->readPathEntry("lastURL");

}

void Kooka::dragEnterEvent(QDragEnterEvent *event)
{
    // accept uri drops only
    event->accept(KURLDrag::canDecode(event));
}


void Kooka::filePrint()
{
    // this slot is called whenever the File->Print menu is selected,
    // the Print shortcut is pressed (usually CTRL+P) or the Print toolbar
    // button is clicked
    m_view->print();

}

void Kooka::optionsShowScanParams()
{
   m_view->slSetScanParamsVisible( m_scanParamsAction->isChecked() );
}

void Kooka::optionsShowPreviewer()
{
   m_view->slSetTabWVisible( m_previewerAction->isChecked());
}

void Kooka::optionsConfigureToolbars()
{
    // use the standard toolbar editor
    saveMainWindowSettings(KGlobal::config(), autoSaveGroup());
    KEditToolbar dlg(factory());
    connect(&dlg, SIGNAL(newToolbarConfig()), SLOT(newToolbarConfig()));
    dlg.exec();
}

void Kooka::newToolbarConfig()
{
    // OK/Apply pressed in the toolbar editor
    applyMainWindowSettings(KGlobal::config(), autoSaveGroup());
}


void Kooka::optionsPreferences()
{
    // popup some sort of preference dialog, here
    KookaPref dlg;
    dlg.showPage(m_prefDialogIndex);
    connect(&dlg,SIGNAL(dataSaved()),m_view,SLOT(slFreshUpThumbView()));

    if (dlg.exec())
    {
        // redo your settings
       m_prefDialogIndex = dlg.activePageIndex();

       m_view->gallery()->setAllowRename(dlg.allowGalleryRename());
       // m_view->slFreshUpThumbView();
    }
}


void Kooka::optionsOcrPreferences()
{
    m_prefDialogIndex = 4;				// go to OCR page
    optionsPreferences();
}


void Kooka::changeStatusbar(const QString& text)
{
    // display the text on the statusbar
    statusBar()->changeItem( text, KookaView::StatusTemp );
}

void Kooka::changeCaption(const QString& text)
{
    // display the text on the caption
    setCaption(text);
}

void Kooka::slMirrorVertical( void )
{
   m_view->slMirrorImage( KookaView::MirrorVertical );
}

void Kooka::slMirrorHorizontal( void )
{
    m_view->slMirrorImage( KookaView::MirrorHorizontal );
}

void Kooka::slRotateClockWise( void )
{
   m_view->slRotateImage( 90 );
}

void Kooka::slRotateCounterClockWise( void )
{
   m_view->slRotateImage( -90 );

}

void Kooka::slRotate180( void )
{
    m_view->slMirrorImage( KookaView::MirrorBoth );
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
    kdDebug(28000) << k_funcinfo << "hc=" << haveConnection << endl;

    scanAction->setEnabled(haveConnection);
    previewAction->setEnabled(haveConnection);
    paramsAction->setEnabled(haveConnection);

    setCaption(m_view->scannerName());
}


void Kooka::slotUpdateRectangleActions(bool haveSelection)
{
    kdDebug(28000) << k_funcinfo << "hs=" << haveSelection << endl;

    ocrSelectAction->setEnabled(haveSelection);
    newFromSelectionAction->setEnabled(haveSelection);
}


void Kooka::slotUpdateGalleryActions(bool isDir,int howmanySelected)
{
    kdDebug(28000) << k_funcinfo << "isdir=" << isDir << " howmany=" << howmanySelected << endl;

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
    kdDebug(28000) << k_funcinfo << "loaded=" << isLoaded << " isDir=" << isDir << endl;
    unloadImageAction->setEnabled(isLoaded || isDir);
}


void Kooka::slotUpdateOcrResultActions(bool haveText)
{
    kdDebug(28000) << k_funcinfo << "have=" << haveText << endl;
    m_saveOCRTextAction->setEnabled(haveText);
    ocrSpellAction->setEnabled(haveText);
}


void Kooka::slotOpenWithMenu()
{
    m_view->gallery()->showOpenWithMenu(openWithMenu);
}
