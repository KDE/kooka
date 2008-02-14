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

#include "kookaview.h"

#include "resource.h"
#include "kscandevice.h"
#include "imgscaninfo.h"
#include "devselector.h"
#include "ksaneocr.h"
#include "imgsaver.h"
#include "kookapref.h"
#include "imgnamecombo.h"
#include "thumbview.h"
#include "dwmenuaction.h"
#include "kookaimage.h"
#include "kookaimagemeta.h"
#include "ocrresedit.h"
#include "kookaprint.h"
#include "imgprintdialog.h"
#if 0
#include "paramsetdialogs.h"
#endif
#include "adddevice.h"

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


#define STARTUP_IMG_SELECTION   "SelectedImageOnStartup"


KookaView::KookaView( KParts::DockMainWindow *parent, const QCString& deviceToUse)
   : QObject(),
     m_ocrResultImg(0),
     ocrFabric(0),
     m_mainDock(0),
     m_dockScanParam(0),
     m_dockThumbs(0),
     m_dockPackager(0),
     m_dockRecent(0),
     m_dockPreview(0),
     m_dockOCRText(0),
     m_mainWindow(parent),
     m_ocrResEdit(0)
{
   KIconLoader *loader = KGlobal::iconLoader();
   scan_params = NULL;
   preview_canvas = NULL;
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
   connect( img_canvas, SIGNAL( imageReadOnly(bool)),
	    this, SLOT(slViewerReadOnly(bool)));
   connect( img_canvas, SIGNAL( newRect()),
	    this, SLOT(slSelectionChanged()));
   connect( img_canvas, SIGNAL( noRect()),
	    this, SLOT(slSelectionChanged()));
   
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
   packager = new ScanPackager( m_dockPackager );
   m_dockPackager->setWidget( packager );
   m_dockPackager->manualDock( m_mainDock,              // dock target
                         KDockWidget::DockLeft, // dock site
                         30 );                  // relation target/this (in percent)


   connect( packager, SIGNAL(showThumbnails( KFileTreeViewItem* )),
	    this, SLOT( slShowThumbnails( KFileTreeViewItem* )));
   connect( m_thumbview, SIGNAL( selectFromThumbnail( const KURL& )),
	    packager, SLOT( slSelectImage(const KURL&)));
   connect(packager,SIGNAL(selectionChanged()),
	   this,SLOT(slotGallerySelectionChanged()));
   connect(packager,SIGNAL(showImage(KookaImage *,bool)),
	   this,SLOT(slotLoadedImageChanged(KookaImage *,bool)));


   /*
    * Create a Kombobox that holds the last folders visible even on the preview page
    */
   m_dockRecent  = parent->createDockWidget( "Recent",
					     loader->loadIcon( "image", KIcon::Small ),
					     0L, i18n("Gallery Folders"));

   m_dockRecent->setDockSite(KDockWidget::DockFullSite);

   QHBox *recentBox = new QHBox( m_dockRecent );
   recentBox->setMargin(KDialog::marginHint());
   QLabel *lab = new QLabel( i18n("Gallery:"), recentBox );
   lab->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
   recentFolder = new ImageNameCombo( recentBox );

   m_dockRecent->setWidget( recentBox );
   m_dockRecent->manualDock( m_dockPackager,              // dock target
                         KDockWidget::DockBottom, // dock site
                         5 );                  // relation target/this (in percent)



   connect( packager,  SIGNAL( galleryPathSelected( KFileTreeBranch*, const QString&)),
	    recentFolder, SLOT( slotGalleryPathChanged( KFileTreeBranch*, const QString& )));

   connect( packager,  SIGNAL( directoryToRemove( KFileTreeBranch*, const QString&)),
	    recentFolder, SLOT(   slotPathRemove( KFileTreeBranch*, const QString& )));

   connect( recentFolder, SIGNAL(activated( const QString& )),
	    packager, SLOT(slotSelectDirectory( const QString& )));

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

   m_dockScanParam->manualDock( m_dockRecent,              // dock target
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

   /* New image created after scanning */
   connect(sane, SIGNAL(sigNewImage(QImage*,ImgScanInfo*)), this, SLOT(slNewImageScanned(QImage*,ImgScanInfo*)));
   /* New preview image */
   connect(sane, SIGNAL(sigNewPreview(QImage*,ImgScanInfo *)), this, SLOT( slNewPreview(QImage*,ImgScanInfo *)));

   connect( sane, SIGNAL( sigScanStart() ), this, SLOT( slScanStart()));
   connect( sane, SIGNAL( sigScanFinished(KScanStat)), this, SLOT(slScanFinished(KScanStat)));
   connect( sane, SIGNAL( sigAcquireStart()), this, SLOT( slAcquireStart()));
   /* Image canvas should show a new document */
   connect( packager, SIGNAL( showImage( KookaImage*,bool )),
            this,       SLOT( slShowAImage( KookaImage*)));

   connect( packager, SIGNAL( aboutToShowImage(const KURL&)),
	    this,       SLOT( slStartLoading( const KURL& )));

   /* Packager unloads the image */
   connect( packager, SIGNAL( unloadImage( KookaImage* )),
            this,       SLOT( slUnloadAImage( KookaImage*)));

   /* a image changed mostly through a image manipulation method like rotate */
   connect( packager,  SIGNAL( fileChanged( KFileItem* )),
	    m_thumbview, SLOT( slImageChanged( KFileItem* )));

   connect( packager, SIGNAL( fileRenamed( KFileItem*, const KURL& )),
            m_thumbview, SLOT( slImageRenamed( KFileItem*, const KURL& )));

   connect( packager,  SIGNAL( fileDeleted( KFileItem* )),
	    m_thumbview, SLOT( slImageDeleted( KFileItem* )));


   packager->openRoots();

   /* Status Bar */
   KStatusBar *statBar = m_mainWindow->statusBar();

   // statBar->insertItem(QString("1"), SBAR_ZOOM,  0, true );
   statBar->insertItem( QString("-"), StatusImage,  0, true );

   /* Set a large enough size */
   int w = statBar->fontMetrics().
           width(img_canvas->imageInfoString(2000, 2000, 48));
   kdDebug(28000) << "Fixed size for status bar: " << w << " from string " << img_canvas->imageInfoString(2000, 2000, 48) << endl;
   statBar->setItemFixed( StatusImage, w );

}


KookaView::~KookaView()
{
   saveProperties( KGlobal::config () );
   delete preview_canvas;

   kdDebug(28000)<< "Finished saving config data" << endl;
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
        if (selDevice.isEmpty()) return (false);	// dialogue cancelled
    }

    // Allow reopening
    //if( connectedDevice == selDevice ) {
    //  kdDebug( 28000) << "Device " << selDevice << " is already selected!" << endl;
    //return( true );
    //}

    if (scan_params!=NULL) slCloseScanDevice();		// remove existing GUI object
    scan_params = new ScanParams(m_dockScanParam);	// and create a new one
    Q_CHECK_PTR(scan_params);

    if (!selDevice.isEmpty())				// connect to the selected scanner
    {
        while (!haveConnection)
        {
            kdDebug(28000) << "Opening device " << selDevice << endl;
            if (sane->openDevice(selDevice)!=KSCAN_OK)
            {
                QString msg = i18n("<qt><p>\
There was a problem opening the scanner device. \
Check that the scanner is connected and switched on, and that SANE support for it \
is correctly configured.\
<p>\
Trying to use scanner device: <b>%2</b>\
<br>\
The error reported was: <b>%1</b>").arg(sane->lastErrorMessage()).arg(selDevice);

                if (KMessageBox::warningContinueCancel(m_parent,msg,QString::null,
                                                       KGuiItem("Retry"))==KMessageBox::Cancel)
                {
                    break;
                }
            }
            else
            {
                connect( scan_params,    SIGNAL( scanResolutionChanged( int, int )),
                         preview_canvas, SLOT( slNewScanResolutions( int, int )));

                if (!scan_params->connectDevice(sane))
                {
                    // This will never happen, connectDevice always returns TRUE
                    kdDebug(28000) << "Connecting to the scanner failed :( ->TODO" << endl;
                }
                else
                {
                    haveConnection = true;
                    connectedDevice = selDevice;

                    /* New Rectangle selection in the preview, now scanimge exists */
                    ImageCanvas *previewCanvas = preview_canvas->getImageCanvas();
                    connect( previewCanvas , SIGNAL( newRect(QRect)),
                             scan_params, SLOT(slCustomScanSize(QRect)));
                    connect( previewCanvas, SIGNAL( noRect()),
                             scan_params, SLOT(slMaximalScanSize()));
                    // connect( scan_params,    SIGNAL( scanResolutionChanged( int, int )),
                    // 		     preview_canvas, SLOT( slNewScanResolutions( int, int )));

                    if (preview_canvas!=NULL)		/* load the preview image */
                    {
                        preview_canvas->setPreviewImage( sane->loadPreviewImage() );
                        /* Call this after the device is actually open */
                        preview_canvas->slConnectScanner( sane );
                    }
                }
            }
        }
    }

    m_dockScanParam->setWidget(scan_params);
    m_dockScanParam->show();				/* Show the widget again */

    if (!haveConnection)
    {
        // No devices available, or starting in gallery mode
        if (scan_params!=NULL) scan_params->connectDevice(NULL,gallery_mode);
    }

    emit signalScannerChanged(haveConnection);
    return (haveConnection);
}




