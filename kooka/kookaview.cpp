#include "kookaview.h"
#include "resource.h"
#include "kscandevice.h"
#include "devselector.h"
#include "ksaneocr.h"

#include <qpainter.h>
#include <qlayout.h>
#include <qsplitter.h>
#include <qstrlist.h>

#include <kurl.h>
#include <krun.h>
#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>
#include <ktrader.h>
#include <klibloader.h>
#include <kmessagebox.h>
#include <krun.h>
#include <keditcl.h>


#define STARTUP_IMG_SELECTION "SelectedImageOnStartup"


KookaView::KookaView(QWidget *parent)
    : QSplitter(parent)
{

   /* Another splitter for splitting the Packager Tree from the Scan Parameter */
   ocrFabric = 0L;
   
   QSplitter *left_splitter = new QSplitter( QSplitter::Vertical, this );
   setOpaqueResize( false );
   left_splitter->setOpaqueResize( false );

   /* An image canvas for the large image, right side */
   
   img_canvas  = new ImageCanvas( this );
   
   setResizeMode( img_canvas,    QSplitter::Stretch );
   setResizeMode( left_splitter, QSplitter::FollowSizeHint );

   /* The Tabwidget to contain the preview and the packager */
   tabw  = new QTabWidget( left_splitter, "TABWidget" );

   /* A new packager to contain the already scanned images */
   packager = new ScanPackager( tabw );
   {
   }
   
   /* Image Canvas for the preview image, same object as the large image canvas */
   preview_img = 0L;
   preview_canvas = new Previewer( tabw );
   {
      preview_canvas->setMinimumSize( 100,100);
   	
      preview_img = new QImage();
      preview_img->load( packager->getSaveRoot() + "preview.bmp" );
      preview_canvas->newImage( preview_img );
   }

   /* build up the Preview/Packager-Tabview */
   tabw->addTab( packager, I18N( "&Images") );
   tabw->addTab( preview_canvas, I18N("&Preview") );

   tabw->setMinimumSize(100, 100);
   tabw->showPage(preview_canvas);	

   /* ..and create the Scanner parameter object, which displays the parameter
    *   the current scanner provides.
    */
   scan_params = new ScanParams( left_splitter);
   CHECK_PTR(scan_params);

   /* the object from the kscan lib to handle low level scanning */
   sane = new KScanDevice( this );
   CHECK_PTR(sane);
   
   /* a list of backends the scan backend knows */
   QStrList backends = sane->getDevices();

   /* Human readable scanner descriptions */
   QStringList hrbackends;
   QStrListIterator  it( backends );
  
   // for( it = backends.first(); it != backends.last(); ++it );
   while( it )
   {
      hrbackends.append( sane->getScannerName( it.current() ));
      ++it;
   }

   /* A dialog, which allows the user to select one of the scanner */
   kapp->config()->setGroup( GROUP_STARTUP );
   QString selDevice;
   bool skipDialog = kapp->config()->readBoolEntry( STARTUP_SKIP_ASK, false );
   if( skipDialog )
   {
      selDevice = kapp->config()->readEntry( STARTUP_SCANDEV, "" );
   }
   
   
   if( ! skipDialog || selDevice.isEmpty())
   {
      DeviceSelector ds( this, backends, hrbackends );
   
      if( ds.exec() == QDialog::Accepted )
      {
	 kapp->config()->writeEntry( STARTUP_SKIP_ASK,
				     ds.getShouldSkip());
      }
      else
      {
	 exit(0);
      }

      selDevice = ds.getSelectedDevice();
   }

   if( selDevice.isEmpty() || selDevice.isNull() )
   {
      kdDebug() << "Damned, no selected device-> TODO !" << endl;
   }

   kdDebug() << "Opening device " << selDevice << endl;

   /* This connects to the selected scanner */
   sane->openDevice( selDevice.local8Bit() );
   if( ! scan_params->connectDevice( sane ) )
   {
      kdDebug() << "Connecting to the scanner failed :( ->TODO" << endl;
   }
#if 0
   /* Try to set the left splitter to the requested width of the scan params */
   QSize s = scan_params->sizeHint();
   QValueList<int> sizelist;
   /* children of this: leftsplitter, canvas */
   sizelist.append( s.width() );
   debug( "Setting splitter width to %d", s.width() );
   setSizes( sizelist );

   /* children of leftsplitter: tabwidget, scanparams */	
   QValueList<int> sizel2;
   sizel2.append( 400 );
   sizel2.append( s.height() );

   left_splitter->setSizes( sizel2 );

   left_splitter->setResizeMode( scan_params, QSplitter::KeepSize );
   left_splitter->setResizeMode( tabw,        QSplitter::Stretch );

   debug( "Have size found: %d x %d", s.width(), s.height());
#endif
   scan_params->resize( scan_params->sizeHint() );
   scan_params->show();	


   /* New image created after scanning */
   connect(sane, SIGNAL(sigNewImage(QImage*)), packager, SLOT(slAddImage(QImage*)));	
   /* New preview image */
   connect(sane, SIGNAL(sigNewPreview(QImage*)), this, SLOT( slNewPreview(QImage*)));
	
   /* Image canvas should show a new document */
   connect( packager, SIGNAL( showImage( QImage* )),
            img_canvas, SLOT( newImage( QImage*)));
   /* Packager deletes the image */	
   connect( packager, SIGNAL( deleteImage( QImage* )),
            img_canvas, SLOT( deleteView( QImage*)));
   /* Packager unloads the image */
   connect( packager, SIGNAL( unloadImage( QImage* )),
            img_canvas, SLOT( deleteView( QImage*)));
	
   /* New Rectangle selection in the preview */
   connect( preview_canvas->getImageCanvas(), SIGNAL( newRect(QRect)),
            scan_params, SLOT(slCustomScanSize(QRect)));
   connect( preview_canvas->getImageCanvas(), SIGNAL( noRect()),
            scan_params, SLOT(slMaximalScanSize()));	


#if 0
   KTrader::OfferList offers = KTrader::self()->query("text/html", "'KParts/ReadOnlyPart' in ServiceTypes");

   KLibFactory *factory = 0;
   // in theory, we only care about the first one.. but let's try all
   // offers just in case the first can't be loaded for some reason
   KTrader::OfferList::Iterator it(offers.begin());
   for( ; it != offers.end(); ++it)
   {
      KService::Ptr ptr = (*it);

      // we now know that our offer can handle HTML and is a part.
      // since it is a part, it must also have a library... let's try to
      // load that now
      factory = KLibLoader::self()->factory( ptr->library() );
      if (factory)
      {
	 m_html = static_cast<KParts::ReadOnlyPart *>(factory->create(this, ptr->name(),
								      "KParts::ReadOnlyPart"));
	 break;
      }
   }

   // if our factory is invalid, then we never found our component
   // and we might as well just exit now
   if (!factory)
   {
      KMessageBox::error(this, "Could not find a suitable HTML component");
      return;
   }
#endif

   /* Now set the configured stuff */
   kapp->config()->setGroup(GROUP_STARTUP);
   QString startup = kapp->config()->readEntry( STARTUP_IMG_SELECTION, "" );
   kdDebug() << "KookaView: Loading Startup-Image <" << startup << ">" << endl;
   
   if( !startup.isEmpty()) {
      kdDebug() << "Loading startup image !" << endl;
      packager->slSelectImage( startup );
   }
   
   
   
}


