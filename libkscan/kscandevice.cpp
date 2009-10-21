/* This file is part of the KDE Project
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kscandevice.h"
#include "kscandevice.moc"

#include <stdlib.h>
#include <unistd.h>

#include <qwidget.h>
#include <qobject.h>
#include <q3asciidict.h>
#include <qcombobox.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qimage.h>
#include <qfileinfo.h>
#include <qapplication.h>
#include <qsocketnotifier.h>

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kstandarddirs.h>

#include "kgammatable.h"
#include "kscancontrols.h"
#include "kscanoption.h"
#include "kscanoptset.h"
#include "devselector.h"
#include "imgscaninfo.h"
#include "previewer.h"


#define MIN_PREVIEW_DPI		75
#define UNDEF_SCANNERNAME	I18N_NOOP( "undefined" )
#define MAX_PROGRESS		100

#define USERDEV_GROUP		"User Specified Scanners"
#define USERDEV_DEVS		"Devices"
#define USERDEV_DESC		"Description"

/* ---------------------------------------------------------------------------
   Private class for KScanDevice
   ------------------------------------------------------------------------- */
class KScanDevice::KScanDevicePrivate

{
public:
    KScanDevicePrivate()
	: currScanResolutionX(0),
	  currScanResolutionY(0)
	{
	    
	}

    int currScanResolutionX, currScanResolutionY;
    
};


/* ---------------------------------------------------------------------------

   ------------------------------------------------------------------------- */
void KScanDevice::guiSetEnabled( const QByteArray& name, bool state )
{
    KScanOption *so = getExistingGuiElement( name );

    if( so )
    {
	QWidget *w = so->widget();

	if( w )
	    w->setEnabled( state );
    }
}


/* ---------------------------------------------------------------------------

   ------------------------------------------------------------------------- */
KScanOption *KScanDevice::getExistingGuiElement( const QByteArray& name )
{
    KScanOption *ret = NULL;
    QByteArray alias = aliasName(name);

    for (QList<KScanOption *>::const_iterator it = gui_elements.constBegin();
         it!=gui_elements.constEnd(); ++it)
    {
        KScanOption *opt = (*it);
        if (opt->getName()==alias)
        {
            ret = opt;
            break;
        }
    }

    return (ret);
}
/* ---------------------------------------------------------------------------

   ------------------------------------------------------------------------- */

KScanOption *KScanDevice::getGuiElement(const QByteArray &name,
                                        QWidget *parent,
                                        const QString &desc,
                                        const QString &tooltip )
{
   if (name.isEmpty()) return (NULL);
   if (!optionExists(name)) return (NULL);

   QByteArray alias = aliasName( name );

   kDebug() << "for name" << name << "desc" << desc << "alias" << alias;

   /* Check if already exists */
   KScanOption *so = getExistingGuiElement( name );

   if( so ) return( so );

   /* ...else create a new one */
   so = new KScanOption( alias );

   if( so->valid() && so->softwareSetable())
   {
      /** store new gui-elem in list of all gui-elements */
      gui_elements.append( so );

      QWidget *w = so->createWidget( parent, desc, tooltip );
      if( w )
      {
	 connect( so,   SIGNAL( optionChanged( KScanOption* ) ),
		  this, SLOT(   slotOptChanged( KScanOption* )));
	 w->setEnabled( so->active() );
      }
      else
      {
	 kDebug() << "Error: No widget created for" << name;
      }
   }
   else
   {
      if( !so->valid())
	 kDebug() << "no option" << alias;
      else
      if( !so->softwareSetable())
	 kDebug() << "option" << alias << "is not Software Settable";

      delete so;
      so = 0;
   }

   return( so );
}
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------


KScanDevice::KScanDevice( QObject *parent )
   : QObject( parent )
{
    kDebug();

    /* Get SANE translations - bug 98150 */
    KGlobal::dirs()->addResourceDir( "locale", QString::fromLatin1("/usr/share/locale/") );
    KGlobal::locale()->insertCatalog(QString::fromLatin1("sane-backends"));

    sane_stat = sane_init(NULL, NULL );

    d = new KScanDevicePrivate();
    
    option_dic = new Q3AsciiDict<int>;
    option_dic->setAutoDelete( true );

    scanner_initialised = false;  /* stays false until openDevice. */
    scanStatus = SSTAT_SILENT;

    mScanBuf        = NULL; /* temporary image data buffer while scanning */
    mSocketNotifier           = NULL; /* socket notifier for async scanning         */
    mScanImage          = NULL; /* temporary image to scan in                 */
    storeOptions = NULL; /* list of options to store during preview    */
    overall_bytes = 0;
    rest_bytes = 0;
    pixel_x = 0;
    pixel_y = 0;
    scanner_name = "";

    const KConfigGroup grp = KGlobal::config()->group(GROUP_STARTUP);
    bool netaccess = grp.readEntry( STARTUP_ONLY_LOCAL, false );
    kDebug() << "Query for network scanners:" << (netaccess ? "not enabled" : "enabled");

    if( sane_stat == SANE_STATUS_GOOD )
    {
        sane_stat = sane_get_devices( &dev_list, netaccess ? SANE_TRUE : SANE_FALSE );

        // NO network devices yet

        // Store all available Scanner to Stringlist
        for( int devno = 0; sane_stat == SANE_STATUS_GOOD &&
	      dev_list[ devno ]; ++devno )
        {
	    if( dev_list[devno] )
 	    {
	        scanner_avail.append( dev_list[devno]->name );
		scannerDevices.insert( dev_list[devno]->name, dev_list[devno] );
	    	kDebug() << "Found scanner:" << dev_list[devno]->name;
            }
        }

	const KConfig scanConfig(SCANNER_DB_FILE, KConfig::SimpleConfig);
        const KConfigGroup grp = scanConfig.group(USERDEV_GROUP);
	QStringList devs = grp.readEntry(USERDEV_DEVS, QStringList());
	if (devs.count()>0)
	{
		QStringList dscs = grp.readEntry(USERDEV_DESC, QStringList());
		QStringList::const_iterator it2 = dscs.begin();
		for (QStringList::const_iterator it1 = devs.begin(); it1!=devs.end(); ++it1,++it2)
		{					// avoid duplication
                    if (!scanner_avail.contains((*it1).toLocal8Bit()))
			addUserSpecifiedDevice((*it1),(*it2));
		}
	}

#if 0
        connect( this, SIGNAL(sigOptionsChanged()), SLOT(slotRoadAll()));
#endif
	gammaTables = new KScanOptSet( "GammaTables" );
     }
     else
     {
        kDebug() << "Error: sane_init() failed -> SANE installed?";
     }

    connect( this, SIGNAL( sigScanFinished( KScanStat )), SLOT( slotScanFinished( KScanStat )));
}


