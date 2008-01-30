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

#include <stdlib.h>
#include <qwidget.h>
#include <qobject.h>
#include <qasciidict.h>
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
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kstandarddirs.h>

#include <unistd.h>
#include "kgammatable.h"
#include "kscandevice.h"
#include "kscanslider.h"
#include "kscanoption.h"
#include "kscanoptset.h"
#include "devselector.h"
#include "imgscaninfo.h"

#include <ksimpleconfig.h>

#define MIN_PREVIEW_DPI 75
#define UNDEF_SCANNERNAME I18N_NOOP( "undefined" )
#define MAX_PROGRESS 100

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
void KScanDevice::guiSetEnabled( const QCString& name, bool state )
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
KScanOption *KScanDevice::getExistingGuiElement( const QCString& name )
{
    KScanOption *ret = 0L;

    QCString alias = aliasName( name );

    /* gui_elements is a QList<KScanOption> */
    for( ret = gui_elements.first(); ret != 0; ret = gui_elements.next())
    {
       if( ret->getName() == alias ) break;
    }

    return( ret );
}
/* ---------------------------------------------------------------------------

   ------------------------------------------------------------------------- */

KScanOption *KScanDevice::getGuiElement( const QCString& name, QWidget *parent,
					 const QString& desc,
					 const QString& tooltip )
{
   if( name.isEmpty() ) return(0);
   QWidget *w = 0;
   KScanOption *so = 0;

   QCString alias = aliasName( name );

   /* Check if already exists */
   so = getExistingGuiElement( name );

   if( so ) return( so );

   /* ...else create a new one */
   so = new KScanOption( alias );

   if( so->valid() && so->softwareSetable())
   {
      /** store new gui-elem in list of all gui-elements */
      gui_elements.append( so );

      w = so->createWidget( parent, desc, tooltip );
      if( w )
      {
	 connect( so,   SIGNAL( optionChanged( KScanOption* ) ),
		  this, SLOT(   slOptChanged( KScanOption* )));
	 w->setEnabled( so->active() );
      }
      else
      {
	 kdDebug(29000) << "ERROR: No widget created for " << name << endl;
      }
   }
   else
   {
      if( !so->valid())
	 kdDebug(29000) << "getGuiElem: no option <" << alias << ">" << endl;
      else
      if( !so->softwareSetable())
	 kdDebug(29000) << "getGuiElem: option <" << alias << "> is not software Setable" << endl;

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
    SANE_Status sane_stat = sane_init(NULL, NULL );

    d = new KScanDevicePrivate();
    
    option_dic = new QAsciiDict<int>;
    option_dic->setAutoDelete( true );
    gui_elements.setAutoDelete( true );

    scanner_initialised = false;  /* stays false until openDevice. */
    scanStatus = SSTAT_SILENT;

    data         = 0; /* temporary image data buffer while scanning */
    sn           = 0; /* socket notifier for async scanning         */
    img          = 0; /* temporary image to scan in                 */
    storeOptions = 0; /* list of options to store during preview    */
    overall_bytes = 0;
    rest_bytes = 0;
    pixel_x = 0;
    pixel_y = 0;
    scanner_name = 0L;

    KConfig *konf = KGlobal::config ();
    konf->setGroup( GROUP_STARTUP );
    bool netaccess = konf->readBoolEntry( STARTUP_ONLY_LOCAL, false );
    kdDebug(29000) << "Query for network scanners " << (netaccess ? "Not enabled" : "Enabled") << endl;
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
	    	kdDebug(29000) << "Found Scanner: " << dev_list[devno]->name << endl;
            }
        }
#if 0
        connect( this, SIGNAL(sigOptionsChanged()), SLOT(slReloadAll()));
#endif
	gammaTables = new KScanOptSet( "GammaTables" );
     }
     else
     {
        kdDebug(29000) << "ERROR: sane_init failed -> SANE installed ?" << endl;
     }

    connect( this, SIGNAL( sigScanFinished( KScanStat )), SLOT( slScanFinished( KScanStat )));

}


KScanDevice::~KScanDevice()
{
  if( storeOptions )  delete (storeOptions );
  kdDebug(29000) << "Calling sane_exit to finish sane!" << endl;
  sane_exit();
  delete d;
}


KScanStat KScanDevice::openDevice( const QCString& backend )
{
   KScanStat    stat      = KSCAN_OK;
   SANE_Status 	sane_stat = SANE_STATUS_GOOD;

   if( backend.isEmpty() ) return KSCAN_ERR_PARAM;

   // search for scanner
   int indx = scanner_avail.find( backend );

   if( indx < 0 ) {
      stat = KSCAN_ERR_NO_DEVICE;
   }

   // if available, build lists of properties
   if( stat == KSCAN_OK )
   {
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
   }

   if( stat == KSCAN_OK )
      scanner_initialised = true;

   return( stat );
}

