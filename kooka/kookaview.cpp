/**************************************************************************
              kookaview.cpp  -  kookas visible stuff
                             -------------------
    begin                : ?
    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de

    $Id$
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

#include <qlabel.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qsplitter.h>
#include <qstrlist.h>
#include <qpaintdevice.h>
#include <qpaintdevicemetrics.h>
#include <qpopupmenu.h>
#include <qwidgetstack.h>
#include <qimage.h>

#include <kurl.h>
#include <krun.h>
#include <kapplication.h>
#include <kstatusbar.h>
#include <kconfig.h>
#include <kdebug.h>
#include <ktrader.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <keditcl.h>
#include <kled.h>
#include <kcombobox.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kshortcut.h>
#include <kdockwidget.h>
#include <kpopupmenu.h>

#include <kparts/componentfactory.h>

#include "kscandevice.h"
#include "imgscaninfo.h"
#include "devselector.h"
#include "imgsaver.h"
#include "kookapref.h"
#include "imagenamecombo.h"
#include "thumbview.h"
#include "dwmenuaction.h"
#include "kookaimage.h"
#include "kookaimagemeta.h"
#include "ocrresedit.h"
#include "kookaprint.h"
#include "imgprintdialog.h"
#include "adddevice.h"
#include "scanparamsdialog.h"
#include "kookagallery.h"
#include "scanpackager.h"

#include "photocopyprintdialogpage.h"

#include "kookaview.h"
#include "kookaview.moc"



#define STARTUP_IMG_SELECTION   "SelectedImageOnStartup"


KookaView::KookaView( KParts::DockMainWindow *parent, const QCString& deviceToUse)
   : QObject(),
     m_ocrResultImg(0),
     ocrFabric(NULL),
     m_mainDock(0),
     m_dockScanParam(0),
     m_dockThumbs(0),
     m_dockPackager(0),
     m_dockPreview(0),
     m_dockOCRText(0),
     m_mainWindow(parent),
     m_ocrResEdit(0)
{
   KIconLoader *loader = KGlobal::iconLoader();
   scan_params = NULL;
   preview_canvas = NULL;
   isPhotoCopyMode=false;

   m_parent = dynamic_cast<QWidget *>(parent);		// for dialogues, etc

   m_mainDock = parent->createDockWidget( "Kookas MainDock",
                                          loader->loadIcon( "folder_image", KIcon::Small ),
                                          0L, i18n("Image Viewer"));
   m_mainDock->setEnableDocking(KDockWidget::DockNone );
   m_mainDock->setDockSite( KDockWidget::DockFullSite );

   parent->setView( m_mainDock);
   parent->setMainDockWidget( m_mainDock);

   img_canvas  = new ImageCanvas( m_mainDock );
   img_canvas->setMinimumSize(100,200);
   img_canvas->enableContextMenu(true);

   // Connections ImageCanvas --> myself
   connect(img_canvas,SIGNAL(imageReadOnly(bool)),SLOT(slViewerReadOnly(bool)));
   connect(img_canvas,SIGNAL( newRect(QRect)),SLOT(slotSelectionChanged(QRect)));
   
   KPopupMenu *ctxtmenu = static_cast<KPopupMenu*>(img_canvas->contextMenu());
   if( ctxtmenu )
       ctxtmenu->insertTitle(i18n("Image View"));
   m_mainDock->setWidget( img_canvas );

   /** Thumbview **/
   m_dockThumbs = parent->createDockWidget( "Thumbs",
					    loader->loadIcon( "thumbnail", KIcon::Small ),
					    0L,  i18n("Thumbnails"));
   m_dockThumbs->setDockSite(KDockWidget::DockFullSite );

   /* thumbnail viewer widget */
   m_thumbview = new ThumbView( m_dockThumbs);
   m_dockThumbs->setWidget( m_thumbview );

   m_dockThumbs->manualDock( m_mainDock,              // dock target
			     KDockWidget::DockBottom, // dock site
			     20 );                  // relation target/this (in percent)

   /** Packager Dock **/
   /* A new packager to contain the already scanned images */
   m_dockPackager = parent->createDockWidget( "Scanpackager",
					    loader->loadIcon( "palette_color", KIcon::Small ),
					    0L, i18n("Gallery"));
   m_dockPackager->setDockSite(KDockWidget::DockFullSite);
   m_gallery = new KookaGallery(m_dockPackager);
   m_dockPackager->setWidget( m_gallery );
   m_dockPackager->manualDock( m_mainDock,              // dock target
                         KDockWidget::DockLeft, // dock site
                         30 );                  // relation target/this (in percent)

   ScanPackager *packager = m_gallery->galleryTree();

   // Connections ScanPackager <--> ThumbView
   connect(packager,SIGNAL(showItem(const KFileTreeViewItem *)),
           m_thumbview,SLOT(slotSelectImage(const KFileTreeViewItem *)));
   connect(m_thumbview,SIGNAL(selectFromThumbnail(const KURL &)),
           packager,SLOT(slSelectImage(const KURL &)));

   // Connections ScanPackager --> myself
   connect(packager, SIGNAL(selectionChanged()),
           SLOT(slotGallerySelectionChanged()));
   connect(packager,SIGNAL(showImage(const KookaImage *,bool)),
	   SLOT(slotLoadedImageChanged(const KookaImage *,bool)));


   ImageNameCombo *recentFolder = m_gallery->galleryRecent();

   // Connections ScanPackager <--> recent folder history
   connect(packager,SIGNAL(galleryPathChanged( KFileTreeBranch *,const QString &)),
           recentFolder,SLOT(slotPathChanged(KFileTreeBranch *,const QString &)));
   connect(packager,SIGNAL(galleryDirectoryRemoved(KFileTreeBranch *,const QString &)),
           recentFolder,SLOT(slotPathRemoved(KFileTreeBranch *,const QString &)));
   connect(recentFolder,SIGNAL(pathSelected(const QString &,const QString &)),
           packager,SLOT(slotSelectDirectory(const QString &,const QString &)));

   /* the object from the kscan lib to handle low level scanning */
   m_dockScanParam = parent->createDockWidget( "Scan Parameter",
 					     loader->loadIcon( "folder", KIcon::Small ),
 					     0L, i18n("Scan Parameter"));
   //
   m_dockScanParam->setDockSite(KDockWidget::DockFullSite);

   m_dockScanParam->setWidget( 0 ); // later
   sane = new KScanDevice( m_dockScanParam );
   Q_CHECK_PTR(sane);
   if (!deviceToUse.isEmpty() && deviceToUse!="gallery")
       sane->addUserSpecifiedDevice(deviceToUse,"on command line",true);

   m_dockScanParam->manualDock( m_dockPackager,              // dock target
				KDockWidget::DockBottom, // dock site
				20 );                  // relation target/this (in percent)
   m_dockScanParam->hide();

   /* select the scan device, either user or from config, this creates and assembles
    * the complete scanner options dialog
    * scan_params must be zero for that */

   m_dockPreview = parent->createDockWidget( "Preview ",
					   loader->loadIcon( "viewmag", KIcon::Small ),
					   0L, i18n("Scan Preview"));

   preview_canvas = new Previewer( m_dockPreview );
   {
       preview_canvas->setMinimumSize( 100,100);

      /* since the scan_params will be created in slSelectDevice, do the
       * connections later
       */
   }
   m_dockPreview->setWidget( preview_canvas );
   m_dockPreview->manualDock( m_mainDock,              // dock target
			      KDockWidget::DockCenter, // dock site
			      100 );                  // relation target/this (in percent)

   /* Create a text editor part for ocr results */

   m_dockOCRText = parent->createDockWidget( "OCRResults",
                                             loader->loadIcon("edit", KIcon::Small ),
                                             0L, i18n("OCR Result Text"));
   // m_textEdit
   m_ocrResEdit  = new ocrResEdit( m_dockOCRText );

   if( m_ocrResEdit )
   {
       m_dockOCRText->setWidget( m_ocrResEdit ); // m_textEdit->widget() );
       m_dockOCRText->manualDock( m_dockThumbs,              // dock target
                                  KDockWidget::DockCenter, // dock site
                                  100 );                  // relation target/this (in percent)

       m_ocrResEdit->setTextFormat( Qt::PlainText );
       m_ocrResEdit->setWordWrap( QTextEdit::NoWrap );
       // m_dockOCRText->hide();
   }

   if( slSelectDevice(deviceToUse,false))
   {
      /* Load from config which tab page was selected last time */
   }

   // Connections KScanDevice --> myself
   connect(sane,SIGNAL(sigScanFinished(KScanStat)),SLOT(slScanFinished(KScanStat)));
   connect(sane,SIGNAL(sigScanFinished(KScanStat)),SLOT(slPhotoCopyScan(KScanStat)));
   /* New image created after scanning now call save dialog*/
   connect(sane,SIGNAL(sigNewImage(QImage *,ImgScanInfo *)),SLOT(slNewImageScanned(QImage *,ImgScanInfo *)));
   /* New image created after scanning now print it*/    
   connect(sane,SIGNAL(sigNewImage(QImage *,ImgScanInfo *)),SLOT(slPhotoCopyPrint(QImage *,ImgScanInfo *)));
   /* New preview image */
   connect(sane,SIGNAL(sigNewPreview(QImage *,ImgScanInfo *)),SLOT(slNewPreview(QImage *,ImgScanInfo *)));

   connect(sane,SIGNAL(sigScanStart()),SLOT(slScanStart()));
   connect(sane,SIGNAL(sigAcquireStart()),SLOT( slAcquireStart()));

   // Connections ScanPackager --> myself
   connect(packager,SIGNAL(showImage(const KookaImage *,bool)),SLOT(slShowAImage(const KookaImage *)));
   connect(packager,SIGNAL(aboutToShowImage(const KURL &)),SLOT( slStartLoading(const KURL &)));
   /* Packager unloads the image */
   connect(packager,SIGNAL(unloadImage(KookaImage *)),SLOT(slUnloadAImage(KookaImage *)));

   // Connections ScanPackager --> ThumbView
   connect(packager,SIGNAL(imageChanged(const KFileItem *)),
           m_thumbview,SLOT(slotImageChanged(const KFileItem *)));
   connect(packager,SIGNAL(fileRenamed(const KFileTreeViewItem *,const QString &)),
           m_thumbview,SLOT(slotImageRenamed(const KFileTreeViewItem *,const QString &)));
   connect(packager,SIGNAL(fileDeleted(const KFileItem *)),
	    m_thumbview,SLOT(slotImageDeleted(const KFileItem *)));

   packager->openRoots();

   /* Status Bar */
   KStatusBar *statBar = m_mainWindow->statusBar();

   // statBar->insertItem(QString("1"), SBAR_ZOOM,  0, true );
   statBar->insertItem( QString("-"), StatusImage,  0, true );

   /* Set a large enough size */
   int w = statBar->fontMetrics().
           width(img_canvas->imageInfoString(2000, 2000, 48)+"--");
   kdDebug(28000) << "Fixed size for status bar " << w << endl;
   statBar->setItemFixed( StatusImage, w );
}