KScanDevice::~KScanDevice()
{
    kDebug();

    sane_exit();
    delete storeOptions;
    delete d;
}



void KScanDevice::addUserSpecifiedDevice(const QString& backend,const QString& description,bool dontSave)
{
	if (backend.isEmpty()) return;

	kDebug() << "adding" << backend << "desc" << description << "dontSave" << dontSave;

	if (!dontSave)
	{
		KConfig scanConfig(SCANNER_DB_FILE, KConfig::SimpleConfig);
                KConfigGroup grp = scanConfig.group(USERDEV_GROUP);

		QStringList devs = grp.readEntry(USERDEV_DEVS, QStringList());
		QStringList dscs = grp.readEntry(USERDEV_DESC, QStringList());
							// get existing device lists
		int i = devs.indexOf(backend);
		if (i>=0)				// see if already in list
		{
			dscs[i] = description;		// if so just update
		}
		else
		{
			devs.append(backend);		// add new entry to lists
			dscs.append(description);
		}

		grp.writeEntry(USERDEV_DEVS, devs);
		grp.writeEntry(USERDEV_DESC, dscs);
		grp.sync();
	}

	SANE_Device *userdev = new SANE_Device;

	// Need a permanent copy of the strings, because SANE_Device only holds
	// pointers to them.  Unfortunately there is a memory leak here, the
	// two QByteArray's are never deleted.  There is only a limited number
        // of these objects in most applications, so hopefully it won't matter
        // too much.

	userdev->name = *(new QByteArray(backend.toLocal8Bit()));
	userdev->model = *(new QByteArray(description.toLocal8Bit()));
	userdev->vendor = "User specified";
	userdev->type = "scanner";

	scanner_avail.append(userdev->name);
	scannerDevices.insert(userdev->name,userdev);
}



KScanStat KScanDevice::openDevice( const QByteArray& backend )
{
   KScanStat    stat      = KSCAN_OK;

   kDebug() << "backend" << backend;

   sane_stat = SANE_STATUS_UNSUPPORTED;
   if( backend.isEmpty() ) return (KSCAN_ERR_PARAM);

   // search for scanner
   if (!scanner_avail.contains( backend )) return (KSCAN_ERR_NO_DEVICE);

   QApplication::setOverrideCursor(Qt::WaitCursor);	// potential lengthy operation
   // if available, build lists of properties
   sane_stat = sane_open( backend, &scanner_handle );
   if( sane_stat == SANE_STATUS_GOOD )
   {
	     // fill description dic with names and numbers
	   stat = find_options();
	   scanner_name = backend;
   } else {
	   stat = KSCAN_ERR_OPEN_DEV;
	   scanner_name = UNDEF_SCANNERNAME;
   }

   if( stat == KSCAN_OK )
      scanner_initialised = true;

   QApplication::restoreOverrideCursor();
   return( stat );
}




QString KScanDevice::lastErrorMessage() const
{
	kDebug() << "stat" << sane_stat;
	return (sane_strstatus(sane_stat));
}



// TODO: does this need to be a slot?
void KScanDevice::slotCloseDevice()
{
   /* First of all, send a signal to close down the scanner dev. */
   emit( sigCloseDevice( ));

   kDebug() << "Saving scan settings";
   slotSaveScanConfigSet( DEFAULT_OPTIONSET, i18n("the default startup setup"));

   /* After return, delete all related stuff */
   scanner_name = UNDEF_SCANNERNAME;
   if( scanner_handle )
   {
      if( scanStatus != SSTAT_SILENT )
      {
         kDebug() << "Scanner is still active, calling cancel!";
         sane_cancel( scanner_handle );
      }
      sane_close( scanner_handle );
      scanner_handle = 0;
   }

   qDeleteAll(gui_elements);
   gui_elements.clear();

   option_dic->clear();
   scanner_initialised = false;
}


QString KScanDevice::getScannerName(const QByteArray &name) const
{
    QString ret = i18n("No scanner selected");
    SANE_Device *scanner = NULL;

    if (!scanner_name.isNull() && scanner_initialised && name.isEmpty())
    {
        scanner = scannerDevices[scanner_name];
    }
    else
    {
        scanner = scannerDevices[name];
        ret = QString::null;
    }
    if (scanner!=NULL) ret.sprintf("%s %s", scanner->vendor, scanner->model);

    kDebug() << "returning" << ret;
    return (ret);
}


QSize KScanDevice::getMaxScanSize( void ) const
{
   QSize s;
   double min, max, q;

   KScanOption so_w( SANE_NAME_SCAN_BR_X );
   so_w.getRange( &min, &max, &q );

   s.setWidth( (int) max );

   KScanOption so_h( SANE_NAME_SCAN_BR_Y );
   so_h.getRange( &min, &max, &q );

   s.setHeight( (int) max );

   return( s );
}


