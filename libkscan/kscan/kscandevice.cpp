
/** HEADER **/


#include <stdlib.h>
#include <qwidget.h>
#include <qobject.h>
#include <qdict.h>
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

#include <unistd.h>
#include "kgammatable.h"
#include "kscandevice.h"
#include "kscanslider.h"
#include "kscanoption.h"
#include "kscanoptset.h"

#define MIN_PREVIEW_DPI 20


// global device list

/** These variables are defined in ksaneoption.cpp **/
extern bool        scanner_initialised;
extern SANE_Handle scanner_handle;
extern QDict<int>  option_dic;

extern SANE_Device const **dev_list          = 0;

KScanOptSet gammaTables("GammaTables");


/* ---------------------------------------------------------------------------

   ------------------------------------------------------------------------- */
bool KScanDevice::guiSetEnabled( QString name, bool state )
{
 	
  KScanOption *so;
  
   if( gui_elem_names[ name ] )
    {
      so = gui_elem_names[name];
      QWidget *w = so->widget();
      
      if( w )
	w->setEnabled( state );
    }
}

/* ---------------------------------------------------------------------------

   ------------------------------------------------------------------------- */

KScanOption *KScanDevice::getGuiElement( const char *name, QWidget *parent,
					 const char *desc, const char *tooltip )
{
   if( ! name ) return(0);
   QWidget *w = 0;
   KScanOption *so = 0;

   /* Check if already exists */
   so = gui_elem_names[ name ];

   if( so ) return( so );

   /* ...else create a new one */
   so = new KScanOption( name );

   if( so->valid() && so->softwareSetable())
   {
      /** store new gui-elem in list of all gui-elements */
      gui_elements.append( so );
      gui_elem_names.insert( name, so );
	 	
      w = so->createWidget( parent, desc, tooltip );
      if( w )
	 connect( so,   SIGNAL( optionChanged( KScanOption* ) ),
		  this, SLOT(   slOptChanged( KScanOption* )));	
   }
   else
   {
      if( !so->valid())
	 debug( "getGuiElem: no option <%s>", name );
      else
      if( !so->softwareSetable())
	 debug( "getGuiElem: option <%s> is not software Setable", name );

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
    option_dic.setAutoDelete( true );
    gui_elements.setAutoDelete( true );

    scanner_initialised = false;  /* stays false until openDevice. */
    scanStatus = SSTAT_SILENT;

    data         = 0; /* temporar image data buffer while scanning */
    sn           = 0; /* socket notifier for async scanning        */
    img          = 0; /* temporar image to scan in                 */
    storeOptions = 0; /* list of options to store during preview   */
    overall_bytes = 0;
    rest_bytes = 0;
    pixel_x = 0;
    pixel_y = 0;
    

    if( sane_stat == SANE_STATUS_GOOD )
    {
        sane_stat = sane_get_devices( &dev_list, SANE_TRUE );

        // NO network devices yet

        // Store all available Scanner to Stringlist
        for( int devno = 0; sane_stat == SANE_STATUS_GOOD &&
	      dev_list[ devno ]; ++devno )
        {
	    if( dev_list[devno] )
 	    {
	        scanner_avail.append( dev_list[devno]->name );
		scannerDevices.insert( dev_list[devno]->name, dev_list[devno] );
	    	debug( "Found Scanner: %s", dev_list[devno]->name );
            }
        }
#if 0
        connect( this, SIGNAL(sigOptionsChanged()), SLOT(slReloadAll()));
#endif
     }
     else
     {
        debug( "ERROR: sane_init failed -> SANE installed ? ");
     }

    connect( this, SIGNAL( sigScanFinished( KScanStat )), SLOT( slScanFinished( KScanStat )));

}


KScanDevice::~KScanDevice()
{
  if( storeOptions )  delete (storeOptions );
}


KScanStat KScanDevice::openDevice( const char* backend )
{
   KScanStat    stat      = KSCAN_OK;
   SANE_Status 	sane_stat = SANE_STATUS_GOOD;

   if( ! backend ) return KSCAN_ERR_PARAM;

   // search for scanner
   int indx = scanner_avail.find( backend );

   if( indx > -1 ) {
      QString scanner = scanner_avail.at( indx );
   } else {
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
	scanner_name = "undefined";
      }
   }

   if( stat == KSCAN_OK )
      scanner_initialised = true;

   return( stat );
}


