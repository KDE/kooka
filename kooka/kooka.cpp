/*
 * kooka.cpp
 *
 * Copyright (C) 2000 Klaas Freitag <freitag@suse.de>
 */
#include "kooka.h"
#include "kookaview.h"
#include "resource.h"

#include "kookapref.h"

#include <qdragobject.h>
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
#include <kkeydialog.h>
#include <kaccel.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <kconfig.h>
#include <kprinter.h>
#include <kurl.h>
#include <kurlrequesterdlg.h>
#include <qstrlist.h>
#include <kedittoolbar.h>

#include <kstdaccel.h>
#include <kaction.h>
#include <kstdaction.h>

Kooka::Kooka()
    : KMainWindow( 0, "Kooka" ),
      m_view(new KookaView(this)),
      m_printer(0)
{
    // accept dnd
    setAcceptDrops(true);

    // tell the KMainWindow that this is indeed the main widget
    setCentralWidget(m_view);

    // then, setup our actions
    setupActions();

    // and a status bar
    statusBar()->show();

    // allow the view to change the statusbar and caption
    connect(m_view, SIGNAL(signalChangeStatusbar(const QString&)),
            this,   SLOT(changeStatusbar(const QString&)));
    connect(m_view, SIGNAL(signalCleanStatusbar(void)),
	    this, SLOT(cleanStatusbar()));
    connect(m_view, SIGNAL(signalChangeCaption(const QString&)),
            this,   SLOT(changeCaption(const QString&)));

    changeCaption( i18n( "KDE2 Scanning" ));
}

Kooka::~Kooka()
{
   delete( m_view );
}

void Kooka::load(const QString& url)
{
    QString target;
    (void) url;
    // the below code is what you should normally do.  in this
    // example case, we want the url to our own.  you probably
    // want to use this code instead for your app

#if 0
    // download the contents
    if (KIONetAccess::download(url, target))
    {
        // set our caption
        setCaption(url);

        // load in the file (target is always local)
        loadFile(target);

        // and remove the temp file
        KIONetAccess::removeTempFile(target);
    }
#endif

}

void Kooka::startup( void )
{
   kdDebug(29000) << "Starting startup !" << endl;
   if( m_view ) m_view->loadStartupImage();
}


void Kooka::setupActions()
{
#if 0
    KStdAction::openNew(this, SLOT(fileNew()), actionCollection());
    KStdAction::open(this, SLOT(fileOpen()), actionCollection());
    KStdAction::save(this, SLOT(fileSave()), actionCollection());
    KStdAction::saveAs(this, SLOT(fileSaveAs()), actionCollection());
#endif

    KStdAction::print(this, SLOT(filePrint()), actionCollection());
    KStdAction::quit(kapp, SLOT(quit()), actionCollection());

    m_toolbarAction = KStdAction::showToolbar(this, SLOT(optionsShowToolbar()),
					      actionCollection());
    m_statusbarAction = KStdAction::showStatusbar(this, SLOT(optionsShowStatusbar()),
						  actionCollection());
    m_scanParamsAction = new KToggleAction(i18n("Show scan &parameter"), 0, this,
					   SLOT(optionsShowScanParams()), actionCollection(),
					   "show_scanparams" );
    m_scanParamsAction->setChecked( true );
#if 0 
    m_previewerAction = new KToggleAction(i18n("Show scan pre&view"), 0, this,
					   SLOT(optionsShowPreviewer()), actionCollection(),
					  "show_preview" );
#endif
    KStdAction::keyBindings(this, SLOT(optionsConfigureKeys()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()),
				  actionCollection());
    KStdAction::preferences(this, SLOT(optionsPreferences()), actionCollection());


    /* Image Viewer action Toolbar - OCR, Scaling etc. */
    (void) new KAction(i18n("&OCR image..."), "ocr", CTRL+Key_O,
		       m_view, SLOT(doOCR()),
		       actionCollection(), "ocrImage" );

    (void) new KAction(i18n("O&CR on selection..."), "ocr-select", CTRL+Key_C,
		       m_view, SLOT(doOCRonSelection()),
		       actionCollection(), "ocrImageSelect" );

    (void) new KAction(i18n("Scale to W&idth"), "scaletowidth", CTRL+Key_I,
		       m_view, SLOT( slIVScaleToWidth()),
		       actionCollection(), "scaleToWidth" );

    (void) new KAction(i18n("Scale to &Height"), "scaletoheight", CTRL+Key_H,
		       m_view, SLOT( slIVScaleToHeight()),
		       actionCollection(), "scaleToHeight" );
    
    (void) new KAction(i18n("Original &Size"), "scaleorig", CTRL+Key_S,
		       m_view, SLOT( slIVScaleOriginal()),
		       actionCollection(), "scaleOriginal" );

    (void) new KAction(i18n("Create from selectio&n"), "crop", CTRL+Key_N,
		       m_view, SLOT( slCreateNewImgFromSelection() ),
		       actionCollection(), "createFromSelection" );
    
    (void) new KAction(i18n("Mirror image &vertically"), "mirror-vert", CTRL+Key_V,
		       this, SLOT( slMirrorVertical() ),
		       actionCollection(), "mirrorVertical" );

    (void) new KAction(i18n("&Mirror image horizontally"), "mirror-horiz", CTRL+Key_M,
		       this, SLOT( slMirrorHorizontal() ),
		       actionCollection(), "mirrorHorizontal" );

    (void) new KAction(i18n("Mirror image &both directions"), "mirror-both", CTRL+Key_B,
		       this, SLOT( slMirrorBoth() ),
		       actionCollection(), "mirrorBoth" );

    (void) new KAction(i18n("Open image in &graphic application"), "fileopen", CTRL+Key_G,
		       m_view, SLOT( slOpenCurrInGraphApp() ),
		       actionCollection(), "openInGraphApp" );

    (void) new KAction(i18n("&Rotate image clockwise"), "rotate_cw", CTRL+Key_R,
		      this, SLOT( slRotateClockWise() ),
		       actionCollection(), "rotateClockwise" );

    (void) new KAction(i18n("Rotate image counter-clock&wise"), "rotate_ccw", CTRL+Key_W,
		       this, SLOT( slRotateCounterClockWise() ),
		       actionCollection(), "rotateCounterClockwise" );
    (void) new KAction(i18n("Rotate image 180 &degrees"), "rotate", CTRL+Key_D,
		       this, SLOT( slRotate180() ),
		       actionCollection(), "upsitedown" );
#if 0
    (void) new KAction(i18n("Save Scanparameters"), "savedefaultparams", CTRL+Key_S,
		       m_view, SLOT(slSaveScanParams()),
		       actionCollection(), "savescanparam" );
#endif

    createGUI("kookaui.rc");

}