void KScanDevice::slCloseDevice( )
{
   /* First of all, send a signal to close down the scanner dev. */
   emit( sigCloseDevice( ));

   kdDebug(29000) << "Saving scan settings" << endl;
   slSaveScanConfigSet( DEFAULT_OPTIONSET, i18n("the default startup setup"));

   /* After return, delete all related stuff */
   scanner_name = UNDEF_SCANNERNAME;
   if( scanner_handle )
   {
      if( scanStatus != SSTAT_SILENT )
      {
         kdDebug(29000) << "Scanner is still active, calling cancel !" << endl;
         sane_cancel( scanner_handle );
      }
      sane_close( scanner_handle );
      scanner_handle = 0;
   }

   gui_elements.clear();

   option_dic->clear();
   scanner_initialised = false;

}


QString KScanDevice::getScannerName(const QCString& name) const
{
  QString ret = i18n("No scanner selected");
  SANE_Device *scanner = 0L;

  if( scanner_name && scanner_initialised && name.isEmpty())
  {
     scanner = scannerDevices[ scanner_name ];
  }
  else if ( ! name.isEmpty() )
  {
     scanner = scannerDevices[ name ];
     ret = QString::null;
  }

  if( scanner ) {
     // ret.sprintf( "%s %s %s", scanner->vendor, scanner->model, scanner->type );
     ret.sprintf( "%s %s", scanner->vendor, scanner->model );
  }

  kdDebug(29000) << "getScannerName returns <" << ret << ">" << endl;
  return ( ret );
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

	if( d )
	{
	   // logOptions( d );
	   if(d->name )
	   {
	      // Die Option anhand des Namen in den Dict

	      if( strlen( d->name ) > 0 )
	      {
		 new_opt = new int;
		 *new_opt = i;
		 kdDebug(29000) << "Inserting <" << d->name << "> as " << *new_opt << endl;
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
		 kdDebug(29000) << "Unable to detect option " << endl;
	   }
	}
     }
  }
  return stat;
}


QStrList KScanDevice::getAllOptions()
{
   return( option_list );
}

QStrList KScanDevice::getCommonOptions()
{
   QStrList com_opt;

   QCString s = option_list.first();

   while( !s.isEmpty() )
   {
      KScanOption opt( s );
      if( opt.commonOption() )
	 com_opt.append( s );
      s = option_list.next();
   }
   return( com_opt );
}

QStrList KScanDevice::getAdvancedOptions()
{
   QStrList com_opt;

   QCString s = option_list.first();

   while( !s.isEmpty() )
   {
     KScanOption opt( s );
     if( !opt.commonOption() )
       com_opt.append( s );
     s = option_list.next();
   }
   return( com_opt );
}

KScanStat KScanDevice::apply( KScanOption *opt, bool isGammaTable )
{
   KScanStat   stat = KSCAN_OK;
   if( !opt ) return( KSCAN_ERR_PARAM );
   int sane_result = 0;

   int         *num = (*option_dic)[ opt->getName() ];
   SANE_Status sane_stat = SANE_STATUS_GOOD;
   const QCString& oname = opt->getName();

   if ( oname == "preview" || oname == "mode" ) {
      sane_stat = sane_control_option( scanner_handle, *num,
				       SANE_ACTION_SET_AUTO, 0,
				       &sane_result );
      /* No return here, please ! Carsten, does it still work than for you ? */
   }


   if( ! opt->initialised() || opt->getBuffer() == 0 )
   {
      kdDebug(29000) << "Attempt to set Zero buffer of " << oname << " -> skipping !" << endl;

      if( opt->autoSetable() )
      {
	 kdDebug(29000) << "Setting option automatic !" << endl;
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
	 kdDebug(29000) << "Option " << oname << " is not active now!" << endl;
	 stat = KSCAN_OPT_NOT_ACTIVE;
      }
      else if( ! opt->softwareSetable() )
      {
	 kdDebug(29000) << "Option " << oname << " is not software Setable!" << endl;
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
	 kdDebug(29000) << "Applied <" << oname << "> successfully" << endl;

	 if( sane_result & SANE_INFO_RELOAD_OPTIONS )
	 {
	    kdDebug(29000) << "* Setting status to reload options" << endl;
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
	    kdDebug(29000) << "Option <" << oname << "> was set inexact !" << endl;
	 }

	 /* if it is a gamma table, the gamma values must be stored */
	 if( isGammaTable )
	 {
	    gammaTables->backupOption( *opt );
	    kdDebug(29000) << "GammaTable stored: " << opt->getName() << endl;
	 }
      }
      else
      {
	 kdDebug(29000) << "Status of sane is bad: " << sane_strstatus( sane_stat )
			<< " for option " << oname << endl;

      }
   }
   else
   {
      kdDebug(29000) << "Setting of <" << oname << "> failed -> kscanerror." << endl;
   }

   if( stat == KSCAN_OK )
   {
      slSetDirty( oname );
   }

   return( stat );
}