KScanStat KScanDevice::find_options()
{
   KScanStat 	stat = KSCAN_OK;
   SANE_Int 	n;
   SANE_Int 	opt;
   int 	   	*new_opt;

  SANE_Option_Descriptor *d;

  if( sane_control_option(scanner_handle, 0,SANE_ACTION_GET_VALUE, &n, &opt)
      != SANE_STATUS_GOOD )
     stat = KSCAN_ERR_CONTROL;

  // printf("find_options(): Found %d options\n", n );

  // resize the Array which hold the descriptions
  if( stat == KSCAN_OK )
  {

     option_dic->clear();

     for(int i = 1; i<n; i++)
     {
	d = (SANE_Option_Descriptor*)
	   sane_get_option_descriptor( scanner_handle, i);

	if( d!=NULL )
	{
	   // logOptions( d );
	   if (d->name!=NULL)
	   {
	      // Die Option anhand des Namen in den Dict

	      if( strlen( d->name ) > 0 )
	      {
		 new_opt = new int;
		 *new_opt = i;
		 kDebug() << "Inserting" << d->name << "as" << *new_opt;
		 /* create a new option in the set. */
		 option_dic->insert ( (const char*)d->name, new_opt );
		 option_list.append( (const char*) d->name );
#if 0
		 KScanOption *newOpt = new KScanOption( d->name );
		 const QString qq = newOpt->get();
		 qDebug( "INIT: <%s> = <%s>", d->name, qq );
		 allOptionSet->insert( d->name, newOpt );
#endif
	      }
	      else if( d->type == SANE_TYPE_GROUP )
	      {
	      	// qDebug( "######### Group found: %s ########", d->title );
	      }
	      else
		 kDebug() << "Unable to detect option";
	   }
	}
     }
  }
  return stat;
}


QList<QByteArray> KScanDevice::getAllOptions()
{
    return (option_list);
}


QList<QByteArray> KScanDevice::getCommonOptions()
{
    QList<QByteArray> opts;

    for (QList<QByteArray>::const_iterator it = option_list.constBegin();
         it!=option_list.constEnd(); ++it)
    {
        const QByteArray optname = (*it);
        KScanOption opt(optname);
        if (opt.commonOption()) opts.append(optname);
    }

    return (opts);
}


QList<QByteArray> KScanDevice::getAdvancedOptions()
{
    QList<QByteArray> opts;

    for (QList<QByteArray>::const_iterator it = option_list.constBegin();
         it!=option_list.constEnd(); ++it)
    {
        const QByteArray optname = (*it);
        KScanOption opt(optname);
        if (!opt.commonOption()) opts.append(optname);
    }

    return (opts);
}


KScanStat KScanDevice::apply( KScanOption *opt, bool isGammaTable )
{
   KScanStat   stat = KSCAN_OK;
   if( !opt ) return( KSCAN_ERR_PARAM );
   int sane_result = 0;

   int         *num = (*option_dic)[ opt->getName() ];
   sane_stat = SANE_STATUS_GOOD;
   const QByteArray& oname = opt->getName();

   if ( oname == "preview" || oname == "mode" ) {
      sane_stat = sane_control_option( scanner_handle, *num,
				       SANE_ACTION_SET_AUTO, 0,
				       &sane_result );
      /* No return here, please ! Carsten, does it still work than for you ? */
   }


   if( ! opt->initialised() || opt->getBuffer() == 0 )
   {
      kDebug() << "Attempt to set uninit/null buffer of" << oname << "-> skipping!";

      if( opt->autoSetable() )
      {
	 kDebug() << "Setting option" << oname << "automatic";
	 sane_stat = sane_control_option( scanner_handle, *num,
					  SANE_ACTION_SET_AUTO, 0,
					  &sane_result );
      }
      else
      {
	 sane_stat = SANE_STATUS_INVAL;
      }
      stat = KSCAN_ERR_PARAM;
   }
   else
   {
      if( ! opt->active() )
      {
	 kDebug() << "Option" << oname << "is not active";
	 stat = KSCAN_OPT_NOT_ACTIVE;
      }
      else if( ! opt->softwareSetable() )
      {
	 kDebug() << "Option" << oname << "is not Software Settable";
	 stat = KSCAN_OPT_NOT_ACTIVE;
      }
      else
      {

	 sane_stat = sane_control_option( scanner_handle, *num,
					  SANE_ACTION_SET_VALUE,
					  opt->getBuffer(),
					  &sane_result );
      }
   }

   if( stat == KSCAN_OK )
   {
      if( sane_stat == SANE_STATUS_GOOD )
      {
	 kDebug() << "Applied" << oname << "successfully";

	 if( sane_result & SANE_INFO_RELOAD_OPTIONS )
	 {
	    kDebug() << "Setting status to reload options";
	    stat = KSCAN_RELOAD;
#if 0
	    qDebug( "Emitting sigOptionChanged()" );
	    emit( sigOptionsChanged() );
#endif
	 }

#if 0
	 if( sane_result & SANE_INFO_RELOAD_PARAMS )
	    emit( sigScanParamsChanged() );
#endif
	 if( sane_result & SANE_INFO_INEXACT )
	 {
	    kDebug() << "Option" << oname << "was set inexact";
	 }

	 /* if it is a gamma table, the gamma values must be stored */
	 if( isGammaTable )
	 {
	    gammaTables->backupOption( *opt );
	    kDebug() << "GammaTable stored:" << opt->getName();
	 }
      }
      else
      {
	 kDebug() << "Bad SANE status" << sane_strstatus(sane_stat) << "for option" << oname;

      }
   }
   else
   {
      kDebug() << "Setting option" << oname << "failed";
   }

   if( stat == KSCAN_OK )
   {
      slotSetDirty( oname );
   }

   return( stat );
}

