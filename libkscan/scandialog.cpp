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

    resize(700, 500);
    
    m_previewer->setEnabled( false ); // will be enabled in setup()


    /* continue to attach a real scanner */
    /* first, get the list of available devices from libkscan */
    QStringList scannerNames;
    QStrList backends = m_device->getDevices();;
    QStrListIterator it( backends );

    while ( it.current() ) {
       scannerNames.append( m_device->getScannerName( it.current() ));
       ++it;
    }

    /* ..if there are devices.. */
    QCString configDevice;
    good_scan_connect = true;
    if( scannerNames.count() > 0 )
    {
       /* allow the user to select one */
       DeviceSelector ds( this, backends, scannerNames );
       configDevice = ds.getDeviceFromConfig( );

       if( configDevice.isEmpty() || configDevice.isNull() )
       {
	  kdDebug(29000) << "configDevice is not valid - starting selector!" << configDevice << endl;
	  if ( ds.exec() == QDialog::Accepted )
	  {
	     configDevice = ds.getSelectedDevice();
	  }
       }

       /* If a device was selected, */
       if( ! configDevice.isNull() )
       {
	  /* ..open it (init sane with that) */
	  m_device->openDevice( configDevice );

	  /* ..and connect to the gui (create the gui) */
	  if ( !m_scanParams->connectDevice( m_device ) )
	  {
	     kdDebug(29000) << "ERR: Could not connect scan device" << endl;
	     good_scan_connect = false;
	  }	
       }
    }

    if( configDevice.isNull() || configDevice.isEmpty() )
    {
       /* No scanner found, open with information */
       m_scanParams->connectDevice( 0L );
       good_scan_connect = false;
       /* Making the previewer gray */
       /* disabling is much better than hiding for 'advertising' ;) */
    }
    
    m_previewer->setEnabled( false );
}


bool ScanDialog::setup()
{
    // ### FIXME I think ScanDialog should release the scan-device after a 
    // while, e.g. when the dialog is closed. setup() is going to get called,
    // so we could re-open it again.

   /*  Temporary fix: We unfortunately can not open another backend whenever the dialog
    *  pops up, and it is more than a bugfix to change libkscan to be able to open
    *  _and_close_ backends dynamically. That will be an issue for coming releases.
    *  Thus, all the device connection stuff went up into the constructor.
    *
    *  To the user that means that he only can work with _one_ scanner after having
    *  started one of the office apps (and even kooka). To select another scanner,
    *  he would have to end and start the app again - not too cool, but no other way now.
    */
   if( good_scan_connect )   
      m_previewer->setEnabled( true );
    return true;
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