QString KScanDevice::getScannerName(QString name) const
{
  QString ret = "No scanner selected";
  SANE_Device *scanner = 0L;
  
  if( scanner_initialised && name.isEmpty())
  {
     scanner = scannerDevices[ scanner_name ];
  }
  else
  {
     scanner = scannerDevices[ name ];
     ret = "";
  }
     
  if( scanner ) {
     ret.sprintf( "%s %s %s", scanner->vendor, scanner->model, scanner->type );
  }		

  debug( "getScannerName returns <%s>", (const char*) ret );
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

     option_dic.clear();
     
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
		 debug( "Inserting <%s> as %d", d->name, *new_opt );
		 /* create a new option in the set. */
		 option_dic.insert ( (const char*) d->name, new_opt );
		 option_list.append( (const char*) d->name );
#if 0
		 KScanOption *newOpt = new KScanOption( d->name );
		 const QString qq = newOpt->get();
		 debug( "INIT: <%s> = <%s>", d->name, (const char*) qq );
		 allOptionSet->insert( d->name, newOpt );
#endif		 
	      }
	      else if( d->type == SANE_TYPE_GROUP )
	      {
	      	// debug( "######### Group found: %s ########", d->title );
	      }
	      else
		 debug( "Unable to detect option " );
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

   QString s = option_list.first();

   while( !(s.isEmpty() || s.isNull()) )
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

   QString s = option_list.first();

   while( !(s.isEmpty() || s.isNull()) )
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

   int         *num = option_dic[ opt->getName() ];
   SANE_Int    result = 0;
   SANE_Status sane_stat = SANE_STATUS_GOOD;
   const char  *oname = (const char*) opt->getName();
   
   if ( opt->getName() == "preview" || opt->getName() == "mode" ) {
      sane_stat = sane_control_option( scanner_handle, *num,
				       SANE_ACTION_SET_AUTO, 0,
				       &result );
      /* No return here, please ! Carsten, does it still work than for you ? */
   }


   if( ! opt->initialised() || opt->getBuffer() == 0 )
   {
     debug( "Attempt to set Zero buffer of %s -> skipping !", oname );
   	
      if( opt->autoSetable() )
      {
	 debug("Setting option automatic !" );
	 sane_stat = sane_control_option( scanner_handle, *num,
					  SANE_ACTION_SET_AUTO, 0,
					  &result );
      }
      else
      {
	 sane_stat = SANE_STATUS_INVAL;
      }
   }
   else
   {	
      if( ! opt->active() )
      {
	 debug( "Option %s is not active now!", oname );
	 result = 0;
      }
      else if( ! opt->softwareSetable() )
      {
	 debug( "Option %s is not software Setable!", oname );
	 result = 0;
      }
      else
      {
	 
	 sane_stat = sane_control_option( scanner_handle, *num,
					  SANE_ACTION_SET_VALUE,
					  opt->getBuffer(),
					  &result );
      }
   }		

   if( sane_stat == SANE_STATUS_GOOD )
   {
     debug( "Applied <%s> successfully", oname );
     
      if( result & SANE_INFO_RELOAD_OPTIONS )
      {
	 debug( "* Setting status to reload options");
	 stat = KSCAN_RELOAD;
#if 0
	 debug( "Emitting sigOptionChanged()" );
	 emit( sigOptionsChanged() );
#endif
      }

#if 0
      if( result & SANE_INFO_RELOAD_PARAMS )
	 emit( sigScanParamsChanged() );
#endif
      if( result & SANE_INFO_INEXACT )
      {
	debug( "Option <%s> was set inexact !", oname );
		
      }

      /* if it is a gamma table, the gamma values must be stored */
      if( isGammaTable ) 
      {
	 gammaTables.backupOption( *opt );
	 debug( "GammaTable stored: %s", (const char*) opt->getName());
      }
   }
   else
   {
     debug( "Setting of <%s> failed: %s", oname, sane_strstatus( sane_stat ) );
      stat = KSCAN_ERR_CONTROL;
   }
   return( stat );
}