bool KScanDevice::optionExists( const QByteArray& name )
{
   if( name.isEmpty() ) return false;
   int *i = 0L;

   QByteArray altname = aliasName( name );

   if( ! altname.isNull() )
       i = (*option_dic)[ altname ];

   if( !i )
       return( false );
   return( *i > -1 );
}


// TODO: does this need to be a slot?
void KScanDevice::slotSetDirty(const QByteArray &name)
{
    if (optionExists(name))
    {
        if (!dirtyList.contains(name))
        {
            kDebug() << "Setting dirty" << name;
            dirtyList.append(name);
        }
    }
}


/* This function tries to find name aliases which appear from backend to backend.
 *  Example: Custom-Gamma is for epson backends 'gamma-correction' - not a really
 *  cool thing :-|
 *  Maybe this helps us out ?
 */
QByteArray KScanDevice::aliasName( const QByteArray& name )
{
    int *i = (*option_dic)[ name ];
    QByteArray ret;

    if( i ) return( name );
    ret = name;

    if( name == SANE_NAME_CUSTOM_GAMMA )
    {
	if((*option_dic)["gamma-correction"])
	    ret = "gamma-correction";

    }

    if( ret != name )
	kDebug() << "Found alias for" << name << "which is" << ret;

    return( ret );
}



/* Nothing to do yet. This Slot may get active to do same user Widget changes */
void KScanDevice::slotOptChanged(KScanOption *opt)
{
    kDebug() << "for option" << opt->getName();
    apply(opt);
}



/* This might result in a endless recursion ! */
void KScanDevice::slotReloadAllBut(KScanOption *not_opt)
{
    if (not_opt==NULL)
    {
        kDebug() << "called with invalid argument";
        return;
    }

    kDebug() << "Reload of all except" << not_opt->getName() << "forced";
    /* Make sure it's applied */
    apply(not_opt);

    for (QList<KScanOption *>::const_iterator it = gui_elements.constBegin();
         it!=gui_elements.constEnd(); ++it)
    {
        KScanOption *so = (*it);
        if (so!=not_opt)
        {
            kDebug() << "Reloading" << so->getName();
            so->slotReload();
            so->slotRedrawWidget(so);
        }
    }

    kDebug() << "Finished";
}



/* This might result in a endless recursion ! */
void KScanDevice::slotReloadAll()
{
    kDebug();

    for (QList<KScanOption *>::const_iterator it = gui_elements.constBegin();
         it!=gui_elements.constEnd(); ++it)
    {
        KScanOption *so = (*it);
        so->slotReload();
        so->slotRedrawWidget(so);
    }
}



void KScanDevice::slotStopScanning( void )
{
    kDebug() << "Attempt to stop scanning";
    if( scanStatus == SSTAT_IN_PROGRESS )
    {
	emit( sigScanFinished( KSCAN_CANCELLED ));
    }
    scanStatus = SSTAT_STOP_NOW;
}


const QString KScanDevice::previewFile()
{
// TODO: this doesn't work if that directory doesn't exist,
// and nothing ever creates that directory!
// Save somewhere in the application directory instead.

    QString dir = Previewer::galleryRoot();

    QString fname = dir + QString::fromLatin1(".previews/");
    QString sname( getScannerName(shortScannerName()) );
    sname.replace( '/', "_");

    return fname+sname;
}

QImage KScanDevice::loadPreviewImage()
{
   const QString prevFile = previewFile();
   kDebug() << "Loading preview from" << prevFile;

   QImage image;
   image.load( prevFile );

   return image;
}

bool KScanDevice::savePreviewImage( const QImage &image )
{
   if( image.isNull() )
	return false;

   const QString prevFile = previewFile();
   kDebug() << "Saving preview to" << prevFile;

   return image.save( prevFile, "BMP" );
}


KScanStat KScanDevice::acquirePreview( bool forceGray, int dpi )
{
   double min, max, q;

   (void) forceGray;

    if (storeOptions!=NULL) storeOptions->clear();
    else storeOptions = new KScanOptSet("TempStore");

   /* set Preview = ON if exists */
   if( optionExists( SANE_NAME_PREVIEW ) )
   {
      KScanOption prev( aliasName(SANE_NAME_PREVIEW) );

      prev.set( true );
      apply( &prev );

      /* after having applied, save set to false to switch preview mode off after
	 scanning */
      prev.set( false );
      storeOptions->backupOption( prev );
   }

   /* Gray-Preview only  done by widget ? */
   if( optionExists( SANE_NAME_GRAY_PREVIEW ))
   {
     KScanOption *so = getExistingGuiElement( SANE_NAME_GRAY_PREVIEW );
     if( so )
     {
       if( so->get() == "true" )
       {
	 /* Gray preview on */
	 so->set( true );
	 kDebug() << "Setting GrayPreview ON";
       }
       else
       {
	 so->set(false );
	 kDebug() << "Setting GrayPreview OFF";
       }
     }
     apply( so );
   }


   if( optionExists( SANE_NAME_SCAN_MODE ) )
   {
      KScanOption mode( SANE_NAME_SCAN_MODE );
      const QString kk = mode.get();
      kDebug() << "Scan Mode is" << kk;
      storeOptions->backupOption( mode );
      /* apply if it has a widget, or apply always ? */
      if( mode.widget() ) apply( &mode );
   }

   /** Scan Resolution should always exist. **/
   KScanOption res ( SANE_NAME_SCAN_RESOLUTION );
   const QString p = res.get();

   kDebug() << "Scan Resolution (pre-preview) is" << p;
   storeOptions->backupOption( res );

   int set_dpi = dpi;

   if( dpi == 0 )
   {
       /* No resolution argument */
       if( ! res.getRange( &min, &max, &q ) )
       {
	   if( ! res.getRangeFromList ( &min, &max, &q ) )
	   {
	       kDebug() << "Could not retrieve resolution range!";
	       min = 75.0; // Hope that every scanner can 75
	   }
       }
       kDebug() << "Minimum Range" << min << "Maximum Range" << max;

      if( min > MIN_PREVIEW_DPI )
	 set_dpi = (int) min;
      else
	 set_dpi = MIN_PREVIEW_DPI;
   }

   /* Set scan resolution for preview. */
   if( !optionExists( SANE_NAME_SCAN_Y_RESOLUTION ) )
      d->currScanResolutionY = 0;
   else
   {
      KScanOption yres ( SANE_NAME_SCAN_Y_RESOLUTION );
      /* if active ? */
      storeOptions->backupOption( yres );
      yres.set( set_dpi );
      apply( &yres );
      yres.get( &d->currScanResolutionY );

      /* Resolution bind switch ? */
      if( optionExists( SANE_NAME_RESOLUTION_BIND ) )
      {
	 KScanOption bind_so( SANE_NAME_RESOLUTION_BIND );
	 /* Switch binding on if available */
	 storeOptions->backupOption( bind_so );
	 bind_so.set( true );
	 apply( &bind_so );
      }
   }

   res.set( set_dpi );
   apply( &res );

   /* Store the resulting preview for additional image information */
   res.get( &d->currScanResolutionX );

   if ( d->currScanResolutionY == 0 )
      d->currScanResolutionY = d->currScanResolutionX;

   /* Start scanning */
   KScanStat stat = acquire_data( true );

   /* Restauration of the previous values must take place in the scanfinished slot,
    *  because scanning works asynchron now.
    */

   return( stat );
}