KookaView::~KookaView()
{
    saveProperties(KGlobal::config());
    delete preview_canvas;
    kdDebug(28000) << k_funcinfo << "finished" << endl;
}

void KookaView::slViewerReadOnly( bool )
{
    /* retrieve actions that could change the image */
}


bool KookaView::slSelectDevice(const QCString& useDevice,bool alwaysAsk)
{
    kdDebug(28000) << k_funcinfo << "use device [" << useDevice << "] ask=" << alwaysAsk << endl;
    haveConnection = false;
    connectedDevice = "";

    bool gallery_mode = (useDevice=="gallery");

    QCString selDevice;
    /* in case useDevice is the term 'gallery', the user does not want to
     * connect to a scanner, but only work in gallery mode. Otherwise, try
     * to read the device to use from config or from a user dialog */
    if (!gallery_mode)
    {
        selDevice =  useDevice;
        if (selDevice.isEmpty()) selDevice = userDeviceSelection(alwaysAsk);

        if (selDevice.isEmpty())			// dialogue cancelled
        {
            if (scan_params!=NULL) return (false);	// have setup, do nothing
            gallery_mode = true;
        }
    }

    // Allow reopening
    //if( connectedDevice == selDevice ) {
    //  kdDebug( 28000) << "Device " << selDevice << " is already selected!" << endl;
    //return( true );
    //}

    if (scan_params!=NULL) closeScanDevice();		// remove existing GUI object
    scan_params = new ScanParams(m_dockScanParam);	// and create a new one
    Q_CHECK_PTR(scan_params);

    if (!selDevice.isEmpty())				// connect to the selected scanner
    {
        while (!haveConnection)
        {
            kdDebug(28000) << "Opening device " << selDevice << endl;
            if (sane->openDevice(selDevice)==KSCAN_OK) haveConnection = true;
            else
            {
                QString msg = i18n("<qt><p>\
There was a problem opening the scanner device. \
Check that the scanner is connected and switched on, and that SANE support for it \
is correctly configured.\
<p>\
Trying to use scanner device: <b>%2</b>\
<br>\
The error reported was: <b>%1</b>").arg(sane->lastErrorMessage()).arg(selDevice);

                int tryAgain = KMessageBox::warningContinueCancel(m_parent,msg,QString::null,
                                                       KGuiItem("Retry"));
                if (tryAgain==KMessageBox::Cancel) break;
            }
        }

        if (haveConnection)				// scanner connected OK
        {
            QSize s = sane->getMaxScanSize();		// fix for 160148
            kdDebug(28000) << k_funcinfo << "scanner max size = " << s << endl;
            preview_canvas->setScannerBedSize(s.width(),s.height());

            // Connections ScanParams --> Previewer
            connect(scan_params,SIGNAL(scanResolutionChanged(int,int)),
                    preview_canvas,SLOT(slNewScanResolutions(int,int)));
            connect(scan_params,SIGNAL(scanModeChanged(int)),
                    preview_canvas,SLOT(slNewScanMode(int)));
            connect(scan_params,SIGNAL(newCustomScanSize(QRect)),
                    preview_canvas,SLOT(slotNewCustomScanSize(QRect)));

            // Connections Previewer --> ScanParams
            connect(preview_canvas,SIGNAL(newPreviewRect(QRect)),
                    scan_params,SLOT(slotNewPreviewRect(QRect)));

            scan_params->connectDevice(sane);
            connectedDevice = selDevice;

            preview_canvas->setPreviewImage(sane->loadPreviewImage());
							// load saved preview image
            preview_canvas->connectScanner(sane);	// load its autosel options
        }
    }

    m_dockScanParam->setWidget(scan_params);
    m_dockScanParam->show();				// show the widget again

    if (!haveConnection)				// no scanner device available,
    {							// or starting in gallery mode
        if (scan_params!=NULL) scan_params->connectDevice(NULL,gallery_mode);
    }

    emit signalScannerChanged(haveConnection);
    return (haveConnection);
}




