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
#include "kookaview.h"
#include "resource.h"

#include "kookapref.h"
#include "imgprintdialog.h"

#include <qlineedit.h>
#include <qprinter.h>
#include <qprintdialog.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kmenubar.h>
#include <kaccel.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <kconfig.h>
#include <kprinter.h>
#include <kstatusbar.h>
#include <kurl.h>
#include <kurlrequesterdlg.h>
#include <qstrlist.h>
#include <kedittoolbar.h>
#include <kmessagebox.h>
#include <kdockwidget.h>
#include <kparts/partmanager.h>
#include <kstdaccel.h>
#include <kaction.h>
#include <kstdaction.h>
#include <qiconset.h>
#include <kurldrag.h>

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
    statusBar()->insertItem( QString(), KookaView::StatusTemp );
    statusBar()->show();

    // allow the view to change the statusbar and caption
    connect(m_view, SIGNAL(signalChangeStatusbar(const QString&)),
            this,   SLOT(changeStatusbar(const QString&)));
    connect(m_view, SIGNAL(signalCleanStatusbar(void)),
            this, SLOT(cleanStatusbar()));
    connect(m_view, SIGNAL(signalChangeCaption(const QString&)),
            this,   SLOT(changeCaption(const QString&)));

    changeCaption( i18n( "KDE Scanning" ));

    setAutoSaveSettings(  QString::fromLatin1("General Options"),
                          true );
}

void Kooka::createMyGUI( KParts::Part *part )
{
    kdDebug(28000) << "Part changed, Creating gui" << endl;
    createGUI(part);

}

Kooka::~Kooka()
{
   KConfig *konf = KGlobal::config ();
   m_view->slCloseScanDevice();
   writeDockConfig ( konf, DOCK_SIZES );
   delete m_printer;
}

void Kooka::startup( void )
{
   kdDebug(29000) << "Starting startup !" << endl;
   if( m_view ) m_view->loadStartupImage();
}


void Kooka::setupActions()
{

    KStdAction::print(this, SLOT(filePrint()), actionCollection());
    KStdAction::quit(this , SLOT(close()), actionCollection());

    KStdAction::keyBindings(guiFactory(), SLOT(configureShortcuts()), 
actionCollection());
    KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()),
				  actionCollection());
    KStdAction::preferences(this, SLOT(optionsPreferences()), actionCollection());

    m_view->createDockMenu(actionCollection(), this, "settings_show_docks" );

    /* Image Viewer action Toolbar - OCR, Scaling etc. */
    (void) new KAction(i18n("&OCR Image..."), "ocr", CTRL+Key_O,
		       m_view, SLOT(doOCR()),
		       actionCollection(), "ocrImage" );

    (void) new KAction(i18n("O&CR on Selection..."), "ocr-select", CTRL+Key_C,
		       m_view, SLOT(doOCRonSelection()),
		       actionCollection(), "ocrImageSelect" );

    KAction *act;
    act =  new KAction(i18n("Scale to W&idth"), "scaletowidth", CTRL+Key_I,
		       m_view, SLOT( slIVScaleToWidth()),
		       actionCollection(), "scaleToWidth" );
    m_view->connectViewerAction( act );

    act = new KAction(i18n("Scale to &Height"), "scaletoheight", CTRL+Key_H,
		       m_view, SLOT( slIVScaleToHeight()),
		       actionCollection(), "scaleToHeight" );
    m_view->connectViewerAction( act );

    act = new KAction(i18n("Original &Size"), "scaleorig", CTRL+Key_S,
                      m_view, SLOT( slIVScaleOriginal()),
                      actionCollection(), "scaleOriginal" );
    m_view->connectViewerAction( act );