/**
 * prepareScan tries to set as much as parameters as possible.
 *
 **/

inline const char *optionNotifyString(int opt)
{
    return (opt>0 ? "X  |" : "-  |");
}


void KScanDevice::prepareScan( void )
{
    Q3AsciiDictIterator<int> it( *option_dic ); // iterator for dict

    kDebug() << "######################################################################";
    kDebug() << "Scanner" << scanner_name << "=" << getScannerName();
    kDebug() << "----------------------------------+----+----+----+----+----+----+----+";
    kDebug() << " Option-Name                      |SSEL|HSEL|SDET|EMUL|AUTO|INAC|ADVA|";
    kDebug() << "----------------------------------+----+----+----+----+----+----+----+";

    while ( it.current() )
    {
       // qDebug( "%s -> %d", it.currentKey().latin1(), *it.current() );
       int descriptor = *it.current();

       const SANE_Option_Descriptor *d = sane_get_option_descriptor( scanner_handle, descriptor );

       if( d )
       {
	  int cap = d->cap;
	  
	  QString s = QString(it.currentKey()).leftJustified(32);
	  kDebug() << s << "|" <<
		 optionNotifyString( ((cap) & SANE_CAP_SOFT_SELECT)) << 
		 optionNotifyString( ((cap) & SANE_CAP_HARD_SELECT)) << 
		 optionNotifyString( ((cap) & SANE_CAP_SOFT_DETECT)) << 
		 optionNotifyString( ((cap) & SANE_CAP_EMULATED)   ) << 
		 optionNotifyString( ((cap) & SANE_CAP_AUTOMATIC)  ) << 
		 optionNotifyString( ((cap) & SANE_CAP_INACTIVE)   ) << 
		 optionNotifyString( ((cap) & SANE_CAP_ADVANCED )  );

       }
       ++it;
    }
    kDebug() << "----------------------------------+----+----+----+----+----+----+----+";

    KScanOption pso( SANE_NAME_PREVIEW );
    kDebug() << "Preview-Switch is" << pso.get();
}

/** Starts scanning
 *  depending on if a filename is given or not, the function tries to open
 *  the file using the Qt-Image-IO or really scans the image.
 **/
KScanStat KScanDevice::acquire( const QString& filename )
{
    if( filename.isEmpty() )
    {
 	/* *real* scanning - apply all Options and go for it */
 	prepareScan();

        for (QList<KScanOption *>::const_iterator it = gui_elements.constBegin();
             it!=gui_elements.constEnd(); ++it)
        {
            KScanOption *so = (*it);
 	    if (so->active())
 	    {
 	         kDebug() << "apply" << so->getName();
 	         apply( so );
 	    }
 	    else
 	    {
 	        kDebug() << "Option" << so->getName() << "is not active";
 	    }
 	}

	/** Scan Resolution should always exist. **/
	KScanOption res( SANE_NAME_SCAN_RESOLUTION );
	res.get( &d->currScanResolutionX );
        if ( !optionExists( SANE_NAME_SCAN_Y_RESOLUTION ) )
           d->currScanResolutionY = d->currScanResolutionX;
        else
        {
           KScanOption yres( SANE_NAME_SCAN_Y_RESOLUTION );
           yres.get( &d->currScanResolutionY );
        }

	return( acquire_data( false ));
    }
    else
    {
   	/* a filename is in the parameter */
	QFileInfo file( filename );
	if( file.exists() )
	{
	     QImage i;
	     ImgScanInfo info;
	     if( i.load( filename ))
	     {
		info.setXResolution(i.dotsPerMeterX()); // TODO: *2.54/100
		info.setYResolution(i.dotsPerMeterY()); // TODO: *2.54/100
		info.setScannerName(filename);
		emit( sigNewImage( &i, &info ));
	     }
	}
        else
        {
            return KSCAN_ERR_PARAM;
        }
    }

    return KSCAN_OK;
}


/**
 *  Creates a new QImage from the retrieved Image Options
 **/
