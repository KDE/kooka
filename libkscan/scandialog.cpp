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
}

bool ScanDialog::setup()
{
    // ### FIXME I think ScanDialog should release the scan-device after a 
    // while, e.g. when the dialog is closed. setup() is going to get called,
    // so we could re-open it again.
    
    QStringList scannerNames;
    QStrList backends = m_device->getDevices();;
    QStrListIterator it( backends );

    while ( it.current() ) {
        scannerNames.append( m_device->getScannerName( it.current() ));
        ++it;
    }

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
                // ### messagebox? return false?
            }	
        }
        else {// completely abort
            m_previewer->setEnabled( false );
            return false;
        }
    }
    else
    {
        /* No scanner found, open with information */
        m_scanParams->connectDevice( 0L );

        /* Making the previewer gray */
        /* disabling is much better than hiding for 'advertising' ;) */
        m_previewer->setEnabled( false );
        return true;
    }

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