void KookaView::slAddDevice()
{
    kdDebug(28000) << k_funcinfo << endl;

    AddDeviceDialog d(m_parent,i18n("Add Scan Device"));
    if (d.exec())
    {
	QString dev = d.getDevice();
	QString dsc = d.getDescription();
	kdDebug(28000) << k_funcinfo << "dev=[" << dev
		       << "] desc=[" << dsc << "]" << endl;

	sane->addUserSpecifiedDevice(dev,dsc);
    }
}



QCString KookaView::userDeviceSelection(bool alwaysAsk)
{
   /* Human readable scanner descriptions */
   QStringList hrbackends;

   /* a list of backends the scan backend knows */
   QStrList backends = sane->getDevices();

   if (backends.count()==0)
   {
       if (KMessageBox::warningContinueCancel(m_parent,i18n("<qt>\
<p>No scanner devices are available.\
<p>If your scanner is a type that can be auto-detected by SANE, check that it \
is connected, switched on and configured correctly.\
<p>If the scanner cannot be auto-detected by SANE (this includes some network \
scanners), you need to specify the device to use.  Use the \"Add Scan Device\" \
option to enter the backend name and parameters, or see that dialogue for more \
information."),QString::null,KGuiItem(i18n("Add Scan Device...")))!=KMessageBox::Continue) return ("");

       slAddDevice();
       backends = sane->getDevices();			// refresh the list
       if (backends.count()==0) return ("");		// give up this time
   }

   QCString selDevice;
   QStrListIterator  it( backends );
   while( it )
   {
       kdDebug( 28000 ) << k_funcinfo << "Found backend [" << it.current() << "] = " << sane->getScannerName(it.current()) << endl;
       hrbackends.append( sane->getScannerName( it.current() ));
       ++it;
   }

   /* allow the user to select one */
   DeviceSelector ds(m_parent, backends, hrbackends );
   if (!alwaysAsk) selDevice = ds.getDeviceFromConfig( );

   if( selDevice.isEmpty() || selDevice.isNull() )
   {
       kdDebug(28000) << "selDevice not found - starting selector!" << selDevice << endl;
       if ( ds.exec() == QDialog::Accepted )
       {
	   selDevice = ds.getSelectedDevice();
       }
   }
   return( selDevice );
}





