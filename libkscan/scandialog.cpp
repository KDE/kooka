#include <qlabel.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qstrlist.h>

#include <kglobal.h>
#include <klocale.h>

// libkscan stuff
#include "scanparams.h"
#include "devselector.h"

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
    connect(m_device, SIGNAL(sigNewImage(QImage *)), this, SLOT(slotFinalImage(QImage *)));

    QLabel *label = new QLabel( splitter );
    label->setBackgroundMode( PaletteBase );

    QStringList scannerNames;
    QStrList backends = m_device->getDevices();;
    QStrListIterator it( backends );

    while ( it.current() ) {
	scannerNames.append( m_device->getScannerName( it.current() ));
	++it;
    }

    resize(600, 500);

    DeviceSelector ds( this, backends, scannerNames );
    if ( ds.exec() == QDialog::Accepted ) {
	QString device = ds.getSelectedDevice();
	m_device->openDevice( device.local8Bit() );
	
	if ( !m_scanParams->connectDevice( m_device ) ) {
	    qWarning("*** connecting to device failed!");
	}
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