KookaView::~KookaView()
{
   saveProperties( kapp->config() );
   if( preview_img ) delete( preview_img );
   
}

void KookaView::print(QPainter *p, int height, int width)
{
   (void) p;
   (void) height;
   (void) width;
   // do the actual printing, here
   // p->drawText(etc..)
}

void KookaView::slNewPreview( QImage *new_img )
{
   if( new_img )
   {
      if( preview_img )
	 delete( preview_img );
      preview_img = new QImage( *new_img );
	
      if( ! new_img->isNull() )
      {
	 ImgSaveStat is_stat = ISS_OK;
	 ImgSaver img_saver( this );
			
	 is_stat = img_saver.savePreview( new_img );
			
	 if( is_stat != ISS_OK )
	 {
	    kdDebug() << "ERROR in saving preview !" << endl;
	 }
      }
      preview_canvas->newImage( preview_img );
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
   emit( signalChangeStatusbar( I18N("Starting OCR on selection" )));

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
   emit( signalChangeStatusbar( I18N("Starting OCR on the entire image" )));
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

      CHECK_PTR( ocrFabric );
      ocrFabric->setImage( img );

      if( !ocrFabric->startExternOcrVisible() )
      {
	 KMessageBox::error(0, I18N("Could not start OCR-Process,\n"
				    "Probably there is already one running." ));
	 
      }
   }
}

void KookaView::slCreateNewImgFromSelection()
{
   if( img_canvas->rootImage() )
   {
      emit( signalChangeStatusbar( I18N("Create new image from selection" )));
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
	    emit( signalChangeStatusbar( I18N("rotate image 90 degree" )));
	    resImg = rotateRight( img );
	    break;
	 case 180:
	    emit( signalChangeStatusbar( I18N("rotate image 180 degree" )));
	    resImg = rotateRight( img );
	    resImg = rotateRight( &resImg );
	    break;
	 case 270:
	 case -90:
	    emit( signalChangeStatusbar( I18N("rotate image -90 degree" )));
	    resImg = rotateLeft( img );
	    
	    break;
	 default:
	    kdDebug() << "Not supported yet !" << endl;
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
	    emit( signalChangeStatusbar( I18N("Mirroring image vertically" )));
	    resImg = img->mirror();
	    break;
	 case MirrorHorizontal:
	    emit( signalChangeStatusbar( I18N("Mirroring image horizontally" )));
	    resImg = img->mirror( true, false );
	    break;
	 case MirrorBoth:
	    emit( signalChangeStatusbar( I18N("Mirroring image in both directions" )));
	    resImg = img->mirror( true, true );
	    break;
	 default:
	    kdDebug() << "Mirroring: no way ;)" << endl;
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

void KookaView::updateCurrImage( QImage& img ) 
{
   emit( signalChangeStatusbar( I18N("Storing image changes" )));
   packager->slotImageChanged( &img );
   emit( signalCleanStatusbar());
}


void KookaView::saveProperties(KConfig *config)
{
   kdDebug() << "Saving Properties for KookaView !" << endl;
   config->setGroup( GROUP_STARTUP );
   /* Get with path */
   config->writeEntry( STARTUP_IMG_SELECTION, packager->getCurrImageFileName(true));
}


void KookaView::slOpenCurrInGraphApp( void )
{
   QString file;
   
   if( packager ) {
      file = packager->getCurrImageFileName( true );
      
      kdDebug() << "Trying to open <" << file << ">" << endl;
      if( ! file.isEmpty() )
      {
	 KURL::List urllist;
	 
	 urllist.append( KURL(file) );
	 KFileOpenWithHandler kfoh;
	 kfoh.displayOpenWithDialog( urllist );
      }
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

#include "kookaview.moc"