QString KookaView::scannerName() const
{
    if (connectedDevice=="") return (i18n("Gallery"));
    if (!haveConnection) return (i18n("No scanner connected"));
    return (sane->getScannerName(connectedDevice));
}



void KookaView::slotSelectionChanged(QRect newSelection)
{
    emit signalRectangleChanged(newSelection.isValid());
}



void KookaView::slotGallerySelectionChanged()
{
    KFileTreeViewItem *fti = m_gallery->currentItem();

    if (fti==NULL)
    {
        emit signalChangeStatusbar(i18n("No selection"));
	emit signalGallerySelectionChanged(false,0);
    }
    else
    {
        emit signalChangeStatusbar(i18n("Gallery %1 %2")
                                   .arg(fti->isDir() ? i18n("folder") : i18n("image"))
                                   .arg(fti->url().pathOrURL()));
	emit signalGallerySelectionChanged(fti->isDir(),gallery()->selectedItems().count());
//        if (fti->isDir() && m_thumbview!=NULL) m_thumbview->slotSelectImage(fti->url());
    }
}


void KookaView::slotLoadedImageChanged(const KookaImage *img,bool isDir)
{
    if (!isDir && img==NULL) emit signalChangeStatusbar(i18n("Image unloaded"));
    emit signalLoadedImageChanged(img!=NULL,isDir);
}