void KookaView::slAddDevice()
{
    kdDebug(29000) << k_funcinfo << endl;

    AddDeviceDialog d(m_parent,i18n("Add Scan Device"));
    if (d.exec())
    {
	QString dev = d.getDevice();
	QString dsc = d.getDescription();
	kdDebug(29000) << k_funcinfo << "dev=[" << dev
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
<p>If your scanner is a type that should be auto-detected by SANE, check that it \
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
       kdDebug(29000) << "selDevice not found - starting selector!" << selDevice << endl;
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



void KookaView::slSelectionChanged()
{
    kdDebug( 28000) << k_funcinfo << endl;
    emit signalRectangleChanged(img_canvas->selectedImage(NULL));
}



void KookaView::slotGallerySelectionChanged()
{
    kdDebug( 28000) << k_funcinfo << endl;

    KFileTreeViewItem *fti = packager->currentKFileTreeViewItem();
    if (fti==NULL)
    {
	emit signalGallerySelectionChanged(false,0);
    }
    else
    {
	emit signalGallerySelectionChanged(fti->isDir(),packager->selectedItems().count());
    }
}


void KookaView::slotLoadedImageChanged(KookaImage *img,bool isDir)
{
    kdDebug( 28000) << k_funcinfo << "img=" << ((void*)img) << " isDir=" << isDir << endl;
    emit signalLoadedImageChanged(img!=NULL,isDir);
}


bool KookaView::galleryRootSelected() const
{
    if (packager==NULL) return (false);
    KFileTreeViewItem *tvi = packager->currentKFileTreeViewItem();
    if (tvi==NULL) return (true);
    return (tvi==packager->branches().first()->root());
}


void KookaView::loadStartupImage( void )
{
   kdDebug( 28000) << "Starting to load startup image" << endl;

   /* Now set the configured stuff */
   KConfig *konf = KGlobal::config ();
   if( konf )
   {
      konf->setGroup(GROUP_STARTUP);
      bool wantReadOnStart = konf->readBoolEntry( STARTUP_READ_IMAGE, true );

      if( wantReadOnStart )
      {
	 QString startup = konf->readPathEntry( STARTUP_IMG_SELECTION );

	 if( !startup.isEmpty() )
	 {
	    kdDebug(28000) << "Loading startup image !" << endl;
	    packager->slSelectImage( KURL(startup) );
	 }
      }
      else
      {
	 kdDebug(28000) << "Do not load startup image due to config value" << endl;
      }
   }
}


void KookaView::print()
{
    /* For now, print a single file. Later, print multiple images to one page */

    KookaImage *img = packager->getCurrImage();
    if ( !img )
        return;
    KPrinter printer; // ( true, pMode );
    printer.setUsePrinterResolution(true);
    printer.addDialogPage( new ImgPrintDialog( img ));

    if( printer.setup( m_mainWindow, i18n("Print %1").arg(img->localFileName().section('/', -1)) ))
    {
	KookaPrint kookaprint( &printer );
	kookaprint.printImage(img);
    }
}


void KookaView::slNewPreview( QImage *new_img,ImgScanInfo *info)
{
   if (!new_img) return;

   kdDebug(29000) << k_funcinfo << "new preview image, size=" << new_img->size()
		  << " res=[" << info->getXResolution() << "x" << info->getYResolution() << "]" << endl;
							// flip preview to front
   if (!new_img->isNull()) m_dockPreview->makeDockVisible();
   preview_canvas->newImage(new_img);			// set new image and size
}


bool KookaView::ToggleVisibility( int item )
{
   QWidget *w = 0;
   bool    ret = false;

   switch( item )
   {
      case ID_VIEW_SCANPARAMS:
	 w = scan_params;
	 break;
      case ID_VIEW_POOL:
	 w = preview_canvas;
	 break;
      default:
	 w = 0;
   }

   if( w )
   {
      if( w->isVisible() )
      {
	 w->hide();
	 ret = false;
      }
      else
      {
	 w->show();
	 ret = true;
      }
   }
   return ret;
}


void KookaView::doOCRonSelection( void )
{
   emit( signalChangeStatusbar( i18n("Starting OCR on selection" )));

   KookaImage img;

   if( img_canvas->selectedImage(&img) )
   {
      startOCR( &img );
   }
   emit( signalCleanStatusbar() );
}

/* Does OCR on the entire picture */
void KookaView::doOCR( void )
{
   emit( signalChangeStatusbar( i18n("Starting OCR on the entire image" )));
    KookaImage *img = packager->getCurrImage();
   startOCR( img );
   emit( signalCleanStatusbar( ));
}

void KookaView::startOCR( KookaImage *img )
{
   if( img && ! img->isNull() )
   {
      if( ocrFabric == 0L )
      {
          ocrFabric = new KSaneOcr( m_mainDock, KGlobal::config() );
          ocrFabric->setImageCanvas( img_canvas );

          connect( ocrFabric, SIGNAL( newOCRResultText( const QString& )),
                   m_ocrResEdit, SLOT(setText( const QString& )));

	  connect( ocrFabric, SIGNAL( newOCRResultText( const QString& )),
		   m_dockOCRText, SLOT( show() ));
	  
          connect( ocrFabric, SIGNAL( repaintOCRResImage( )),
                   img_canvas, SLOT(repaint()));

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

      Q_CHECK_PTR( ocrFabric );
      ocrFabric->slSetImage( img );

      if( !ocrFabric->startOCRVisible(m_mainDock) )
      {
	 KMessageBox::sorry(0, i18n("Could not start OCR-Process.\n"
				    "Probably there is already one running." ));

      }
   }
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
    KookaImageMeta *meta = new KookaImageMeta;
    meta->setScanResolution(si->getXResolution(), si->getYResolution());
    packager->slAddImage(img, meta);
}



void KookaView::slScanFinished( KScanStat stat )
{
   kdDebug(28000) << "Scan finished with status " << stat << endl;

   if (stat!=KSCAN_OK && stat!=KSCAN_CANCELLED)
   {
       QString msg = i18n("<qt><p>\
There was a problem during preview or scanning. \
<br>\
Check that the scanner is still connected and switched on, \
and that media is loaded if required.\
<p>\
Trying to use scanner device: <b>%2</b>\
<br>\
The error reported was: <b>%1</b>").arg(sane->lastErrorMessage()).arg(connectedDevice);

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


void KookaView::slCloseScanDevice( )
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
	 packager->slAddImage( &img );
      }
      emit( signalCleanStatusbar( ));
   }

}


void KookaView::slRotateImage(int angle)
{
   KookaImage *img = packager->getCurrImage();
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


void KookaView::slLoadScanParams( )
{
   if( ! sane ) return;
#if 0
   /* not yet cooked */
   LoadSetDialog loadDialog( m_mainDock, sane->shortScannerName(), sane );
   if( loadDialog.exec())
   {
      kdDebug(28000)<< "Executed successfully" << endl;
   }
#endif
}

void KookaView::slSaveScanParams( )
{
   if( !sane ) return;

   /* not yet cooked */
#if 0
   KScanOptSet optSet( "SaveSet" );

   sane->getCurrentOptions( &optSet );
   SaveSetDialog dialog( m_mainDock /* this */ , &optSet );
   if( dialog.exec())
   {
      kdDebug(28000)<< "Executed successfully" << endl;
      QString name = dialog.paramSetName();
      QString desc = dialog.paramSetDescription();
      sane->slSaveScanConfigSet( name, desc );
   }
#endif
}

void KookaView::slShowAImage( KookaImage *img )
{
   kdDebug(28000) << "Show new Image" << endl;
   if( img_canvas )
   {
      img_canvas->newImage( img );
      img_canvas->setReadOnly(false);
   }

   /* tell ocr about */
   if( ocrFabric )
   {
       ocrFabric->slSetImage( img );
   }

   /* Status Bar */
   KStatusBar *statBar = m_mainWindow->statusBar();
   if( img_canvas )
       statBar->changeItem( img_canvas->imageInfoString(), StatusImage );
}

void KookaView::slUnloadAImage( KookaImage * )
{
   kdDebug(28000) << "Unloading Image" << endl;
   if( img_canvas )
   {
      img_canvas->newImage( 0L );
   }
}


void KookaView::slShowThumbnails(KFileTreeViewItem *dirKfi, bool forceRedraw )
{
   /* If no item is specified, use the current one */
   if( ! dirKfi )
   {
      /* do on the current visible dir */
      KFileTreeViewItem *kftvi = packager->currentKFileTreeViewItem();
      if ( !kftvi )
      {
          return;
      }
      
      if( kftvi->isDir())
      {
          dirKfi = kftvi;
      }
      else
      {
	 kftvi = static_cast<KFileTreeViewItem*>(static_cast<QListViewItem*>(kftvi)->parent());
	 dirKfi = kftvi;
	 forceRedraw = true;
	 packager->setSelected( static_cast<QListViewItem*>(dirKfi), true );
      }
   }

   kdDebug(28000) << "Showing thumbs for " << dirKfi->url().prettyURL() << endl;

   /* Only do the new thumbview if the old is on another dir */
   if( m_thumbview && (forceRedraw || m_thumbview->currentDir() != dirKfi->url()) )
   {
      m_thumbview->clear();
      /* Find a list of child KFileItems */
      if( forceRedraw ) m_thumbview->readSettings();

      KFileItemList fileItemsList;

      QListViewItem * myChild = dirKfi->firstChild();
      while( myChild )
      {
         fileItemsList.append( static_cast<KFileTreeViewItem*>(myChild)->fileItem());
         myChild = myChild->nextSibling();
      }

      m_thumbview->slNewFileItems( fileItemsList );
      m_thumbview->setCurrentDir( dirKfi->url());
      // m_thumbview->arrangeItemsInGrid();
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
	packager->slotCurrentImageChanged( &img );
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
   kdDebug(28000) << "Saving Properties for KookaView !" << endl;
   config->setGroup( GROUP_STARTUP );
   /* Get with path */
   config->writePathEntry( STARTUP_IMG_SELECTION, packager->getCurrImageFileName(true));

}


void KookaView::slOpenCurrInGraphApp( void )
{
   QString file;

   if( packager )
   {
      KFileTreeViewItem *ftvi = packager->currentKFileTreeViewItem();

      if( ! ftvi ) return;

      kdDebug(28000) << "Trying to open <" << ftvi->url().prettyURL()<< ">" << endl;
      KURL::List urllist;

      urllist.append( ftvi->url());

      KRun::displayOpenWithDialog( urllist );
   }
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



void KookaView::connectViewerAction( KAction *action )
{
   QPopupMenu *popup = img_canvas->contextMenu();
   //kdDebug(29000) << "This is the popup: " << popup << endl;
   if( popup && action )
   {
      action->plug( popup );
   }
}

void KookaView::connectGalleryAction( KAction *action )
{
   QPopupMenu *popup = packager->contextMenu();

   if( popup && action )
   {
      action->plug( popup );
   }
}

void KookaView::slFreshUpThumbView()
{
   if( m_thumbview )
   {
      /* readSettings returns true if something changes */
      if( m_thumbview->readSettings() )
      {
	 kdDebug(28000) << "Thumbview-Settings changed, readraw thumbs" << endl;
	 /* new settings */
	 slShowThumbnails(0, true);
      }
   }
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

   actionMenu->insert( new dwMenuAction( i18n("Show Recent Gallery Folders"),
					 KShortcut(), m_dockRecent, col,
					 mainWin, "dock_recent" ));
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


#include "kookaview.moc"