bool KScanDevice::optionExists( const char *name )
{
   if( ! name ) return( 0 );
   int *i = option_dic[name];

   if( !i ) return( false );
   return( *i > -1 ? true : false );
}



/* Nothing to do yet. This Slot may get active to do same user Widget changes */
void KScanDevice::slOptChanged( KScanOption *opt )
{
    debug( "Slot Option Changed for Option " + opt->getName() );
    apply( opt );
}

/* This might result in a endless recursion ! */
void KScanDevice::slReloadAllBut( KScanOption *not_opt )
{
        if( ! not_opt )
        {
            debug( "ReloadAllBut called with invalid argument" );
            return;
        }        	

        /* Make sure its applied */
	apply( not_opt );

	debug( "*** Reload of all except <%s> forced ! ***",
	       (const char*) not_opt->getName() );
	
	for( KScanOption *so = gui_elements.first(); so; so = gui_elements.next())
	{
	    if( so != not_opt )
	    {
	       debug( "Reloading <%s>", (const char*) so->getName());
	 	so->slReload();
	 	so->slRedrawWidget(so);
	    }
	}
	debug( "*** Reload of all finished ! ***" );
		       
}


/* This might result in a endless recursion ! */
void KScanDevice::slReloadAll( )
{
	debug( "*** Reload of all forced ! ***" );
	
	for( KScanOption *so = gui_elements.first(); so; so = gui_elements.next())
	{
	 	so->slReload();
	 	so->slRedrawWidget(so);
	}
}


void KScanDevice::slStopScanning( void )
{
    debug( "Attempt to stop scanning" );
    if( scanStatus == SSTAT_IN_PROGRESS )
    {
    	scanStatus = SSTAT_STOP_NOW;
	emit( sigScanFinished( KSCAN_CANCELLED ));
    }
}