bool KookaView::galleryRootSelected() const
{
    KFileTreeViewItem *tvi = m_gallery->currentItem();
    if (tvi==NULL) return (true);
    return (tvi==gallery()->branches().first()->root());
}


void KookaView::loadStartupImage()
{
   KConfig *konf = KGlobal::config();
   konf->setGroup(GROUP_STARTUP);
   bool wantReadOnStart = konf->readBoolEntry(STARTUP_READ_IMAGE,true);

   if (wantReadOnStart)
   {
       QString startup = konf->readPathEntry(STARTUP_IMG_SELECTION);
       kdDebug(28000) << k_funcinfo << "load startup image [" << startup << "]" << endl;
       if (!startup.isEmpty()) gallery()->slSelectImage(startup);
   }
   else kdDebug(28000) << k_funcinfo << "Do not load startup image" << endl;
}


void KookaView::print()
{
    /* For now, print a single file. Later, print multiple images to one page */

    KookaImage *img = gallery()->getCurrImage(true);	// load image if necessary
    if (img==NULL) return;

    KPrinter printer; // ( true, pMode );
    printer.setUsePrinterResolution(true);
    printer.addDialogPage( new ImgPrintDialog( img ));

    if( printer.setup( m_mainWindow, i18n("Print %1").arg(img->localFileName().section('/', -1)) ))
    {
	KookaPrint kookaprint( &printer );
	kookaprint.printImage(img, 10);
    }
}


void KookaView::slNewPreview( QImage *new_img,ImgScanInfo *info)
{
   if (!new_img) return;

   kdDebug(28000) << k_funcinfo << "new preview image, size=" << new_img->size()
		  << " res=[" << info->getXResolution() << "x" << info->getYResolution() << "]" << endl;
							// flip preview to front
   if (!new_img->isNull()) m_dockPreview->makeDockVisible();
   preview_canvas->newImage(new_img);			// set new image and size
}


void KookaView::doOCRonSelection()
{
   emit signalChangeStatusbar(i18n("Starting OCR on selection"));

   KookaImage img;
   if (img_canvas->selectedImage(&img)) startOCR(&img);

   emit signalCleanStatusbar();
}


void KookaView::doOCR()
{
   emit signalChangeStatusbar(i18n("Starting OCR on the image"));

   KookaImage *img = gallery()->getCurrImage(true);
   startOCR(img);

   emit signalCleanStatusbar();
}


void KookaView::slOcrSpellCheck()
{
    kdDebug() << k_funcinfo << endl;

    emit signalChangeStatusbar(i18n("OCR Spell Check"));

    if (ocrFabric==NULL || !ocrFabric->engineValid())
    {
        KMessageBox::sorry(m_parent,
                           i18n("OCR has not been performed yet, or the engine has been changed"),
                           i18n("OCR Spell Check not possible"));
        return;
    }

    ocrFabric->performSpellCheck();
    emit signalCleanStatusbar();
}




void KookaView::startOCR(KookaImage *img)
{
    if (img==NULL || img->isNull()) return;

    if (ocrFabric!=NULL && !ocrFabric->engineValid())	// exists, but needs to be changed?
    {
        delete ocrFabric;
        ocrFabric = NULL;
    }

    if (ocrFabric==NULL)
    {
        //ocrFabric = new OcrEngine( m_mainDock, KGlobal::config() );
        bool gotoPrefs = false;
        ocrFabric = OcrEngine::createEngine(m_mainDock,&gotoPrefs);
        if (ocrFabric==NULL)
        {
            kdDebug(28000) << k_funcinfo << "Cannot create OCR engine!" << endl;
            if (gotoPrefs) emit signalOcrPrefs();
            return;
        }

        ocrFabric->setImageCanvas( img_canvas );

        // Connections OcrEngine --> myself
        connect( ocrFabric, SIGNAL( newOCRResultText( const QString& )),
                 SLOT(slotOcrResultText( const QString& )));
        connect( ocrFabric, SIGNAL( newOCRResultText( const QString& )),
                 m_dockOCRText, SLOT( show() ));
	  
        // Connections OcrEngine --> ImageCanvas
        connect( ocrFabric, SIGNAL( repaintOCRResImage( )),
                 img_canvas, SLOT(repaint()));

        // Connections OcrEngine --> OCR Results
        connect( ocrFabric, SIGNAL( clearOCRResultText()),
                 m_ocrResEdit, SLOT(clear()));

        connect( ocrFabric,    SIGNAL( updateWord(int, const QString&, const QString& )),
                 m_ocrResEdit, SLOT( slUpdateOCRResult( int, const QString&, const QString& )));

        connect( ocrFabric,    SIGNAL( ignoreWord(int, const ocrWord&)),
                 m_ocrResEdit, SLOT( slIgnoreWrongWord( int, const ocrWord& )));

        connect( ocrFabric, SIGNAL( markWordWrong(int, const ocrWord& )),
                 m_ocrResEdit, SLOT( slMarkWordWrong( int, const ocrWord& )));

        connect( ocrFabric,    SIGNAL( readOnlyEditor( bool )),
                 m_ocrResEdit, SLOT( setReadOnly( bool )));

        connect( ocrFabric,    SIGNAL( selectWord( int, const ocrWord& )),
                 m_ocrResEdit, SLOT( slSelectWord( int, const ocrWord& )));
    }

    ocrFabric->setImage(img);
    ocrFabric->startOCRVisible(m_mainDock);
}


