/***************************************************************************
              kookaview.cpp  -  kookas visible stuff
                             -------------------                                         
    begin                : ?
    copyright            : (C) 1999 by Klaas Freitag                         
    email                : freitag@suse.de

    $Id$
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/

#include "kookaview.h"
#include "resource.h"
#include "kscandevice.h"
#include "devselector.h"
#include "ksaneocr.h"
#include "img_saver.h"
#include "kookapref.h"
#include "imgnamecombo.h"
#include "thumbview.h"

#include <qlabel.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qsplitter.h>
#include <qstrlist.h>
#include <qpaintdevice.h>
#include <qpaintdevicemetrics.h>
#include <qpopupmenu.h>
#include <qwidgetstack.h>

#include <kurl.h>
#include <krun.h>
#include <kapplication.h>
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

#define PACKAGER_TAB 0
#define PREVIEWER_TAB 1

#define STARTUP_IMG_SELECTION   "SelectedImageOnStartup"
#define STARTUP_SELECTED_PAGE   "TabWidgetPage"
#define STARTUP_SCANPARAM_SIZES "ScanParamDialogSizes"
#define STARTUP_VIEWER_SIZES    "ViewerSplitterSizes"


KookaView::KookaView(QWidget *parent, const QCString& deviceToUse)
    : QSplitter(parent)
{

   /* Another splitter for splitting the Packager Tree from the Scan Parameter */
   ocrFabric = 0L;
   
   paramSplitter = new QSplitter( QSplitter::Vertical, this );
   setOpaqueResize( false );
   paramSplitter->setOpaqueResize( false );

   /* An image canvas for the large image, right side */
   m_stack = new QWidgetStack( this );
   
   img_canvas  = new ImageCanvas( m_stack );
   m_stack->addWidget( img_canvas );
   m_stack->raiseWidget( img_canvas );
   img_canvas->setMinimumSize(100,200);
   img_canvas->enableContextMenu(true);

   /* thumbnail viewer widget */
   m_thumbview = new ThumbView( m_stack );
   m_stack->addWidget( m_thumbview );
   
   setResizeMode( m_stack /* img_canvas */,    QSplitter::Stretch );
   setResizeMode( paramSplitter, QSplitter::KeepSize);

   /* Create a vbox to take the tabwidget and the the combobox */
   QVBox *vbox = new QVBox( paramSplitter );
   /* The Tabwidget to contain the preview and the packager */
   tabw  = new QTabWidget( vbox, "TABWidget" );

   /* A new packager to contain the already scanned images */
   packager = new ScanPackager( tabw );
   {
      
   }
   connect( packager, SIGNAL(showThumbnails( const KURL& )),
	    this, SLOT( slShowThumbnails( const KURL& )));
   /* build up the Preview/Packager-Tabview */
   tabw->insertTab( packager, i18n( "&Gallery"), PACKAGER_TAB );

   tabw->setMinimumSize(100, 100);

   /*
    * Create a Kombobox that holds the last folders visible even on the preview page
    */
   QHBox *recentBox = new QHBox( vbox );
   recentBox->setMargin(KDialog::marginHint());
   QLabel *lab = new QLabel( i18n("Gallery"), recentBox );
   lab->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
   recentFolder = new ImageNameCombo( recentBox );

   
   connect( packager,  SIGNAL( galleryPathSelected( KFileTreeBranch*, const QString&)),
	    recentFolder, SLOT( slotGalleryPathChanged( KFileTreeBranch*, const QString& )));

   connect( packager,  SIGNAL( directoryToRemove( KFileTreeBranch*, const QString&)),
	    recentFolder, SLOT(   slotPathRemove( KFileTreeBranch*, const QString& )));

   connect( recentFolder, SIGNAL(activated( const QString& )),
	    packager, SLOT(slotSelectDirectory( const QString& )));
   
   /* the object from the kscan lib to handle low level scanning */
   sane = new KScanDevice( this );
   Q_CHECK_PTR(sane);
   // connect( sane, SIGNAL( sigCloseDevice()), this, SLOT( slCloseScanDevice()));

   /* select the scan device, either user or from config, this creates and assembles
    * the complete scanner options dialog
    * scan_params must be zero for that */
   scan_params = 0L;
   preview_canvas = 0L;
   
   preview_canvas = new Previewer( tabw );
   {
      preview_canvas->setMinimumSize( 100,100);
   	
      /* since the scan_params will be created in slSelectDevice, do the
       * connections later
       */
   }
   tabw->insertTab( preview_canvas, i18n("&Preview"), PREVIEWER_TAB );

   if( slSelectDevice(deviceToUse))
   {
      /* If a scanner exists */

      /* Load from config which tab page was selected last time */
      KConfig *konf = KGlobal::config ();
      konf->setGroup(GROUP_STARTUP);
      QString startup = konf->readEntry( STARTUP_SELECTED_PAGE, "nothing" );
      if( startup == "previewer" )
	 tabw->showPage(preview_canvas);
      else
	 tabw->showPage(packager);
   }
   else
   {
      /* If there is no scanner, disable the preview tab */
      tabw->setTabEnabled( preview_canvas, false );
      tabw->showPage( packager );
   }

   /* New image created after scanning */
   connect(sane, SIGNAL(sigNewImage(QImage*)), packager, SLOT(slAddImage(QImage*)));
   /* New preview image */
   connect(sane, SIGNAL(sigNewPreview(QImage*)), this, SLOT( slNewPreview(QImage*)));

   connect( sane, SIGNAL( sigScanStart() ), this, SLOT( slScanStart()));
   connect( sane, SIGNAL( sigScanFinished(KScanStat)), this, SLOT(slScanFinished(KScanStat)));
   connect( sane, SIGNAL( sigAcquireStart()), this, SLOT( slAcquireStart()));
   /* Image canvas should show a new document */
   connect( packager, SIGNAL( showImage( QImage* )),
            img_canvas, SLOT( newImage( QImage*)));

   connect( packager, SIGNAL( aboutToShowImage(const KURL&)),
	    this, SLOT( slStartLoading( const KURL& )));
   
   /* Packager unloads the image */
   connect( packager, SIGNAL( unloadImage( QImage* )),
            img_canvas, SLOT( deleteView( QImage*)));


}