KScanStat KScanDevice::createNewImage(const SANE_Parameters *p)
{
    if (p==NULL) return (KSCAN_ERR_PARAM);
    KScanStat stat = KSCAN_OK;

    delete mScanImage;
    mScanImage = NULL;

    if (p->depth==1)					//  Line art (bitmap)
    {
        mScanImage = new QImage(p->pixels_per_line,p->lines,QImage::Format_Mono);
        if (mScanImage!=NULL)
        {
            mScanImage->setColor(0,qRgb(0x00,0x00,0x00));
            mScanImage->setColor(1,qRgb(0xFF,0xFF,0xFF));
        }
    }
    else if (p->depth==8)				// 8 bit RGB
    {
        if (p->format==SANE_FRAME_GRAY)			// Grey scale
        {
            mScanImage = new QImage(p->pixels_per_line,p->lines,QImage::Format_Indexed8);
            if (mScanImage!=NULL)
            {
                for (int i = 0; i<256; i++) mScanImage->setColor(i,qRgb(i,i,i));
            }
        }
        else						// True colour
        {
            mScanImage = new QImage(p->pixels_per_line,p->lines,QImage::Format_RGB32);
        }
    }
    else						// Error, no others supported
    {
        kDebug() << "Only bit depths 1 or 8 supported!";
        stat = KSCAN_ERR_PARAM;
    }

    if (stat==KSCAN_OK && mScanImage==NULL) stat = KSCAN_ERR_MEMORY;
    return (stat);
}


KScanStat KScanDevice::acquire_data( bool isPreview )
{
   sane_stat = SANE_STATUS_GOOD;
   KScanStat       stat = KSCAN_OK;

   scanningPreview = isPreview;

   emit sigScanStart();

   QApplication::setOverrideCursor(Qt::WaitCursor);	// potential lengthy operation
   sane_stat = sane_start( scanner_handle );
   if( sane_stat == SANE_STATUS_GOOD )
   {
      sane_stat = sane_get_parameters( scanner_handle, &sane_scan_param );

      if( sane_stat == SANE_STATUS_GOOD )
      {
	 kDebug() << "Pre-Loop...";
	 kDebug() << "  format:          " << sane_scan_param.format;
	 kDebug() << "  last_frame:      " << sane_scan_param.last_frame;
	 kDebug() << "  lines:           " << sane_scan_param.lines;
	 kDebug() << "  depth:           " << sane_scan_param.depth;
	 kDebug() << "  pixels_per_line: " << sane_scan_param.pixels_per_line;
	 kDebug() << "  bytes_per_line:  " << sane_scan_param.bytes_per_line;

         if( sane_scan_param.pixels_per_line == 0 || sane_scan_param.lines < 1 )
         {
             kDebug() << "Error: Acquiring empty picture!";
             stat = KSCAN_ERR_EMPTY_PIC;
         }
      }
      else
      {
	 stat = KSCAN_ERR_OPEN_DEV;
	 kDebug() << "sane_get_parameters() error:" << sane_strstatus( sane_stat );
      }
   }
   else
   {
	 stat = KSCAN_ERR_OPEN_DEV;
	 kDebug() << "sane_start() error:" << sane_strstatus( sane_stat );
   }
   QApplication::restoreOverrideCursor();

   /* Create new Image from SANE-Parameters */
   if( stat == KSCAN_OK )
   {
      stat = createNewImage( &sane_scan_param );
   }

    if (stat==KSCAN_OK)
    {
        /* new buffer for scanning one line */
        if (mScanBuf!=NULL) delete [] mScanBuf;
        mScanBuf = new SANE_Byte[sane_scan_param.bytes_per_line+4];
        if (mScanBuf==NULL) stat = KSCAN_ERR_MEMORY;	// can this ever happen?
   }

   QApplication::setOverrideCursor(Qt::BusyCursor);	// display this while scanning
   /* Signal for a progress dialog */
   emit( sigScanProgress( 0 ) );
   emit( sigAcquireStart() );

   if( stat == KSCAN_OK )
   {
      /* initiates Redraw of the Progress-Window */
      qApp->processEvents();
   }

   if( stat == KSCAN_OK )
   {
      overall_bytes 	= 0;
      pixel_x 		    = 0;
      pixel_y 		    = 0;
      overall_bytes         = 0;
      rest_bytes            = 0;

      /* internal status to indicate Scanning in progress
       *  this status might be changed by pressing Stop on a GUI-Dialog displayed during scan */
      scanStatus = SSTAT_IN_PROGRESS;

      int fd = 0;
      //  Don't assume that sane_get_select_fd will succeed even if sane_set_io_mode
      //  successfully sets I/O mode to noblocking - bug 159300
      if (sane_set_io_mode(scanner_handle,SANE_TRUE)==SANE_STATUS_GOOD &&
          sane_get_select_fd(scanner_handle,&fd)==SANE_STATUS_GOOD)
      {
          mSocketNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
          connect(mSocketNotifier, SIGNAL(activated(int)), SLOT(doProcessABlock()));
      }
      else
      {
	 do
	 {
	    doProcessABlock();
	    if( scanStatus != SSTAT_SILENT )
	    {
	       sane_stat = sane_get_parameters( scanner_handle, &sane_scan_param );

               kDebug() << "Process a Block loop...";
               kDebug() << "  format:          " << sane_scan_param.format;
               kDebug() << "  last_frame:      " << sane_scan_param.last_frame;
               kDebug() << "  lines:           " << sane_scan_param.lines;
               kDebug() << "  depth:           " << sane_scan_param.depth;
               kDebug() << "  pixels_per_line: " << sane_scan_param.pixels_per_line;
               kDebug() << "  bytes_per_line:  " << sane_scan_param.bytes_per_line;
	    }
	 } while ( scanStatus != SSTAT_SILENT );
      }
   }

   if( stat != KSCAN_OK )
   {
      /* Scanning was disturbed in any way - end it */
      kDebug() << "Scanning was disturbed";
      emit( sigScanFinished( stat ));
   }
   return( stat );
}


void KScanDevice::getCurrentFormat(int *format,int *depth)
{
    sane_get_parameters(scanner_handle,&sane_scan_param );
    *format = sane_scan_param.format;
    *depth = sane_scan_param.depth;
}


