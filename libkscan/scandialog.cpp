#include <qlabel.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qstrlist.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

// libkscan stuff
#include "scanparams.h"
#include "devselector.h"
#include "img_canvas.h"
#include "previewer.h"
#include "scandialog.h"

extern "C" {
    void * init_libkscan() {
	return new ScanDialogFactory;
    }
}

ScanDialogFactory::ScanDialogFactory( QObject *parent, const char *name )
    : KScanDialogFactory( parent, name )
{
    setName( "ScanDialogFactory" );
    KGlobal::locale()->insertCatalogue( QString::fromLatin1("libkscan") );
}

KScanDialog * ScanDialogFactory::createDialog( QWidget *parent,
					       const char *name, bool modal)
{
    return new ScanDialog( parent, name, modal );
}


///////////////////////////////////////////////////////////////////


ScanDialog::ScanDialog( QWidget *parent, const char *name, bool modal )
    : KScanDialog( Tabbed, Close|Help, parent, name, modal )
{
    QVBox *page = addVBoxPage( i18n("Scanning") );
    QSplitter *splitter = new QSplitter( Horizontal, page, "splitter" );

    m_scanParams = new ScanParams( splitter );
    m_device = new KScanDevice( this );
    connect(m_device, SIGNAL(sigNewImage(QImage *)),
	    this, SLOT(slotFinalImage(QImage *)));

    /* Create a preview widget to the right side of the splitter */
    m_previewer = new Previewer( splitter );
    CHECK_PTR(m_previewer );
    /* ... and connect to the selector-slots. They communicate user's
     * selection to the scanner parameter engine */
    connect( m_previewer->getImageCanvas(), SIGNAL( newRect(QRect)),
	     m_scanParams, SLOT(slCustomScanSize(QRect)));
    connect( m_previewer->getImageCanvas(), SIGNAL( noRect()),
	     m_scanParams, SLOT(slMaximalScanSize()));	
    
    /* a new preview signal */
    connect( m_device, SIGNAL( sigNewPreview( QImage* )),
	     this, SLOT( slotNewPreview( QImage* )));
    
    // QLabel *label = new QLabel( splitter );
    // label->setBackgroundMode( PaletteBase );

    QStringList scannerNames;
    QStrList backends = m_device->getDevices();;
    QStrListIterator it( backends );

    while ( it.current() ) {
	scannerNames.append( m_device->getScannerName( it.current() ));
	++it;
    }

    resize(700, 500);

    if( scannerNames.count() > 0 )
    {
       DeviceSelector ds( this, backends, scannerNames );
       if ( ds.exec() == QDialog::Accepted )
       {
	  m_device->openDevice( ds.getSelectedDevice() );

	  /* ConnectDevice does the gui for scanParams. */
	  if ( !m_scanParams->connectDevice( m_device ) )
	  {
	     kdDebug(29000) << "ERR: Could not connect scan device" << endl;
	  }	
       }
       else
       {
	  /** TODO: hmmm - this one should skip the entire action **/
       }
    }
    else
    {
       /* No scanner found, open with information */
       m_scanParams->connectDevice( 0L );

       /* Making the previewer gray */
       /* disabling is much better than hiding for 'advertising' ;) */
       m_previewer->setEnabled( false );
    }
}



void ScanDialog::slotNewPreview( QImage *image )
{
   if( image )
   {
      m_previewImage = *image;
      // hmmm - dont know, if conversion of the bit-depth is neccessary.
      // m_previewImage.convertDepth(32);

      /* The previewer does not copy the image data ! */
      m_previewer->newImage( &m_previewImage );
   }
   
}

ScanDialog::~ScanDialog()
{
}

void ScanDialog::slotFinalImage(QImage *image)
{
    emit finalImage(*image, nextId());
}

#include "scandialog.moc"