KookaView::~KookaView()
{
   saveProperties( KGlobal::config () );
   kdDebug(28000)<< "Finished saving config data" << endl;
}


bool KookaView::slSelectDevice( const QCString& useDevice )
{

   kdDebug(28000) << "Kookaview: select a device!" << endl;
   bool haveConnection = false;

   QCString selDevice;
   /* in case useDevice is the term 'gallery', the user does not want to
    * connect to a scanner, but only work in gallery mode. Otherwise, try
    * to read the device to use from config or from a user dialog */
   if( useDevice != "gallery" )
   {
      selDevice =  useDevice;
      if( selDevice.isEmpty())
      {
	 selDevice = userDeviceSelection();
      }
   }
   
   if( !selDevice.isEmpty() )
   {
      kdDebug(28000) << "Opening device " << selDevice << endl;

      if( connectedDevice == selDevice ) {
	 kdDebug( 28000) << "Device " << selDevice << " is already selected!" << endl;
	 return( true );
      }
      
      if( scan_params )
      {
	 /* This deletes the existing scan_params-object */
	 slCloseScanDevice();
      }
      /* This connects to the selected scanner */
      scan_params = new ScanParams( paramSplitter );
      Q_CHECK_PTR(scan_params);
      paramSplitter->setResizeMode( scan_params, QSplitter::FollowSizeHint );

      if( sane->openDevice( selDevice ) == KSCAN_OK )
      {
         connect( scan_params,    SIGNAL( scanResolutionChanged( int, int )),
                  preview_canvas, SLOT( slNewScanResolutions( int, int )));

	 if( ! scan_params->connectDevice( sane ) )
	 {
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
	 }
      }
      else
      {
	 kdDebug(28000) << "Could not open device <" << selDevice << ">" << endl;
	 scan_params->connectDevice(0);
      }

      /* try to load the size from config */
   }
   else
   {
      // no devices available or starting in gallery mode
      if( scan_params )
	 scan_params->connectDevice( 0L );
   }

   KConfig *konf = KGlobal::config ();
   QString referToScanner( sane->shortScannerName());
   if( !haveConnection ) referToScanner = "gallery";

   konf->setGroup( referToScanner );
   QValueList<int> sizes = konf->readIntListEntry( STARTUP_SCANPARAM_SIZES );
   QValueList<int> vsizes = konf->readIntListEntry( STARTUP_VIEWER_SIZES );
   
   if( sizes.count() == 0  )
   {
      kdDebug(28000) << "Setting default sizes" << endl;
	 
      /* Shitty, nothing yet in the config */
      if( haveConnection )
      {
	 sizes << packager->height();
	 sizes << scan_params->height();
	 vsizes << (scan_params->sizeHint()).width();
      }
      else
      {
	 /* only push one value to vsizes */
	 vsizes << 150;
      }
   }
   kdDebug(28000) << "Setting sizes !" << endl;
   setSizes( vsizes );
   if( haveConnection )
   {
      paramSplitter->setSizes( sizes );

      scan_params->show();
      preview_canvas->loadPreviewImage( selDevice );
   }

   return( haveConnection );
}