bool KScanDevice::optionExists( const QCString& name )
{
   if( name.isEmpty() ) return false;
   int *i = 0L;

   QCString altname = aliasName( name );

   if( ! altname.isNull() )
       i = (*option_dic)[ altname ];

   if( !i )
       return( false );
   return( *i > -1 );
}

void KScanDevice::slSetDirty( const QCString& name )
{
   if( optionExists( name ) )
   {
      if( dirtyList.find( name ) == -1 )
      {
	 kdDebug(29000)<< "Setting dirty <" << name << ">" << endl;
	 /* item not found */
	 dirtyList.append( name );
      }
   }
}


/* This function tries to find name aliases which appear from backend to backend.
 *  Example: Custom-Gamma is for epson backends 'gamma-correction' - not a really
 *  cool thing :-|
 *  Maybe this helps us out ?
 */
QCString KScanDevice::aliasName( const QCString& name )
{
    int *i = (*option_dic)[ name ];
    QCString ret;

    if( i ) return( name );
    ret = name;

    if( name == SANE_NAME_CUSTOM_GAMMA )
    {
	if((*option_dic)["gamma-correction"])
	    ret = "gamma-correction";

    }

    if( ret != name )
	kdDebug( 29000) << "Found alias for <" << name << "> which is <" << ret << ">" << endl;

    return( ret );
}



/* Nothing to do yet. This Slot may get active to do same user Widget changes */
void KScanDevice::slOptChanged( KScanOption *opt )
{
    kdDebug(29000) << "Slot Option Changed for Option " << opt->getName() << endl;
    apply( opt );
}

/* This might result in a endless recursion ! */
void KScanDevice::slReloadAllBut( KScanOption *not_opt )
{
        if( ! not_opt )
        {
            kdDebug(29000) << "ReloadAllBut called with invalid argument" << endl;
            return;
        }

        /* Make sure its applied */
	apply( not_opt );

	kdDebug(29000) << "*** Reload of all except <" << not_opt->getName() << "> forced ! ***" << endl;

	for( KScanOption *so = gui_elements.first(); so; so = gui_elements.next())
	{
	    if( so != not_opt )
	    {
	       kdDebug(29000) << "Reloading <" << so->getName() << ">" << endl;
	 	so->slReload();
	 	so->slRedrawWidget(so);
	    }
	}
	kdDebug(29000) << "*** Reload of all finished ! ***" << endl;

}


/* This might result in a endless recursion ! */
void KScanDevice::slReloadAll( )
{
	kdDebug(29000) << "*** Reload of all forced ! ***" << endl;

	for( KScanOption *so = gui_elements.first(); so; so = gui_elements.next())
	{
	 	so->slReload();
	 	so->slRedrawWidget(so);
	}
}


void KScanDevice::slStopScanning( void )
{
    kdDebug(29000) << "Attempt to stop scanning" << endl;
    if( scanStatus == SSTAT_IN_PROGRESS )
    {
	emit( sigScanFinished( KSCAN_CANCELLED ));
    }
    scanStatus = SSTAT_STOP_NOW;
}


const QString KScanDevice::previewFile()
{
   QString dir = (KGlobal::dirs())->saveLocation( "data", "ScanImages", true );
   if( !dir.endsWith("/") )
      dir += "/";

   QString fname = dir + QString::fromLatin1(".previews/");
   QString sname( getScannerName(shortScannerName()) );
   sname.replace( '/', "_");

   return fname+sname;
}

QImage KScanDevice::loadPreviewImage()
{
   const QString prevFile = previewFile();
   kdDebug(29000) << "Loading preview from file " << prevFile << endl;

   QImage image;
   image.load( prevFile );

   return image;
}

bool KScanDevice::savePreviewImage( const QImage &image )
{
   if( image.isNull() )
	return false;

   const QString prevFile = previewFile();
   kdDebug(29000) << "Saving preview to file " << prevFile << endl;

   return image.save( prevFile, "BMP" );
}