void KScanDevice::loadOptionSet( KScanOptSet *optSet )
{
   if (optSet==NULL) return;

   kDebug() << "Loading option set" << optSet->optSetName() << "with" << optSet->count() << "options";

   Q3AsciiDictIterator<KScanOption> it(*optSet);
   while( it.current() )
   {
      KScanOption *so = it.current();
      if( ! so->initialised() )
	 kDebug() << "Option" << so->getName() << "is not initialised";

      if( ! so->active() )
	 kDebug() << "Option" << so->getName() << "is not active";

      if( so && so->active() && so->initialised())
      {
	 kDebug() << "Option" << so->getName() << "set to" << so->get();
	 apply( so );
      }
      ++it;
   }

}


void KScanDevice::slotScanFinished( KScanStat status )
{
    // clean up
    if (mSocketNotifier!=NULL)
    {
	mSocketNotifier->setEnabled(false);
	delete mSocketNotifier;
	mSocketNotifier = NULL;
    }

    emit( sigScanProgress( MAX_PROGRESS ));
    QApplication::restoreOverrideCursor();

    kDebug() << "status" <<  status;

    if (mScanBuf!=NULL)
    {
	delete[] mScanBuf;
	mScanBuf = NULL;
    }

    if( status == KSCAN_OK && mScanImage!=NULL)
    {
	ImgScanInfo info;
	info.setXResolution(d->currScanResolutionX);
	info.setYResolution(d->currScanResolutionY);
	info.setScannerName(shortScannerName());

	// put the resolution also into the image itself
	mScanImage->setDotsPerMeterX(static_cast<int>(d->currScanResolutionX / 0.0254 + 0.5));
	mScanImage->setDotsPerMeterY(static_cast<int>(d->currScanResolutionY / 0.0254 + 0.5));

	if( scanningPreview )
	{
	    savePreviewImage(*mScanImage);
	    emit( sigNewPreview( mScanImage, &info ));

	    /* The old settings need to be redefined */
	    loadOptionSet( storeOptions );
	}
	else
	{
	    emit( sigNewImage( mScanImage, &info ));
	}
    }


    sane_cancel(scanner_handle);

    /* This follows after sending the signal */
    if (mScanImage!=NULL)
    {
	delete mScanImage;
	mScanImage = NULL;
    }

    /* delete the socket notifier */
    if (mSocketNotifier!=NULL)
    {
	delete mSocketNotifier;
	mSocketNotifier = NULL;
    }
}


/* This function calls at least sane_read and converts the read data from the scanner
 * to the qimage.
 * The function needs:
 * QImage img valid
 * the data-buffer  set to a appropriate size
 **/

// TODO: probably needs to be extended for 16-bit scanner support