void KookaView::slotOcrResultText(const QString &text)
{
    m_ocrResEdit->setText(text);
    emit signalOcrResultAvailable(!text.isEmpty());
}



void KookaView::slOCRResultImage( const QPixmap& pix )
{
    kdDebug(28000) << "Showing OCR Result Image" << endl;
    if( ! img_canvas ) return;

    if( m_ocrResultImg )
    {
        img_canvas->newImage(0L);
        delete m_ocrResultImg;
    }

    m_ocrResultImg = new QImage();
    *m_ocrResultImg = pix;
    img_canvas->newImage( m_ocrResultImg );
    img_canvas->setReadOnly(true); // ocr result images should be read only.
}

void KookaView::slScanStart( )
{
   kdDebug(28000) << "Scan starts " << endl;
   if( scan_params )
   {
      scan_params->setEnabled( false );
      KLed *led = scan_params->operationLED();
      if( led )
      {
	 led->setColor( Qt::red );
	 led->setState( KLed::On );
      }
   }
}

void KookaView::slAcquireStart( )
{
   kdDebug(28000) << "Acquire starts " << endl;
   if( scan_params )
   {
      KLed *led = scan_params->operationLED();
      if( led )
      {
	 led->setColor( Qt::green );
      }
   }
}

void KookaView::slNewImageScanned( QImage* img, ImgScanInfo* si )
{
    if ( isPhotoCopyMode ) return;
    KookaImageMeta *meta = new KookaImageMeta;
    meta->setScanResolution(si->getXResolution(), si->getYResolution());
    gallery()->addImage(img, meta);
}



void KookaView::slScanFinished( KScanStat stat )
{
   kdDebug(28000) << "Scan finished with status " << stat << endl;

   if (stat!=KSCAN_OK && stat!=KSCAN_CANCELLED)
   {
       QString msg = i18n("<qt><p>"
                          "There was a problem during preview or scanning."
                          "<br>"
                          "Check that the scanner is still connected and switched on, "
                          "and that media is loaded if required."
                          "<p>"
                          "Trying to use scanner device: <b>%3</b><br>"
                          "SANE reported error: <b>%1</b><br>"
                          "libkscan reported error: <b>%2</b>").arg(sane->lastErrorMessage())
                                                               .arg(KScanOption::errorMessage(stat))
                                                               .arg(connectedDevice);

       KMessageBox::error(m_parent,msg);
   }

   if( scan_params )
   {
      scan_params->setEnabled( true );
      KLed *led = scan_params->operationLED();
      if( led )
      {
	 led->setColor( Qt::green );
	 led->setState( KLed::Off );
      }
   }
}


void KookaView::closeScanDevice( )
{
   kdDebug(28000) << "Scanner Device closes down !" << endl;
   if( scan_params ) {
      delete scan_params;
      scan_params = NULL;
      m_dockScanParam->setWidget(NULL);
      m_dockScanParam->hide();
   }

   sane->slCloseDevice();
}

void KookaView::slCreateNewImgFromSelection()
{
   if( img_canvas->rootImage() )
   {
      emit( signalChangeStatusbar( i18n("Create new image from selection" )));
      QImage img;
      if( img_canvas->selectedImage( &img ) )
      {
	 gallery()->addImage( &img );
      }
      emit( signalCleanStatusbar( ));
   }

}