KScanStat KScanDevice::acquirePreview( bool forceGray, int dpi )
{
   if( ! scanner_handle )
      return KSCAN_ERR_NO_DEVICE;

   double min, max, q;

   (void) forceGray;

   if( storeOptions )
      storeOptions->clear();
   else
      storeOptions = new KScanOptSet( "TempStore" );


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
	 kdDebug(29000) << "Setting GrayPreview ON" << endl;
       }
       else
       {
	 so->set(false );
	 kdDebug(29000) << "Setting GrayPreview OFF" << endl;
       }
     }
     apply( so );
   }


   if( optionExists( SANE_NAME_SCAN_MODE ) )
   {
      KScanOption mode( SANE_NAME_SCAN_MODE );
      const QString kk = mode.get();
      kdDebug(29000) << "Mode is <" << kk << ">" << endl;
      storeOptions->backupOption( mode );
      /* apply if it has a widget, or apply always ? */
      if( mode.widget() ) apply( &mode );
   }

   /** Scan Resolution should always exist. **/
   KScanOption res ( SANE_NAME_SCAN_RESOLUTION );
   const QString p = res.get();

   kdDebug(29000) << "Scan Resolution pre Preview is " << p << endl;
   storeOptions->backupOption( res );

   int set_dpi = dpi;

   if( dpi == 0 )
   {
       /* No resolution argument */
       if( ! res.getRange( &min, &max, &q ) )
       {
	   if( ! res.getRangeFromList ( &min, &max, &q ) )
	   {
	       kdDebug(29000) << "Could not retrieve resolution range!" << endl;
	       min = 75.0; // Hope that every scanner can 75
	   }
       }
       kdDebug(29000) << "Minimum Range: " << min << ", Maximum Range: " << max << endl;

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
#define NOTIFIER(X) optionNotifyString(X)

QString KScanDevice::optionNotifyString( int i ) const
{
    const QString sOff = "        |";
    const QString sOn  = "   X    |";   
    if( i > 0 )
    {
	return sOn;
    }
    return sOff;
}


void KScanDevice::prepareScan( void )
{
    QAsciiDictIterator<int> it( *option_dic ); // iterator for dict

    kdDebug(29000) << "########################################################################################################" << endl;
    kdDebug(29000) << "Scanner: " << scanner_name << endl;
    kdDebug(29000) << "         " << getScannerName() << endl;
    kdDebug(29000) << "----------------------------------+--------+--------+--------+--------+--------+--------+--------+" << endl;
    kdDebug(29000) << " Option-Name                      |SOFT_SEL|HARD_SEL|SOFT_DET|EMULATED|AUTOMATI|INACTIVE|ADVANCED|" << endl;
    kdDebug(29000) << "----------------------------------+--------+--------+--------+--------+--------+--------+--------+" << endl;

    while ( it.current() )
    {
       // qDebug( "%s -> %d", it.currentKey().latin1(), *it.current() );
       int descriptor = *it.current();

       const SANE_Option_Descriptor *d = sane_get_option_descriptor( scanner_handle, descriptor );

       if( d )
       {
	  int cap = d->cap;
	  
	  QString s = QString(it.currentKey()).leftJustify(32, ' ');
	  kdDebug(29000) << " " << s << " |" <<
		 NOTIFIER( ((cap) & SANE_CAP_SOFT_SELECT)) << 
		 NOTIFIER( ((cap) & SANE_CAP_HARD_SELECT)) << 
		 NOTIFIER( ((cap) & SANE_CAP_SOFT_DETECT)) << 
		 NOTIFIER( ((cap) & SANE_CAP_EMULATED)   ) << 
		 NOTIFIER( ((cap) & SANE_CAP_AUTOMATIC)  ) << 
		 NOTIFIER( ((cap) & SANE_CAP_INACTIVE)   ) << 
		 NOTIFIER( ((cap) & SANE_CAP_ADVANCED )  )    << endl;

       }
       ++it;
    }
    kdDebug(29000) << "----------------------------------+--------+--------+--------+--------+--------+--------+--------+" << endl;

    KScanOption pso( SANE_NAME_PREVIEW );
    const QString q = pso.get();

    kdDebug(29000) << "Preview-Switch is at the moment: " << q << endl;


}

/** Starts scanning
 *  depending on if a filename is given or not, the function tries to open
 *  the file using the Qt-Image-IO or really scans the image.
 **/
KScanStat KScanDevice::acquire( const QString& filename )
{
    if( ! scanner_handle )
       return KSCAN_ERR_NO_DEVICE;

    KScanOption *so = 0;

    if( filename.isEmpty() )
    {
 	/* *real* scanning - apply all Options and go for it */
 	prepareScan();

 	for( so = gui_elements.first(); so; so = gui_elements.next() )
 	{
 	    if( so->active() )
 	    {
 	         kdDebug(29000) << "apply <" << so->getName() << ">" << endl;
 	         apply( so );
 	    }
 	    else
 	    {
 	        kdDebug(29000) << "Option <" << so->getName() << "> is not active !" << endl;
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
    }

    return KSCAN_OK;
}


/**
 *  Creates a new QImage from the retrieved Image Options
 **/
KScanStat KScanDevice::createNewImage( SANE_Parameters *p )
{
  if( !p ) return( KSCAN_ERR_PARAM );
  KScanStat       stat = KSCAN_OK;

  if( img ) {
     delete( img );
     img = 0;
  }

  if( p->depth == 1 )  //  Lineart !!
  {
    img =  new QImage( p->pixels_per_line, p->lines, 8 );
    if( img )
    {
      img->setNumColors( 2 );
      img->setColor( 0, qRgb( 0,0,0));
      img->setColor( 1, qRgb( 255,255,255) );
    }
  }
  else if( p->depth == 8 )
  {
    // 8 bit rgb-Picture
    if( p->format == SANE_FRAME_GRAY )
    {
      /* Grayscale */
      img = new QImage( p->pixels_per_line, p->lines, 8 );
      if( img )
      {
	img->setNumColors( 256 );

	for(int i = 0; i<256; i++)
	  img->setColor(i, qRgb(i,i,i));
      }
    }
    else
    {
      /* true color image */
      img =  new QImage( p->pixels_per_line, p->lines, 32 );
      if( img )
	img->setAlphaBuffer( false );
    }
  }
  else
  {
    /* ERROR: NO OTHER DEPTHS supported */
    kdDebug(29000) << "KScan supports only bit dephts 1 and 8 yet!" << endl;
  }

  if( ! img ) stat = KSCAN_ERR_MEMORY;
  return( stat );
}


KScanStat KScanDevice::acquire_data( bool isPreview )
{
   SANE_Status	  sane_stat = SANE_STATUS_GOOD;
   KScanStat       stat = KSCAN_OK;

   scanningPreview = isPreview;

   emit sigScanStart();

   sane_stat = sane_start( scanner_handle );
   if( sane_stat == SANE_STATUS_GOOD )
   {
      sane_stat = sane_get_parameters( scanner_handle, &sane_scan_param );

      if( sane_stat == SANE_STATUS_GOOD )
      {
	 kdDebug(29000) << "--Pre-Loop" << endl;
	 kdDebug(29000) << "format : " << sane_scan_param.format << endl;
	 kdDebug(29000) << "last_frame : " << sane_scan_param.last_frame << endl;
	 kdDebug(29000) << "lines : " << sane_scan_param.lines << endl;
	 kdDebug(29000) << "depth : " << sane_scan_param.depth << endl;
	 kdDebug(29000) << "pixels_per_line : " << sane_scan_param.pixels_per_line << endl;
	 kdDebug(29000) << "bytes_per_line : " << sane_scan_param.bytes_per_line << endl;
      }
      else
      {
	 stat = KSCAN_ERR_OPEN_DEV;
	 kdDebug(29000) << "sane-get-parameters-Error: " << sane_strstatus( sane_stat ) << endl;
      }
   }
   else
   {
	 stat = KSCAN_ERR_OPEN_DEV;
	 kdDebug(29000) << "sane-start-Error: " << sane_strstatus( sane_stat ) << endl;
   }


   if( sane_scan_param.pixels_per_line == 0 || sane_scan_param.lines < 1 )
   {
      kdDebug(29000) << "ERROR: Acquiring empty picture !" << endl;
      stat = KSCAN_ERR_EMPTY_PIC;
   }

   /* Create new Image from SANE-Parameters */
   if( stat == KSCAN_OK )
   {
      stat = createNewImage( &sane_scan_param );
   }

   if( stat == KSCAN_OK )
   {
      /* new buffer for scanning one line */
      if(data) delete [] data;
      data = new SANE_Byte[ sane_scan_param.bytes_per_line +4 ];
      if( ! data ) stat = KSCAN_ERR_MEMORY;
   }

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
      scanStatus = SSTAT_IN_PROGRESS;
      pixel_x 		    = 0;
      pixel_y 		    = 0;
      overall_bytes         = 0;
      rest_bytes            = 0;

      /* internal status to indicate Scanning in progress
       *  this status might be changed by pressing Stop on a GUI-Dialog displayed during scan */
      if( sane_set_io_mode( scanner_handle, SANE_TRUE ) == SANE_STATUS_GOOD )
      {

	 int fd = 0;
	 if( sane_get_select_fd( scanner_handle, &fd ) == SANE_STATUS_GOOD )
	 {
	    sn = new QSocketNotifier( fd, QSocketNotifier::Read, this );
	    QObject::connect( sn, SIGNAL(activated(int)),
			      this, SLOT( doProcessABlock() ) );

	 }
      }
      else
      {
	 do
	 {
	    doProcessABlock();
	    if( scanStatus != SSTAT_SILENT )
	    {
	       sane_stat = sane_get_parameters( scanner_handle, &sane_scan_param );
	       kdDebug(29000) << "--ProcessABlock-Loop" << endl;
	       kdDebug(29000) << "format : " << sane_scan_param.format << endl;
	       kdDebug(29000) << "last_frame : " << sane_scan_param.last_frame << endl;
	       kdDebug(29000) << "lines : " << sane_scan_param.lines << endl;
	       kdDebug(29000) << "depth : " << sane_scan_param.depth << endl;
	       kdDebug(29000) << "pixels_per_line : " << sane_scan_param.pixels_per_line << endl;
	       kdDebug(29000) << "bytes_per_line : " << sane_scan_param.bytes_per_line << endl;
	    }
	 } while ( scanStatus != SSTAT_SILENT );
      }
   }

   if( stat != KSCAN_OK )
   {
      /* Scanning was disturbed in any way - end it */
      kdDebug(29000) << "Scanning was disturbed - clean up" << endl;
      emit( sigScanFinished( stat ));
   }
   return( stat );
}

void KScanDevice::loadOptionSet( KScanOptSet *optSet )
{
   if( !optSet )
   {
      return;
   }

   // kdDebug(29000) << "Loading Option set: " << optSet->optSetName() << endl;
   QAsciiDictIterator<KScanOption> it(*optSet);

   kdDebug(29000) << "Postinstalling " << optSet->count() << " options" << endl;
   while( it.current() )
   {
      KScanOption *so = it.current();
      if( ! so->initialised() )
	 kdDebug(29000) << so->getName() << " is not initialised" << endl;

      if( ! so->active() )
	 kdDebug(29000) << so->getName() << " is not active" << endl;

      if( so && so->active() && so->initialised())
      {
	 const QString qq = so->get();

	 kdDebug(29000) << "Post-Scan-apply <" << it.currentKey() << ">: " << qq << endl;
	 apply( so );
      }
      ++it;
   }

}


void KScanDevice::slScanFinished( KScanStat status )
{
    // clean up
    if( sn ) {
	sn->setEnabled( false );
	delete sn;
	sn = 0;
    }

    emit( sigScanProgress( MAX_PROGRESS ));

    kdDebug(29000) << "Slot ScanFinished hit with status " <<  status << endl;

    if( data )
    {
	delete[] data;
	data = 0;
    }

    if( status == KSCAN_OK && img )
    {
	ImgScanInfo info;
	info.setXResolution(d->currScanResolutionX);
	info.setYResolution(d->currScanResolutionY);
	info.setScannerName(shortScannerName());

	// put the resolution also into the image itself
	img->setDotsPerMeterX(static_cast<int>(d->currScanResolutionX / 0.0254 + 0.5));
	img->setDotsPerMeterY(static_cast<int>(d->currScanResolutionY / 0.0254 + 0.5));
	kdDebug(29000) << "resolutionX:" << d->currScanResolutionX << " resolutionY:" << d->currScanResolutionY << endl;

	if( scanningPreview )
	{
	    kdDebug(29000) << "Scanning a preview !" << endl;
	    savePreviewImage(*img);
	    emit( sigNewPreview( img, &info ));

	    /* The old settings need to be redefined */
	    loadOptionSet( storeOptions );
	}
	else
	{
	    emit( sigNewImage( img, &info ));
	}
    }


    sane_cancel(scanner_handle);

    /* This follows after sending the signal */
    if( img )
    {
	delete img;
	img = 0;
    }

    /* delete the socket notifier */
    if( sn ) {
	delete( sn );
	sn = 0;
    }

}


/* This function calls at least sane_read and converts the read data from the scanner
 * to the qimage.
 * The function needs:
 * QImage img valid
 * the data-buffer  set to a appropriate size
 **/

void KScanDevice::doProcessABlock( void )
{
  int 		  val,i;
  QRgb      	  col, newCol;

  SANE_Byte	  *rptr         = 0;
  SANE_Int	  bytes_written = 0;
  int		  chan          = 0;
  SANE_Status     sane_stat     = SANE_STATUS_GOOD;
  uchar	          eight_pix     = 0;
  bool 	  goOn = true;

  // int 	rest_bytes = 0;
  while( goOn && data )
  {
     sane_stat =
	sane_read( scanner_handle, data + rest_bytes, sane_scan_param.bytes_per_line, &bytes_written);
     int	red = 0;
     int       green = 0;
     int       blue = 0;

     if( sane_stat != SANE_STATUS_GOOD )
     {
	/** any other error **/
	kdDebug(29000) << "sane_read returned with error <" << sane_strstatus( sane_stat ) << ">: " << bytes_written << " bytes left" << endl;
	goOn = false;
     }

     if( goOn && bytes_written < 1 )
     {
	// qDebug( "No bytes written -> leaving out !" );
	goOn = false;  /** No more data -> leave ! **/
     }

     if( goOn )
     {
	overall_bytes += bytes_written;
	// qDebug( "Bytes read: %d, bytes written: %d", bytes_written, rest_bytes );

	rptr = data; // + rest_bytes;

	switch( sane_scan_param.format )
	{
	   case SANE_FRAME_RGB:
	      if( sane_scan_param.lines < 1 ) break;
	      bytes_written += rest_bytes; // die übergebliebenen Bytes dazu
	      rest_bytes = bytes_written % 3;

	      for( val = 0; val < ((bytes_written-rest_bytes) / 3); val++ )
	      {
		 red   = *rptr++;
		 green = *rptr++;
		 blue  = *rptr++;

		 // kdDebug(29000) <<  "RGB: %d, %d, %d\n", red, green, blue" << endl;
		 if( pixel_x  == sane_scan_param.pixels_per_line )
		 { pixel_x = 0; pixel_y++; }
		 if( pixel_y < img->height())
		    img->setPixel( pixel_x, pixel_y, qRgb( red, green,blue ));

		 pixel_x++;
	      }
	      /* copy the remaining bytes to the beginning of the block :-/ */
	      for( val = 0; val < rest_bytes; val++ )
	      {
		 *(data+val) = *rptr++;
	      }

	      break;
	   case SANE_FRAME_GRAY:
	      for( val = 0; val < bytes_written ; val++ )
	      {
		 if( pixel_y >= sane_scan_param.lines ) break;
		 if( sane_scan_param.depth == 8 )
		 {
		    if( pixel_x  == sane_scan_param.pixels_per_line ) { pixel_x = 0; pixel_y++; }
		    img->setPixel( pixel_x, pixel_y, *rptr++ );
		    pixel_x++;
		 }
		 else
		 {  // Lineart
		    /* Lineart needs to be converted to byte */
		    eight_pix = *rptr++;
		    for( i = 0; i < 8; i++ )
		    {
		       if( pixel_y < sane_scan_param.lines )
		       {
			  chan = (eight_pix & 0x80)> 0 ? 0:1;
			  eight_pix = eight_pix << 1;
			    	//qDebug( "Setting on %d, %d: %d", pixel_x, pixel_y, chan );
			  img->setPixel( pixel_x, pixel_y, chan );
			  pixel_x++;
			  if( pixel_x  >= sane_scan_param.pixels_per_line )
			  {
			     pixel_x = 0; pixel_y++;
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
	      kdDebug(29000) << "Scanning Single color Frame: " << bytes_written << " Bytes!" << endl;
	      for( val = 0; val < bytes_written ; val++ )
	      {
		 if( pixel_x >= sane_scan_param.pixels_per_line )
		 {
		    pixel_y++; pixel_x = 0;
		 }

		 if( pixel_y < sane_scan_param.lines )
		 {

		    col = img->pixel( pixel_x, pixel_y );

		    red   = qRed( col );
		    green = qGreen( col );
		    blue  = qBlue( col );
		    chan  = *rptr++;

		    switch( sane_scan_param.format )
		    {
		       case SANE_FRAME_RED :
			  newCol = qRgba( chan, green, blue, 0xFF );
			  break;
		       case SANE_FRAME_GREEN :
			  newCol = qRgba( red, chan, blue, 0xFF );
			  break;
		       case SANE_FRAME_BLUE :
			  newCol = qRgba( red , green, chan, 0xFF );
			  break;
		       default:
			  kdDebug(29000) << "Undefined format !" << endl;
			  newCol = qRgba( 0xFF, 0xFF, 0xFF, 0xFF );
			  break;
		    }
		    img->setPixel( pixel_x, pixel_y, newCol );
		    pixel_x++;
		 }
	      }
	      break;
	   default:
	      kdDebug(29000) << "Unexpected ERROR: No Format type" << endl;
	      break;
	} /* switch */


	if( (sane_scan_param.lines > 0) && (sane_scan_param.lines * pixel_y> 0) )
	{
	   int progress =  (int)(((double)MAX_PROGRESS) / sane_scan_param.lines *
				  pixel_y);
	   if( progress < MAX_PROGRESS)
	      emit( sigScanProgress( progress));
	}

	if( bytes_written == 0 || sane_stat == SANE_STATUS_EOF )
	{
	   kdDebug(29000) << "Down under sane_stat not OK" << endl;
	   goOn = false;
	}
     }

     if( goOn && scanStatus == SSTAT_STOP_NOW )
     {
	/* scanStatus is set to SSTAT_STOP_NOW due to hitting slStopScanning   */
	/* Mostly that one is fired by the STOP-Button in the progress dialog. */

	/* This is also hit after the normal finish of the scan. Most probably,
	 * the QSocketnotifier fires for a few times after the scan has been
	 * cancelled.  Does it matter ? To see it, just uncomment the qDebug msg.
	 */
	kdDebug(29000) << "Stopping the scan progress !" << endl;
	goOn = false;
	scanStatus = SSTAT_SILENT;
	emit( sigScanFinished( KSCAN_OK ));
     }

  } /* while( 1 ) */


  /** Comes here if scanning is finished or has an error **/
  if( sane_stat == SANE_STATUS_EOF)
  {
     if ( sane_scan_param.last_frame)
     {
	/** Everythings okay, the picture is ready **/
	kdDebug(29000) << "last frame reached - scan successful" << endl;
	scanStatus = SSTAT_SILENT;
	emit( sigScanFinished( KSCAN_OK ));
     }
     else
     {
	/** EOF und nicht letzter Frame -> Parameter neu belegen und neu starten **/
	scanStatus = SSTAT_NEXT_FRAME;
	kdDebug(29000) << "EOF, but another frame to scan" << endl;

     }
  }

  if( sane_stat == SANE_STATUS_CANCELLED )
  {
     scanStatus = SSTAT_STOP_NOW;
     kdDebug(29000) << "Scan was cancelled" << endl;

     // stat = KSCAN_CANCELLED;
     // emit( sigScanFinished( stat ));

     /* hmmm - how could this happen ? */
  }
} /* end of fkt */


void KScanDevice::slSaveScanConfigSet( const QString& setName, const QString& descr )
{
   if( setName.isEmpty() || setName.isNull()) return;

   kdDebug(29000) << "Saving Scan Configuration" << setName << endl;

   KScanOptSet optSet( DEFAULT_OPTIONSET );
   getCurrentOptions( &optSet );

   optSet.saveConfig( scanner_name , setName, descr );

}


void  KScanDevice::getCurrentOptions( KScanOptSet *optSet )
{
   if( ! optSet ) return;

   for( KScanOption *so = gui_elements.first(); so; so = gui_elements.next())
   {
      kdDebug(29000) << "Storing <" << so->getName() << ">" << endl;
      if( so && so->active())
      {
	 apply(so);
	 optSet->backupOption( *so );
      }

      /* drop the thing from the dirty-list */
      dirtyList.removeRef( so->getName());
   }

   QStrListIterator it( dirtyList );
   while( it.current())
   {
      KScanOption so( it.current() );
      optSet->backupOption( so );
      ++it;
   }
}

QString KScanDevice::getConfig( const QString& key, const QString& def ) const
{
    QString confFile = SCANNER_DB_FILE;

    KSimpleConfig scanConfig( confFile, true );
    scanConfig.setGroup( shortScannerName() );

    return scanConfig.readEntry( key, def );
}

void KScanDevice::slStoreConfig( const QString& key, const QString& val )
{
    QString confFile = SCANNER_DB_FILE;
    QString scannerName = shortScannerName();

    if( scannerName.isEmpty() || scannerName == UNDEF_SCANNERNAME )
    {
        kdDebug(29000) << "Skipping config write, scanner name is empty!" << endl;
    }
    else
    {
        kdDebug(29000) << "Storing config " << key << " in Group " << scannerName << endl;

        KSimpleConfig scanConfig( confFile );
        scanConfig.setGroup( scannerName );
        scanConfig.writeEntry( key, val );
        scanConfig.sync();
    }
}


bool KScanDevice::scanner_initialised = false;
SANE_Handle KScanDevice::scanner_handle = 0L;
SANE_Device const **KScanDevice::dev_list = 0L;
QAsciiDict<int> *KScanDevice::option_dic = 0L;
KScanOptSet *KScanDevice::gammaTables = 0L;


#include "kscandevice.moc"
