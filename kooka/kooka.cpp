/*
 * kooka.cpp
 *
 * Copyright (C) 2000 Klaas Freitag <freitag@suse.de>
 */
#include "kooka.h"
#include "resource.h"

#include "kookapref.h"
#include "img_saver.h"


#include <qdragobject.h>
#include <qlineedit.h>
#include <qprinter.h>
#include <qprintdialog.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>

#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmenubar.h>
#include <kkeydialog.h>
#include <kaccel.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <kconfig.h>
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
    connect(m_view, SIGNAL(signalChangeCaption(const QString&)),
            this,   SLOT(changeCaption(const QString&)));

}

Kooka::~Kooka()
{
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

void Kooka::setupActions()
{
#if 0
    KStdAction::openNew(this, SLOT(fileNew()), actionCollection());
    KStdAction::open(this, SLOT(fileOpen()), actionCollection());
    KStdAction::save(this, SLOT(fileSave()), actionCollection());
#endif

    KStdAction::saveAs(this, SLOT(fileSaveAs()), actionCollection());
    KStdAction::print(this, SLOT(filePrint()), actionCollection());
    KStdAction::quit(kapp, SLOT(quit()), actionCollection());

    m_toolbarAction = KStdAction::showToolbar(this, SLOT(optionsShowToolbar()),
					      actionCollection());
    m_statusbarAction = KStdAction::showStatusbar(this, SLOT(optionsShowStatusbar()),
						  actionCollection());
    m_scanParamsAction = new KToggleAction(I18N("Show scan &parameter"), 0, this,
					   SLOT(optionsShowScanParams()), actionCollection(),
					   "show_scanparams" );
    m_scanParamsAction->setChecked( true );
#if 0 
    m_previewerAction = new KToggleAction(I18N("Show scan pre&view"), 0, this,
					   SLOT(optionsShowPreviewer()), actionCollection(),
					  "show_preview" );
#endif
    KStdAction::keyBindings(this, SLOT(optionsConfigureKeys()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());
    KStdAction::preferences(this, SLOT(optionsPreferences()), actionCollection());

    (void) new KAction(I18N("&OCR"), "edit", CTRL+Key_O, this, SLOT(slStartOCR()),
		       actionCollection(), "ocr" );
    
    createGUI("kookaui.rc");
}

void Kooka::saveProperties(KConfig *config)
{
   (void) config;
    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored
    
   //if (m_view->currentURL() != QString::null)
   //     config->writeEntry("lastURL", m_view->currentURL()); 
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

#endif
void Kooka::fileSaveAs()
{
    // this slot is called whenever the File->Save As menu is selected,
   QStrList strlist;
   strlist.append( "BMP" );
   strlist.append( "JPEG" );
   FormatDialog fd( 0, "FormatDialog", &strlist );
   fd.exec();
   
}


void Kooka::slStartOCR()
{
   changeStatusbar( I18N( "Starting OCR" ));
   m_view->doOCR();
}


void Kooka::filePrint()
{
    // this slot is called whenever the File->Print menu is selected,
    // the Print shortcut is pressed (usually CTRL+P) or the Print toolbar
    // button is clicked
    if (!m_printer) m_printer = new QPrinter;
    if (QPrintDialog::getPrinterSetup(m_printer))
    {
        // setup the printer.  with Qt, you always "print" to a
        // QPainter.. whether the output medium is a pixmap, a screen,
        // or paper
        QPainter p;
        p.begin(m_printer);

        // we let our view do the actual printing
        QPaintDeviceMetrics metrics(m_printer);
        m_view->print(&p, metrics.height(), metrics.width());

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

#include "kooka.moc"