QCString KookaView::userDeviceSelection( ) const
{
   /* Human readable scanner descriptions */
   QStringList hrbackends;

   /* a list of backends the scan backend knows */
   QStrList backends = sane->getDevices();
   QStrListIterator  it( backends );
   
   QCString selDevice;
   if( backends.count() > 0 )
   {
      while( it )
      {
	 kdDebug( 28000 ) << "Found backend: " << it.current() << endl;
	 hrbackends.append( sane->getScannerName( it.current() ));
	 ++it;
      }

      /* allow the user to select one */
       DeviceSelector ds( 0, backends, hrbackends );
       selDevice = ds.getDeviceFromConfig( );

       if( selDevice.isEmpty() || selDevice.isNull() )
       {
	  kdDebug(29000) << "selDevice not found - starting selector!" << selDevice << endl;
	  if ( ds.exec() == QDialog::Accepted )
	  {
	     selDevice = ds.getSelectedDevice();
	  }
       }
   }
   return( selDevice );
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
	 QString startup = konf->readEntry( STARTUP_IMG_SELECTION, "" );

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


void KookaView::print(QPainter *p, KPrinter* printer, QPaintDeviceMetrics& metrics )
{
   int pheight = metrics.height();
   int pwidth  = metrics.width();

   /* Check the device */
   if( pheight == 0 || pwidth == 0 ) return;
   
   (void) printer;

   const QImage *img = img_canvas->rootImage();

   kdDebug(28000) << "Printing canvas size: "<< pwidth << "x" << pheight << endl;
   if( img )
   {
      int w = img->width();
      int h = img->height();
      bool scaled = false;
      double ratio = 1.0;
      
      kdDebug(28000) << "printing image size " << w << " x " << h << endl;
      if( w > pwidth || h > pheight )
      {
	 /* scaling is neccessary */
	 double wratio = double(w) / double(pwidth);
	 double hratio = double(h) / double(pheight);

	 /* take the larger one */
	 ratio = wratio > hratio ? wratio : hratio;
	 if( ratio > 0.0 )
	 {
	    w = (int) (double(w) / ratio);
	    h = (int) (double(h) / ratio);
	 }
	 scaled = true;
	 kdDebug(28000) << "image ratio: " << ratio << " leads to new height " << h << endl;
      }

      QPoint poin ( 1+int(( pwidth-w)/2), 1+int((pheight-h)/2) );
      kdDebug(28000) << "Printing on point " << poin.x() << "/" << poin.y() << endl;
      if( scaled )
      {
         // Phys. image is larger than print area, needs  to scale
         p->drawImage( poin, img->smoothScale( w, h ));
	 kdDebug(28000) <<  "Needed to scale new size is " << w << "x" << h << endl;
      }
      else
      {
         // Phys. image fits on the page
         p->drawImage( poin, *img );
	 kdDebug(28000) <<  "Needed to scale new size is " << w << "x" << h << endl;
      }
   }
}

void KookaView::slNewPreview( QImage *new_img )
{
   if( new_img )
   {
      if( ! new_img->isNull() )
      {
	 ImgSaveStat is_stat = ISS_OK;
	 ImgSaver img_saver( this );

	 is_stat = img_saver.savePreview( new_img, sane->shortScannerName() );

	 if( is_stat != ISS_OK )
	 {
	    kdDebug(28000) << "ERROR in saving preview !" << endl;
	 }
      }
      preview_canvas->newImage( new_img ); 
   }
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

   QImage img;

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
   const QImage *img = img_canvas->rootImage();
   startOCR( img );
   emit( signalCleanStatusbar( ));
}

void KookaView::startOCR( const QImage *img )
{
   if( img && ! img->isNull() )
   {
      if( ocrFabric == 0L )
	 ocrFabric = new KSANEOCR(this );

      Q_CHECK_PTR( ocrFabric );
      ocrFabric->setImage( img );

      if( !ocrFabric->startExternOcrVisible() )
      {
	 KMessageBox::sorry(0, i18n("Could not start OCR-Process.\n"
				    "Probably there is already one running." ));

      }
   }
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



void KookaView::slScanFinished( KScanStat stat )
{
   kdDebug(28000) << "Scan finished with status " << stat << endl;
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
      scan_params = 0;
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
   QImage *img = (QImage*) img_canvas->rootImage();
   bool doUpdate = true;

   if( img )
   {
      QImage resImg;

      QApplication::setOverrideCursor(waitCursor);
      switch( angle )
      {
	 case 90:
	    emit( signalChangeStatusbar( i18n("rotate image 90 degrees" )));
	    resImg = rotateRight( img );
	    break;
	 case 180:
	    emit( signalChangeStatusbar( i18n("rotate image 180 degrees" )));
	    resImg = rotateRight( img );
	    resImg = rotateRight( &resImg );
	    break;
	 case 270:
	 case -90:
	    emit( signalChangeStatusbar( i18n("rotate image -90 degrees" )));
	    resImg = rotateLeft( img );

	    break;
	 default:
	    kdDebug(28000) << "Not supported yet !" << endl;
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
	    kdDebug(28000) << "Mirroring: no way ;)" << endl;
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

void KookaView::slSaveScanParams( void )
{
   if( !sane ) return;

   KScanOptSet optSet( "SaveSet" );
   
   sane->getCurrentOptions( &optSet );
#if 0
   SaveSetDialog dialog( this, &optSet );
   if( dialog.exec())
   {
      kdDebug(28000)<< "Executed successfully" << endl;
   }
   sane->slSaveScanConfigSet( "sysmtem-default", "default configuration" );
#endif
}

void KookaView::slShowThumbnails( const KURL& url )
{
   kdDebug(28000) << "Showing thumbs for " << url.prettyURL() << endl;
   m_stack->raiseWidget( m_thumbview );
}

/* this slot is called when the user clicks on an image in the packager
 * and loading of the image starts
 */
void KookaView::slStartLoading( const KURL& url )
{
   emit( signalChangeStatusbar( i18n("Loading " ) + url.prettyURL()));

   if( m_stack->visibleWidget() != img_canvas )
      m_stack->raiseWidget( img_canvas );

}


void KookaView::updateCurrImage( QImage& img )
{
   emit( signalChangeStatusbar( i18n("Storing image changes" )));
   packager->slotImageChanged( &img );
   emit( signalCleanStatusbar());
}


void KookaView::saveProperties(KConfig *config)
{
   kdDebug(28000) << "Saving Properties for KookaView !" << endl;
   config->setGroup( GROUP_STARTUP );
   /* Get with path */
   config->writeEntry( STARTUP_IMG_SELECTION, packager->getCurrImageFileName(true));

   QString tabwPre = "packager";
   int idx = tabw->currentPageIndex();
   kdDebug(28000) << "Idx ist" << idx << endl;
   if( idx == PREVIEWER_TAB )
      tabwPre = "previewer";

   config->writeEntry( STARTUP_SELECTED_PAGE, tabwPre );

   /* Save the scan parameters size in accordance to the selected scanner */
   if( sane && scan_params )
   {
      config->setGroup( sane->shortScannerName());
      QValueList<int> pssizes = paramSplitter->sizes();
      config->writeEntry( STARTUP_SCANPARAM_SIZES, pssizes );
   }
   else
   {
      config->setGroup( "gallery" );
   }
   /* Sizes of the vertical splitter */
   QValueList<int> vsizes = sizes();
   config->writeEntry(STARTUP_VIEWER_SIZES, vsizes );
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
      if ( m_img->depth() == 8)
      {
	 rot.create( m_img->height(), m_img->width(), 
		     m_img->depth(), m_img->numColors() );
	 for (int i=0; i<m_img->numColors(); i++)
	 {
	    rot.setColor( i, m_img->color(i) );
	 }
    
	 unsigned char **ssl = m_img->jumpTable();
    
	 for (int y=0; y<m_img->height(); y++)
	 {
	    unsigned char *p = *ssl++;
	    unsigned char **dsl = rot.jumpTable();
	    dsl += m_img->width()-1;
            
	    for (int x=0; x<m_img->width(); x++)
	    {
	       *((*dsl--) + y ) = *p++;
	    }
	 }
      }
      else
      {
	 rot.create( m_img->height(), m_img->width(), 
		     m_img->depth() );
    
	 QRgb **ssl = (QRgb **)m_img->jumpTable();
    
	 for (int y=0; y<m_img->height(); y++)
	 {
	    QRgb *p = *ssl++;
	    QRgb **dsl = (QRgb **)rot.jumpTable();
	    dsl += m_img->width()-1;
      
	    for (int x=0; x<m_img->width(); x++)
	    {
	       *((*dsl--) + y ) = *p++;
	    }
	 }
      }
   }
   return( rot );
}

QImage KookaView::rotateRight( QImage *m_img )
{
   QImage rot;

   if (m_img )
   {
      if (m_img->depth() == 8)
      {
	 rot.create( m_img->height(), m_img->width(), 
		     m_img->depth(), m_img->numColors() );
	 for (int i=0; i<m_img->numColors(); i++)
	 {
	    rot.setColor( i, m_img->color(i) );
	 }
    
	 unsigned char **ssl = m_img->jumpTable();
    
	 for (int y=0; y<m_img->height(); y++)
	 {
	    unsigned char *p = *ssl++;
	    unsigned char **dsl = rot.jumpTable();
            
	    for (int x=0; x<m_img->width(); x++)
	    {
	       *((*dsl++) + m_img->height() - y - 1 ) = *p++;
	    }
	 }
      }
      else
      {
	 rot.create( m_img->height(), m_img->width(), 
		     m_img->depth() );
    
	 QRgb **ssl = (QRgb **)m_img->jumpTable();
    
	 for (int y=0; y<m_img->height(); y++)
	 {
	    QRgb *p = *ssl++;
	    QRgb **dsl = (QRgb **)rot.jumpTable();
      
	    for (int x=0; x<m_img->width(); x++)
	    {
	       *((*dsl++) + m_img->height() - y - 1 ) = *p++;
	    }
	 }
      }
   }
   return( rot );
}

void KookaView::connectViewerAction( KAction *action )
{
   QPopupMenu *popup = img_canvas->contextMenu();

   if( popup && action )
   {
      action->plug( popup );
   }
}


#include "kookaview.moc"