KScanStat KScanDevice::acquirePreview( bool forceGray, int dpi )
{
   double min, max, q;

   (void) forceGray;
         
   
#if 0
   QAsciiDictIterator<KScanOption> it(*standardOptions);

   while( it.current() )
   {
      so = it.current();
      if( so && so->active() && so->initialised())
      {
	 debug( "apply <%s>", (const char*) it.currentKey() );
	 apply( so );
      }
      ++it;
   }
#endif
   if( storeOptions )
      storeOptions->clear();
   else
      storeOptions = new KScanOptSet( "TempStore" );


   /* set Preview = ON if exists */
   if( optionExists( SANE_NAME_PREVIEW ) )
   {
      KScanOption prev( SANE_NAME_PREVIEW );
      
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
     KScanOption *so = gui_elem_names[ SANE_NAME_GRAY_PREVIEW ];
     if( so )
     {
       if( so ->get() )
       {
	 /* Gray preview on */
	 so->set( true );
	 debug( "Setting GrayPreview ON" );
       }
       else
       {
	 so->set(false );
	 debug( "Setting GrayPreview OFF" );
       }
     }
     apply( so );
   }

     
   if( optionExists( SANE_NAME_SCAN_MODE ) )
   {
      KScanOption mode( SANE_NAME_SCAN_MODE );
      const QString kk = mode.get();
      debug( "Mode is <%s>", (const char*) kk);
      storeOptions->backupOption( mode );
      /* apply if it has a widget, or apply always ? */
      if( mode.widget() ) apply( &mode );
   }
  
   /** Scan Resolution should always exist. **/
   KScanOption res ( SANE_NAME_SCAN_RESOLUTION );
   const QString p = res.get();
   
   debug( "Scan Resolution pre Preview is %s", (const char*) p );
   storeOptions->backupOption( res );

   int set_dpi = dpi;
  
   if( dpi == 0 )
   {
      /* No resolution argument */
      res.getRange( &min, &max, &q );
      if( min > MIN_PREVIEW_DPI )
	 set_dpi = (int) min;
      else
	 set_dpi = MIN_PREVIEW_DPI;
   }

   /* Set scan resolution for preview. */
   if( optionExists( SANE_NAME_SCAN_Y_RESOLUTION ) )
   {
      KScanOption yres ( SANE_NAME_SCAN_Y_RESOLUTION );
      /* if active ? */
      storeOptions->backupOption( yres );
      yres.set( set_dpi );
      apply( &yres );
    
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
#define NOTIFIER(X) ( X == 0  ? ' ' : 'X' )

void KScanDevice::prepareScan( void )
{
    QDictIterator<int> it( option_dic ); // iterator for dict

    debug( "########################################################################################################\n" );
    debug( "Scanner: %s", (const char*) scanner_name );
    debug( "         %s", (const char*) getScannerName() );
    debug("----------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+");
    debug(" Option-Name          | SOFT_SEL  | HARD_SEL  | SOFT_DET  | EMULATED  | AUTOMATIC | INACTIVE  | ADVANCED  |");
    debug("----------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+");

    while ( it.current() )
    {
       // debug( "%s -> %d", it.currentKey().latin1(), *it.current() );
       int descriptor = *it.current();

       const SANE_Option_Descriptor *d = sane_get_option_descriptor( scanner_handle, descriptor );

       if( d )
       {
	  int cap = d->cap;
	  const char *fmt = " %20s |     %c     |     %c     |     %c     |     %c     |     %c     |     %c     |     %c     |";

	  debug( fmt, it.currentKey().latin1(),
		 NOTIFIER( ((cap) & SANE_CAP_SOFT_SELECT) ),
		 NOTIFIER( ((cap) & SANE_CAP_HARD_SELECT) ),
		 NOTIFIER( ((cap) & SANE_CAP_SOFT_DETECT) ),
		 NOTIFIER( ((cap) & SANE_CAP_EMULATED) ),
		 NOTIFIER( ((cap) & SANE_CAP_AUTOMATIC) ),
		 NOTIFIER( ((cap) & SANE_CAP_INACTIVE) ),
		 NOTIFIER( ((cap) & SANE_CAP_ADVANCED )));

       }
       ++it;
    }
    debug("----------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+");

    KScanOption pso( SANE_NAME_PREVIEW );
    const QString q = pso.get();

    debug( "Preview-Switch is at the moment: %s", (const char*) q );
    
    
}

/** Starts scanning
 *  depending on if a filename is given or not, the function tries to open
 *  the file using the Qt-Image-IO or really scans the image.
 **/
KScanStat KScanDevice::acquire( QString filename )
{
    KScanOption *so = 0;
 	
    if( filename.isEmpty() )
    {
 	/* *real* scanning - apply all Options and go for it */
 	prepareScan( );
 	
 	for( so = gui_elements.first(); so; so = gui_elements.next())
 	{
 	    if( so->active() )
 	    {
 	         debug( "apply <%s>", (const char*) so->getName());
 	         apply( so );
 	    }
 	    else
 	    {
 	        debug( "Option <%s> is not active !", (const char*) so->getName() );
 	    }
 	}
   	return( acquire_data( false ));
    }
    else
    {
   	/* a filename is in the parameter */
	QFileInfo *file = new QFileInfo( filename );
	if( file->exists() )
	{
	     QImage *i = new QImage();
	     if( i->load( filename ))
	     {
                  emit( sigNewImage( i ));
	     }
	}
    }
}


/**
 *  Creates a new QImage from the retrieved Image Options
 **/
KScanStat KScanDevice::createNewImage( SANE_Parameters *p )
{
  if( !p ) return( KSCAN_ERR_PARAM );
  KScanStat       stat = KSCAN_OK;

  if( img ) delete( img );

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
    debug( "KScan supports only bit dephts 1 and 8 yet!" );
  }

  if( ! img ) stat = KSCAN_ERR_MEMORY;
  return( stat );
}


KScanStat KScanDevice::acquire_data( bool isPreview )
{
   SANE_Status	  sane_stat = SANE_STATUS_GOOD;
   KScanStat       stat = KSCAN_OK;

   scanningPreview = isPreview;

   sane_stat = sane_start( scanner_handle );
   if( sane_stat == SANE_STATUS_GOOD )
   {
      sane_stat = sane_get_parameters( scanner_handle, &sane_scan_param );

      if( sane_stat == SANE_STATUS_GOOD )
      {
	 debug ("format : %d\nlast_frame : %d\nlines : %d\ndepth : %d\n"
		"pixels_per_line : %d\nbytes_per_line : %d\n",
		sane_scan_param.format, sane_scan_param.last_frame,
		sane_scan_param.lines,sane_scan_param.depth,
		sane_scan_param.pixels_per_line, sane_scan_param.bytes_per_line);
      }
      else
      {
	 stat = KSCAN_ERR_OPEN_DEV;
      }
   }

   if( sane_scan_param.pixels_per_line == 0 || sane_scan_param.lines < 1 )
   {
      debug( "ERROR: Acquiring empty picture !" );
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
      if(data) delete data;
      data = new SANE_Byte[ sane_scan_param.bytes_per_line +4 ];
      if( ! data ) stat = KSCAN_ERR_MEMORY;
   }

   /* Signal for a progress dialog */
   if( stat == KSCAN_OK )
   {	
      emit( sigScanProgress( 0 ) );
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

	       debug ("format : %d\nlast_frame : %d\nlines : %d\ndepth : %d\n"
		      "pixels_per_line : %d\nbytes_per_line : %d\n",
		      sane_scan_param.format, sane_scan_param.last_frame,
		      sane_scan_param.lines,sane_scan_param.depth,
		      sane_scan_param.pixels_per_line, sane_scan_param.bytes_per_line);
	    }
	 } while ( scanStatus != SSTAT_SILENT );
      }
   }
   return( stat );
}



