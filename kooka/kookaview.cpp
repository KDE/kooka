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

#include <ktrader.h>
#include <klibloader.h>
#include <kmessagebox.h>
#include <krun.h>
#include <keditcl.h>

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
   setResizeMode( left_splitter, QSplitter::KeepSize );

   /* The Tabwidget to contain the preview and the packager */
   tabw  = new QTabWidget( left_splitter, "TABWidget" );

   /* A new packager to contain the already scanned images */
   packager = new ScanPackager( tabw );
  	
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

   /* A dialog, which allows the user to select one of the scanner */
   DeviceSelector ds( this, backends );
   
   if( ds.exec() != QDialog::Accepted )
   {
      debug( "Selecting a device cancelled" );
   }

   QString selDevice = ds.getSelectedDevice();

   if( selDevice.isEmpty() || selDevice.isNull() )
   {
      debug( "Damned, no selected device-> TODO !" );
   }

   debug( "Opening device %s", (const char*) selDevice );

   /* This connects to the selected scanner */
   sane->openDevice( selDevice );
   if( ! scan_params->connectDevice( sane ) )
   {
      debug( "Connecting to the scanner failed :( ->TODO" );
   }
   
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
}

KookaView::~KookaView()
{
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
	    debug("ERROR in saving preview !" );
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


/* Does OCR on the entire picture */
void KookaView::doOCR( void )
{
   const QImage *img = img_canvas->rootImage();

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




#include "kookaview.moc"