#ifdef QICONSET_HONOUR_ON_OFF
    /* The Toggleaction does not seem to handle the on/off icon from QIconSet */
    QIconSet lockSet;
    lockSet.setPixmap(BarIcon("lock")  , QIconSet::Automatic, QIconSet::Normal, QIconSet::On );
    lockSet.setPixmap(BarIcon("unlock"), QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
    act = new KToggleAction ( i18n("Keep &Zoom Setting"), lockSet, CTRL+Key_Z,
                              actionCollection(), "keepZoom" );
#else
    act = new KToggleAction( i18n("Keep &Zoom Setting"), BarIcon("lockzoom"), CTRL+Key_Z,
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

    (void) new KAction(i18n("Create From Selectio&n"), "crop", CTRL+Key_N,
		       m_view, SLOT( slCreateNewImgFromSelection() ),
		       actionCollection(), "createFromSelection" );

    (void) new KAction(i18n("Mirror Image &Vertically"), "mirror-vert", CTRL+Key_V,
		       this, SLOT( slMirrorVertical() ),
		       actionCollection(), "mirrorVertical" );

    (void) new KAction(i18n("&Mirror Image Horizontally"), "mirror-horiz", CTRL+Key_M,
		       this, SLOT( slMirrorHorizontal() ),
		       actionCollection(), "mirrorHorizontal" );

    (void) new KAction(i18n("Mirror Image &Both Directions"), "mirror-both", CTRL+Key_B,
		       this, SLOT( slMirrorBoth() ),
		       actionCollection(), "mirrorBoth" );

    (void) new KAction(i18n("Open Image in &Graphic Application..."), "fileopen", CTRL+Key_G,
		       m_view, SLOT( slOpenCurrInGraphApp() ),
		       actionCollection(), "openInGraphApp" );

    act = new KAction(i18n("&Rotate Image Clockwise"), "rotate_cw", CTRL+Key_R,
		      this, SLOT( slRotateClockWise() ),
		       actionCollection(), "rotateClockwise" );
    m_view->connectViewerAction( act );

    act = new KAction(i18n("Rotate Image Counter-Clock&wise"), "rotate_ccw", CTRL+Key_W,
		       this, SLOT( slRotateCounterClockWise() ),
		       actionCollection(), "rotateCounterClockwise" );
    m_view->connectViewerAction( act );

    act = new KAction(i18n("Rotate Image 180 &Degrees"), "rotate", CTRL+Key_D,
		       this, SLOT( slRotate180() ),
		       actionCollection(), "upsitedown" );
    m_view->connectViewerAction( act );

    /* Gallery actions */
    act = new KAction(i18n("&Create Folder..."), "folder_new", 0,
		      m_view->gallery(), SLOT( slotCreateFolder() ),
		       actionCollection(), "foldernew" );
    m_view->connectGalleryAction( act );

    act = new KAction(i18n("&Save Image..."), "filesave", 0,
		      m_view->gallery(), SLOT( slotExportFile() ),
		       actionCollection(), "saveImage" );
    m_view->connectGalleryAction( act );

    act = new KAction(i18n("&Import Image..."), "inline_image", 0,
		      m_view->gallery(), SLOT( slotImportFile() ),
		       actionCollection(), "importImage" );
    m_view->connectGalleryAction( act );

    act = new KAction(i18n("&Delete Image"), "edittrash", 0,
		      m_view->gallery(), SLOT( slotDeleteItems() ),
		       actionCollection(), "deleteImage" );
    m_view->connectGalleryAction( act );

    act = new KAction(i18n("&Unload Image"), "fileclose", 0,
		      m_view->gallery(), SLOT( slotUnloadItems() ),
		       actionCollection(), "unloadImage" );
    m_view->connectGalleryAction( act );

#if 0
    /* not yet supported actions - coming post 3.1 */
    (void) new KAction(i18n("&Load Scan Parameters"), "bookmark_add", CTRL+Key_L,
                       m_view, SLOT(slLoadScanParams()),
                       actionCollection(), "loadscanparam" );

    (void) new KAction(i18n("Save &Scan Parameters"), "bookmark_add", CTRL+Key_S,
		       m_view, SLOT(slSaveScanParams()),
		       actionCollection(), "savescanparam" );
#endif

    (void) new KAction(i18n("Select Scan Device"), "scanner", 0,
		       m_view, SLOT( slSelectDevice()),
		       actionCollection(), "selectsource" );

    (void) new KAction( i18n("Enable All Warnings && Messages"), 0,
			this,  SLOT(slEnableWarnings()),
			actionCollection(), "enable_msgs");


    m_saveOCRTextAction = new KAction( i18n("Save OCR Res&ult Text"), "filesaveas", CTRL+Key_U,
                                       m_view, SLOT(slSaveOCRResult()),
                                       actionCollection(), "saveOCRResult");
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

#if 0
void Kooka::dropEvent(QDropEvent *event)
{
    // this is a very simplistic implementation of a drop event.  we
    // will only accept a dropped URL.  the Qt dnd code can do *much*
    // much more, so please read the docs there
    KURL::List uri;

    // see if we can decode a URI.. if not, just ignore it
    if (KURLDrag::decode(event, uri) && !uri.isEmpty())
    {
        // okay, we have a URI.. process it
        const KURL &url = uri.first();
	kdDebug(29000) << "Importing URI " << url.url() << endl;

        // TODO: Do something with url
        // Waba: See also setAcceptDrops() above
    }
}

void Kooka::fileNew()
{
    // this slot is called whenever the File->New menu is selected,
    // the New shortcut is pressed (usually CTRL+N) or the New toolbar
    // button is clicked

    // create a new window
    (new Kooka)->show();
}

void Kooka::fileOpen()
{
    // this slot is called whenever the File->Open menu is selected,
    // the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
    // button is clicked
}

void Kooka::fileSave()
{
    // this slot is called whenever the File->Save menu is selected,
    // the Save shortcut is pressed (usually CTRL+S) or the Save toolbar
    // button is clicked

    // save the current file
}


void Kooka::fileSaveAs()
{
    // this slot is called whenever the File->Save As menu is selected,
   QStrList strlist;
   strlist.append( "BMP" );
   strlist.append( "JPEG" );
   FormatDialog fd( 0, "FormatDialog", &strlist );
   fd.exec();

}
#endif

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
    KookaPreferences dlg;
    dlg.showPage( m_prefDialogIndex );
    connect( &dlg, SIGNAL( dataSaved() ), m_view, SLOT(slFreshUpThumbView()));

    if (dlg.exec())
    {
        // redo your settings
       m_prefDialogIndex = dlg.activePageIndex();
       // m_view->slFreshUpThumbView();
    }
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

void Kooka::slMirrorBoth( void )
{
    m_view->slMirrorImage( KookaView::MirrorBoth );
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
   m_view->slRotateImage( 180 );
}

void Kooka::slEnableWarnings( )
{
   KMessageBox::information (this, i18n("All messages and warnings will now be shown."));
   KMessageBox::enableAllMessages();
   kapp->config()->reparseConfiguration();
}

#include "kooka.moc"