void KScanDevice::doProcessABlock()
{
    int val,i;
    QRgb col, newCol;

    SANE_Byte *rptr = NULL;
    SANE_Int bytes_read = 0;
    int chan = 0;
    sane_stat = SANE_STATUS_GOOD;
    uchar eight_pix = 0;

    while (true)
    {
        sane_stat = sane_read(scanner_handle,
                              (mScanBuf+rest_bytes),
                              sane_scan_param.bytes_per_line,
                              &bytes_read);
        int red = 0;
        int green = 0;
        int blue = 0;

        if (sane_stat!=SANE_STATUS_GOOD)
        {
            if (sane_stat!=SANE_STATUS_EOF)		// this is OK, just stop
            {						// any other error
                kDebug() << "sane_read() error:" << sane_strstatus(sane_stat)
                         << "bytes read" << bytes_read;
            }

            break;
        }

        if (bytes_read<1) break;			// no data, finish loop

	overall_bytes += bytes_read;
	// qDebug( "Bytes read: %d, bytes written: %d", bytes_read, rest_bytes );

	rptr = mScanBuf;				// start of scan data
	switch (sane_scan_param.format)
	{
case SANE_FRAME_RGB:
            if (sane_scan_param.lines<1) break;
            bytes_read += rest_bytes;			// die übergebliebenen Bytes dazu
            rest_bytes = bytes_read % 3;

            for (val = 0; val<((bytes_read-rest_bytes)/3); val++)
            {
                red   = *rptr++;
                green = *rptr++;
                blue  = *rptr++;

                if (pixel_x>=sane_scan_param.pixels_per_line)
                {					// reached end of a row
                    pixel_x = 0;
                    pixel_y++;
                }
                if (pixel_y<mScanImage->height())	// within image height
                {
		    mScanImage->setPixel(pixel_x, pixel_y, qRgb(red, green, blue));
                }
                pixel_x++;
            }

            for (val = 0; val<rest_bytes; val++)	// Copy the remaining bytes down
            {						// to the beginning of the block
                *(mScanBuf+val) = *rptr++;
            }
            break;

case SANE_FRAME_GRAY:
            for (val = 0; val<bytes_read ; val++)
            {
                if (pixel_y>=sane_scan_param.lines) break;
                if (sane_scan_param.depth==8)		// Greyscale
                {
		    if (pixel_x>=sane_scan_param.pixels_per_line)
                    {					// reached end of a row
                        pixel_x = 0;
                        pixel_y++;
                    }
		    mScanImage->setPixel(pixel_x, pixel_y, *rptr++);
		    pixel_x++;
                }
                else					// Lineart (bitmap)
                {					// needs to be converted to byte
		    eight_pix = *rptr++;
		    for (i = 0; i<8; i++)
		    {
                        if (pixel_y<sane_scan_param.lines)
                        {
                            chan = (eight_pix & 0x80)>0 ? 0 : 1;
                            eight_pix = eight_pix << 1;
                            mScanImage->setPixel(pixel_x, pixel_y, chan);
                            pixel_x++;
                            if( pixel_x>=sane_scan_param.pixels_per_line)
                            {
                                pixel_x = 0;
                                pixel_y++;
                                break;
                            }
                        }
		    }
                }
            }
            break;

case SANE_FRAME_RED:
case SANE_FRAME_GREEN:
case SANE_FRAME_BLUE:
            for (val = 0; val<bytes_read ; val++)
            {
                if (pixel_x>=sane_scan_param.pixels_per_line)
                {					// reached end of a row
                    pixel_x = 0;
		    pixel_y++;
                }

                if (pixel_y<sane_scan_param.lines)
                {
		    col = mScanImage->pixel(pixel_x, pixel_y);

		    red   = qRed(col);
		    green = qGreen(col);
		    blue  = qBlue(col);
		    chan  = *rptr++;

		    switch (sane_scan_param.format)
		    {
case SANE_FRAME_RED:    newCol = qRgba(chan, green, blue, 0xFF);
                        break;

case SANE_FRAME_GREEN:  newCol = qRgba(red, chan, blue, 0xFF);
                        break;

case SANE_FRAME_BLUE:   newCol = qRgba(red, green, chan, 0xFF);
                        break;

default:                newCol = qRgba(0xFF, 0xFF, 0xFF, 0xFF);
                        break;
		    }
		    mScanImage->setPixel(pixel_x, pixel_y, newCol);
		    pixel_x++;
                }
            }
            break;

default:    kDebug() << "Undefined sane_scan_param format" << sane_scan_param.format;
            break;
	}						// switch of scan format

	if ((sane_scan_param.lines>0) && ((sane_scan_param.lines*pixel_y)>0))
	{
            int progress =  (int)(((double)MAX_PROGRESS)/sane_scan_param.lines*pixel_y);
            if (progress<MAX_PROGRESS) emit sigScanProgress(progress);
	}

        // cannot get here, bytes_read and EOF tested above
	//if( bytes_read == 0 || sane_stat == SANE_STATUS_EOF )
	//{
	//   kDebug() << "sane_stat not OK:" << sane_stat;
	//   break;
	//}

        if (scanStatus==SSTAT_STOP_NOW)
        {
            /* scanStatus is set to SSTAT_STOP_NOW due to hitting slStopScanning   */
            /* Mostly that one is fired by the STOP-Button in the progress dialog. */

            /* This is also hit after the normal finish of the scan. Most probably,
             * the QSocketnotifier fires for a few times after the scan has been
             * cancelled.  Does it matter ? To see it, just uncomment the qDebug msg.
             */
            kDebug() << "Stopping the scan progress";
            scanStatus = SSTAT_SILENT;
            emit sigScanFinished(KSCAN_OK);
            break;
        }
    }							// end of main loop


    // Here when scanning is finished or has had an error
    if (sane_stat==SANE_STATUS_EOF)			// end of scan pass
    {
        if (sane_scan_param.last_frame)			// end of scanning run
        {
            /** Everythings okay, the picture is ready **/
            kDebug() << "Last frame reached, scan successful";
            scanStatus = SSTAT_SILENT;
            emit sigScanFinished(KSCAN_OK);
        }
        else
        {
            /** EOF und nicht letzter Frame -> Parameter neu belegen und neu starten **/
            scanStatus = SSTAT_NEXT_FRAME;
            kDebug() << "EOF, but another frame to scan";
        }
    }

    if (sane_stat==SANE_STATUS_CANCELLED)
    {
        scanStatus = SSTAT_STOP_NOW;
        kDebug() << "Scan was cancelled";
    }
}


// TODO: does this need to be a slot?
void KScanDevice::slotSaveScanConfigSet(const QString &setName,const QString &descr)
{
    if (setName.isEmpty()) return;
    kDebug() << k_funcinfo << "Saving configuration" << setName;

    KScanOptSet optSet(DEFAULT_OPTIONSET);
    getCurrentOptions(&optSet);
    optSet.saveConfig(scanner_name,setName,descr);
}


void KScanDevice::getCurrentOptions(KScanOptSet *optSet)
{
    if (optSet==NULL) return;

    for (QList<KScanOption *>::const_iterator it = gui_elements.constBegin();
         it!=gui_elements.constEnd(); ++it)
    {
        KScanOption *so = (*it);
        if (so==NULL) continue;

        kDebug() << "Storing" << so->getName();
        if (so->active())
        {
            apply(so);
            optSet->backupOption(*so);
        }

        /* drop the thing from the dirty-list */
        dirtyList.removeOne(so->getName());
    }

    for (QList<QByteArray>::const_iterator it = dirtyList.constBegin();
         it!=dirtyList.constEnd(); ++it)
    {
        KScanOption so(*it);
        optSet->backupOption(so);
    }
}


QString KScanDevice::getConfig(const QString &key, const QString &def) const
{
    QString scannerName = shortScannerName();

    const KConfig scanConfig(SCANNER_DB_FILE, KConfig::SimpleConfig);
    const KConfigGroup grp = scanConfig.group(scannerName);
    return (grp.readEntry(key, def));
}


// TODO: does this need to be a slot?
void KScanDevice::slotStoreConfig(const QString &key, const QString &val)
{
    QString scannerName = shortScannerName();
    if (scannerName.isEmpty() || scannerName==UNDEF_SCANNERNAME)
    {
        kDebug() << "Skipping config write, scanner name is empty!";
        return;
    }

    kDebug() << "Storing config" << key << "in group" << scannerName;

    KConfig scanConfig(SCANNER_DB_FILE, KConfig::SimpleConfig);
    KConfigGroup grp = scanConfig.group(scannerName);
    grp.writeEntry( key, val );
    grp.sync();
}


bool KScanDevice::scanner_initialised = false;
SANE_Handle KScanDevice::scanner_handle = NULL;
SANE_Device const **KScanDevice::dev_list = NULL;
Q3AsciiDict<int> *KScanDevice::option_dic = NULL;
KScanOptSet *KScanDevice::gammaTables = NULL;