void KookaView::slRotateImage(int angle)
{
   KookaImage *img = gallery()->getCurrImage();
   bool doUpdate = true;

   if( img )
   {
      QImage resImg;

      QApplication::setOverrideCursor(waitCursor);
      switch( angle )
      {
	 case 90:
	    emit( signalChangeStatusbar( i18n("Rotate image 90 degrees" )));
	    resImg = rotateRight( img );
	    break;
//	 case 180:
//	    emit( signalChangeStatusbar( i18n("Rotate image 180 degrees" )));
//	    resImg = rotate180( img );
//	    break;
	 case 270:
	 case -90:
	    emit( signalChangeStatusbar( i18n("Rotate image -90 degrees" )));
	    resImg = rotateLeft( img );
	    break;

	 default:
	    kdDebug(28000) << k_funcinfo << "Angle " << angle << " not supported!" << endl;
	    doUpdate = false;
	    break;
      }
      QApplication::restoreOverrideCursor();

      /* updateCurrImage does the status-bar cleanup */
      if( doUpdate )
	 updateCurrImage( resImg );
      else
	 emit(signalCleanStatusbar());
   }

}



void KookaView::slMirrorImage( MirrorType m )
{
   const QImage *img = img_canvas->rootImage();
   bool doUpdate = true;

   if( img )
   {
      QImage resImg;

      QApplication::setOverrideCursor(waitCursor);
      switch( m )
      {
	 case MirrorVertical:
	    emit( signalChangeStatusbar( i18n("Mirroring image vertically" )));
	    resImg = img->mirror();
	    break;
	 case MirrorHorizontal:
	    emit( signalChangeStatusbar( i18n("Mirroring image horizontally" )));
	    resImg = img->mirror( true, false );
	    break;
	 case MirrorBoth:
	    emit( signalChangeStatusbar( i18n("Mirroring image in both directions" )));
	    resImg = img->mirror( true, true );
	    break;
	 default:
	    kdDebug(28000) << k_funcinfo << "Mirror type " << m << " not supported!" << endl;
	    doUpdate = false;
      }
      QApplication::restoreOverrideCursor();

      /* updateCurrImage does the status-bar cleanup */
      if( doUpdate )
	 updateCurrImage( resImg );
      else
	 emit(signalCleanStatusbar());

      // img_canvas->newImage(  );
   }
}


void KookaView::slSaveOCRResult()
{
    if( ! m_ocrResEdit ) return;
    m_ocrResEdit->slSaveText();

}


void KookaView::slScanParams()
{
    if (sane==NULL) return;				// must have a scanner device
    ScanParamsDialog d(m_mainDock,sane);
    d.exec();
}


void KookaView::slShowAImage(const KookaImage *img)
{
    if (img_canvas)
    {
        img_canvas->newImage(img);
        img_canvas->setReadOnly(false);
    }

    if (ocrFabric) ocrFabric->setImage(img);		/* tell ocr about it */

    KStatusBar *statBar = m_mainWindow->statusBar();	/* Status Bar */
    if (img_canvas) statBar->changeItem(img_canvas->imageInfoString(),StatusImage);

    if (img) emit signalChangeStatusbar(i18n("Showing image %1").arg(img->url().pathOrURL()));
}


void KookaView::slUnloadAImage( KookaImage * )
{
   kdDebug(28000) << "Unloading Image" << endl;
   if( img_canvas )
   {
      img_canvas->newImage( 0L );
   }
}



/* this slot is called when the user clicks on an image in the packager
 * and loading of the image starts
 */
void KookaView::slStartLoading( const KURL& url )
{
   emit( signalChangeStatusbar( i18n("Loading %1" ).arg( url.prettyURL() ) ));

   // if( m_stack->visibleWidget() != img_canvas )
   // {
   //    m_stack->raiseWidget( img_canvas );
   // }

}


void KookaView::updateCurrImage( QImage& img )
{
    if( ! img_canvas->readOnly() )
    {
	emit( signalChangeStatusbar( i18n("Storing image changes" )));
	gallery()->slotCurrentImageChanged( &img );
	emit( signalCleanStatusbar());
    }
    else
    {
	emit( signalChangeStatusbar( i18n("Can not save image, it is write protected!")));
	kdDebug(28000) << "Image is write protected, no saving!" << endl;
    }
}


void KookaView::saveProperties(KConfig *config)
{
    kdDebug(28000) << k_funcinfo << endl;

    config->setGroup(GROUP_STARTUP);
    config->writePathEntry(STARTUP_IMG_SELECTION,gallery()->getCurrImageFileName(true));
}


void KookaView::slOpenCurrInGraphApp()
{
    KFileTreeViewItem *ftvi = m_gallery->currentItem();
    if (ftvi==NULL) return;

    KURL::List urllist(ftvi->url());
    KRun::displayOpenWithDialog(urllist);
}


QImage KookaView::rotateLeft( QImage *m_img )
{
   QImage rot;
   
   if( m_img )
   {
       QWMatrix m;

       m.rotate(-90);
       rot = m_img->xForm(m);
   }
   return( rot );
}