void KScanDevice::slScanFinished( KScanStat status )
{
   // clean up
   if( sn ) {
      sn->setEnabled( false );
      delete sn;
      sn = 0;
   }
   
   emit( sigScanProgress( 1000 ));

   debug( "Slot ScanFinished hit with status %d!", (int) status );

   if( data )
   {
      delete data;
      data = 0;
   }

   const QString qq;
   KScanOption *so;
   (void) so;
   
   if( status == KSCAN_OK )
   {
      if( scanningPreview )
      {
	 debug( "Scanning a preview !" );
	 emit( sigNewPreview( img ));

	 /* The old settings need to be redefined */
	 QAsciiDictIterator<KScanOption> it(*storeOptions);

	 debug( "Postinstalling %d options", storeOptions->count() );
	 while( it.current() )
	 {
	    KScanOption *so = it.current();
	    if( ! so->initialised() ) debug( "%s is not initialised", (const char*) so->getName() );
	    if( ! so->active() ) debug( "%s is not active", (const char*) so->getName() );

	    if( so && so->active() && so->initialised())
	    {
	       const QString qq = so->get();	
	       
	       debug( "Post-Scan-apply <%s>: %s", (const char*) it.currentKey(), (const char*) qq );
	       apply( so );
	    }
	    ++it;
	 }
      }
      else
      {
	 emit( sigNewImage( img ));
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
 * the data-buffer  set to a approbiate size
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
  while( goOn )
  {
     sane_stat =
	sane_read( scanner_handle, data + rest_bytes, sane_scan_param.bytes_per_line, &bytes_written);
     int	red = 0;
     int       green = 0;
     int       blue = 0;

     if( sane_stat != SANE_STATUS_GOOD )
     {
	/** any other error **/
	debug( "sane_read returned with error <%s>: %d bytes left", sane_strstatus( sane_stat ),
	       bytes_written );
	goOn = false;
     }

     if( goOn && bytes_written < 1 )
     {
	// debug( "No bytes written -> leaving out !" );
	goOn = false;  /** No more data -> leave ! **/
     }

     if( goOn )
     {
	overall_bytes += bytes_written;
	// debug( "Bytes read: %d, bytes written: %d", bytes_written, rest_bytes );

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

		 // debug( "RGB: %d, %d, %d\n", red, green, blue );
		 if( pixel_x  == sane_scan_param.pixels_per_line )
		 { pixel_x = 0; pixel_y++; }
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
		       if( pixel_x  >= sane_scan_param.pixels_per_line ) { pixel_x = 0; pixel_y++; }
		       if( pixel_y < sane_scan_param.lines )
		       {
			  chan = (eight_pix & 0x80)> 0 ? 0:1;
			  eight_pix = eight_pix << 1;
			    	//debug( "Setting on %d, %d: %d", pixel_x, pixel_y, chan );
			  img->setPixel( pixel_x, pixel_y, chan );
			  pixel_x++;
		       }
		    }
		 }
	      }	
	
	      break;
	   case SANE_FRAME_RED:
	   case SANE_FRAME_GREEN:
	   case SANE_FRAME_BLUE:
	      debug( "Scanning Single color Frame: %d Bytes!", bytes_written);
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
			  debug( "Undefined format !" );
			  newCol = qRgba( 0xFF, 0xFF, 0xFF, 0xFF );
			  break;
		    }
		    img->setPixel( pixel_x, pixel_y, newCol );
		    pixel_x++;
		 }
	      }
	      break;
	   default:
	      debug( "Unexpected ERROR: No Format type" );
	      break;
	} /* switch */

	
	if( (sane_scan_param.lines > 0) && (sane_scan_param.lines * pixel_y> 0) )
	{
	   emit( sigScanProgress( (int)(1000.0 / (double) sane_scan_param.lines *
					(double)pixel_y) ));
	}
      
	if( bytes_written == 0 || sane_stat == SANE_STATUS_EOF )
	{
	   debug( "Down under sane_stat not OK" );
	   goOn = false;
	}
     }
       
     if( goOn && scanStatus == SSTAT_STOP_NOW )
     {
	/* scanStatus is set to SSTAT_STOP_NOW due to hitting slStopScanning   */
	/* Mostly that one is fired by the STOP-Button in the progress dialog. */

	/* This is also hit after the normal finish of the scan. Most probably,
	 * the QSocketnotifier fires for a few times after the scan has been
	 * cancelled.  Does it matter ? To see it, just uncomment the debug msg. 
	 */
	// debug( "Stopping the scan progress !" );
	goOn = false;
     }

  } /* while( 1 ) */


  /** Comes here if scanning is finished or has an error **/
  if( sane_stat == SANE_STATUS_EOF)
  {
     if ( sane_scan_param.last_frame)
     {
	/** Alles ist gut, das bild ist fertig **/
	debug("last frame reached - scan successfull");
	scanStatus = SSTAT_SILENT;
	emit( sigScanFinished( KSCAN_OK ));
     }	
     else
     {
	/** EOF und nicht letzter Frame -> Parameter neu belegen und neu starten **/
	scanStatus = SSTAT_NEXT_FRAME;
	debug("EOF, but another frame to scan");

     }
  }
           
  if( sane_stat == SANE_STATUS_CANCELLED )
  {	
     scanStatus = SSTAT_STOP_NOW;
     debug("Scan was cancelled");

     // stat = KSCAN_CANCELLED;
     // emit( sigScanFinished( stat ));

     /* hmmm - how could this happen ? */
  }
} /* end of fkt */
#include "kscandevice.moc"