void Kooka::saveProperties(KConfig *config)
{
    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored
    
   //if (m_view->currentURL() != QString::null)
   //     config->writeEntry("lastURL", m_view->currentURL());
   kdDebug(28000) << "In kooka's saveProperties !" << endl;
   m_view->saveProperties( config );
}

void Kooka::readProperties(KConfig *config)
{
   (void) config;
    // the 'config' object points to the session managed
    // config file.  this function is automatically called whenever
    // the app is being restored.  read in here whatever you wrote
    // in 'saveProperties'

   // QString url = config->readEntry("lastURL"); 

}

void Kooka::dragEnterEvent(QDragEnterEvent *event)
{
    // accept uri drops only
    event->accept(QUriDrag::canDecode(event));
}

void Kooka::dropEvent(QDropEvent *event)
{
    // this is a very simplistic implementation of a drop event.  we
    // will only accept a dropped URL.  the Qt dnd code can do *much*
    // much more, so please read the docs there
    QStrList uri;

    // see if we can decode a URI.. if not, just ignore it
    if (QUriDrag::decode(event, uri))
    {
        // okay, we have a URI.. process it
        QString url, target;
        url = uri.first();

        // load in the file
        load(url);
    }
}

#if 0
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
   strlist.append( "BMP" );
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
    if (!m_printer) m_printer = new KPrinter;
    if (m_printer->setup(this))
    {
       // setup the printer.  with Qt, you always "print" to a
       // QPainter.. whether the output medium is a pixmap, a screen,
       // or paper
       QPainter p;
       QPaintDeviceMetrics metrics( m_printer );
       p.begin( m_printer);
       // we let our view do the actual printing
       m_view->print(&p, m_printer, metrics );

       // and send the result to the printer
       p.end();
    }
}

void Kooka::optionsShowToolbar()
{
    // this is all very cut and paste code for showing/hiding the
    // toolbar
    if (m_toolbarAction->isChecked())
        toolBar("mainToolBar")->show();
    else
        toolBar("mainToolBar")->hide();
}

void Kooka::optionsShowStatusbar()
{
    // this is all very cut and paste code for showing/hiding the
    // statusbar
    if (m_statusbarAction->isChecked())
        statusBar()->show();
    else
        statusBar()->hide();
}

void Kooka::optionsShowScanParams()
{
   m_view->slSetScanParamsVisible( m_scanParamsAction->isChecked() );
}

void Kooka::optionsShowPreviewer()
{
   m_view->slSetTabWVisible( m_previewerAction->isChecked());
}


void Kooka::optionsConfigureKeys()
{
    KKeyDialog::configureKeys(actionCollection(), "kookaui.rc");
}

void Kooka::optionsConfigureToolbars()
{
    // use the standard toolbar editor
    KEditToolbar dlg(actionCollection());
    if (dlg.exec())
    {
        // recreate our GUI
        createGUI( "kookaui.rc");
    } 
}

void Kooka::optionsPreferences()
{
    // popup some sort of preference dialog, here
    KookaPreferences dlg;
    if (dlg.exec())
    {
        // redo your settings
    }
}

void Kooka::changeStatusbar(const QString& text)
{
    // display the text on the statusbar
    statusBar()->message(text);
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

#include "kooka.moc"