QImage KookaView::rotateRight( QImage *m_img )
{
   QImage rot;
   
   if( m_img )
   {
       QWMatrix m;

       m.rotate(+90);
       rot = m_img->xForm(m);
   }
   return( rot );
}

//QImage KookaView::rotate180( QImage *m_img )
//{
//   QImage rot;
//   
//   if( m_img )
//   {
//       QWMatrix m;
//
//       m.rotate(+180);
//       rot = m_img->xForm(m);
//   }
//   return( rot );
//}



void KookaView::connectViewerAction(KAction *action)
{
    if (action==NULL) return;
    QPopupMenu *popup = img_canvas->contextMenu();
    if (popup!=NULL) action->plug(popup);
}


void KookaView::connectGalleryAction(KAction *action)
{
    if (action==NULL) return;
    QPopupMenu *popup = gallery()->contextMenu();
    if (popup!=NULL) action->plug(popup);
}


void KookaView::connectThumbnailAction(KAction *action)
{
    if (action==NULL) return;
    QPopupMenu *popup = m_thumbview->contextMenu();
    if (popup!=NULL) action->plug(popup);
}


void KookaView::slotApplySettings()
{
   if (m_thumbview!=NULL) m_thumbview->readSettings();	// size and background
   if (m_gallery!=NULL) m_gallery->readSettings();	// layout and rename
}


void KookaView::createDockMenu( KActionCollection *col, KDockMainWindow *mainWin, const char * name )
{
   KActionMenu *actionMenu = new KActionMenu( i18n("Tool Views"), "view_icon", col, name );

   actionMenu->insert( new dwMenuAction( i18n("Show Image Viewer"),
					 KShortcut(), m_mainDock, col,
					 mainWin, "dock_viewer" ));

   actionMenu->insert( new dwMenuAction( i18n("Show Preview"),
					 KShortcut(), m_dockPreview, col,
					 mainWin, "dock_preview" ));

   actionMenu->insert( new dwMenuAction( i18n("Show Gallery"),
					 KShortcut(), m_dockPackager, col,
					 mainWin, "dock_gallery" ));

   actionMenu->insert( new dwMenuAction( i18n("Show Thumbnail Window"),
					 KShortcut(), m_dockThumbs, col,
					 mainWin, "dock_thumbs" ));

   actionMenu->insert( new dwMenuAction( i18n("Show Scan Parameters"),
					 KShortcut(), m_dockScanParam, col,
					 mainWin, "dock_scanparams" ));

   actionMenu->insert( new dwMenuAction( i18n("Show OCR Results"),
					 KShortcut(), m_dockOCRText, col,
					 mainWin, "dock_ocrResults" ));
}


/* Slot called to start copying to printer */
void KookaView::slStartPhotoCopy( )
{
    kdDebug(28000) << "Entered slot KookaView::slStartPhotoCopy" << endl;

    if ( scan_params == 0 ) return;
    isPhotoCopyMode=true;
    photoCopyPrinter = new KPrinter( true, QPrinter::HighResolution ); 
//    photoCopyPrinter->removeStandardPage( KPrinter::CopiesPage );
    photoCopyPrinter->setUsePrinterResolution(true);
    photoCopyPrinter->setOption( OPT_SCALING, "scan");
    photoCopyPrinter->setFullPage(true);
    slPhotoCopyScan( KSCAN_OK );
}

void KookaView::slPhotoCopyPrint( QImage* img, ImgScanInfo* si )
{
    kdDebug(28000) << "Entered slot KookaView::slPhotoCopyPrint" << endl;

    if ( ! isPhotoCopyMode ) return;
    KookaImage kooka_img=KookaImage( *img );
    KookaPrint kookaprint( photoCopyPrinter );
    kookaprint.printImage( &kooka_img ,0 );

}

void KookaView::slPhotoCopyScan( KScanStat status )
{
    kdDebug(28000) << "Entered slot KookaView::slPhotoCopyScan" << endl;
    if ( ! isPhotoCopyMode ) return;

//    photoCopyPrinter->addDialogPage( new PhotoCopyPrintDialogPage( sane ) );

    KScanOption res ( SANE_NAME_SCAN_RESOLUTION );
    photoCopyPrinter->setOption( OPT_SCAN_RES, res.get() );
    photoCopyPrinter->setMargins(QSize(0,0));
    kdDebug(28000) << "Resolution " << res.get() << endl;

//    photoCopyPrinter->addDialogPage( new ImgPrintDialog( 0 ) );
    if( photoCopyPrinter->setup( 0, "Photocopy" )) {
        Q_CHECK_PTR( sane );
        scan_params->slStartScan( );
    }
    else {
        isPhotoCopyMode=false;
        delete photoCopyPrinter;
    }

}


