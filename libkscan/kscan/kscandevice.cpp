
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



#define MIN_PREVIEW_DPI 20


// global device list

static bool        scanner_initialised = false;
static SANE_Handle scanner_handle      = 0;
static QDict<int>  option_dic;

static SANE_Device const **dev_list          = 0;


inline const SANE_Option_Descriptor *getOptionDesc( const char *name )
{
   if( ! scanner_initialised || ! name )
   {
   	debug( "ERROR: Scanner not initialised !" );
	return 0;
   }

   int *idx = option_dic[ name ];
   const SANE_Option_Descriptor *d = 0;
   if ( idx && *idx > 0 )
   {
      d = sane_get_option_descriptor( scanner_handle, *idx );
      // debug( "retrieving Option %s", d->name );
   } else { debug( "no option descriptor for %s", name ); }
   return( d );
}



/* ************************************************************************ */
/* KScan Option                                                             */
/* ************************************************************************ */
KScanOption::KScanOption( const char *new_name ) :
   QObject()
{
	initOption( new_name );

	int  *num = option_dic[ getName() ];	
	if( !num || !buffer )
			return;

	sane_control_option( scanner_handle, *num,
	                     SANE_ACTION_GET_VALUE, buffer, 0 );
}


void KScanOption::initOption( const char *new_name )
{
   desc = 0;
   if( ! new_name ) return;

   name = new_name;
   desc = getOptionDesc( name );
   buffer = 0;
   internal_widget = 0;
   buffer_untouched = true;

   if( desc )
   {
  		
	/* Gamma-Table - initial values */
	gamma = 0; /* marks as unvalid */	
	if( type() == GAMMA_TABLE ) gamma = 100;
	brightness = 0;
	contrast = 0;
  	
	if( type() == GAMMA_TABLE )
	{
	    /* Set to neutral table */
	    gamma = 100;
	}	
  	// allocate memory
  	switch( desc->type )
  	{
  	    case SANE_TYPE_INT:
  	    case SANE_TYPE_FIXED:
	    case SANE_TYPE_STRING:  		
  		buffer_size = desc->size;
 		buffer = (void*) malloc ( buffer_size );
 		memset( buffer, 0, buffer_size );
	    break;
    	    case SANE_TYPE_BOOL:
    		buffer_size = sizeof( SANE_Word );
	 	buffer = (void*) malloc ( buffer_size );
	 	memset( buffer, 0, buffer_size );
	    break;
            default:
    		buffer_size = 0;
	        buffer = 0;
  	}
    }
}

KScanOption::KScanOption( const KScanOption &so ) :
   QObject()
{
   /* desc is stored by sane-lib and may be copied */
   desc = so.desc;
   name = so.name;
   buffer_untouched = so.buffer_untouched;
   gamma = so.gamma;
   brightness = so.brightness;
   contrast = so.contrast;

   /* the widget si not copied ! */
   internal_widget = 0;

   switch( desc->type )
   {
       case SANE_TYPE_INT:
       case SANE_TYPE_FIXED:
       case SANE_TYPE_STRING:  		
	    buffer_size = desc->size;
	    buffer = malloc( buffer_size );
	    memcpy( buffer, so.buffer, desc->size / sizeof( SANE_Word ));
       break;
       case SANE_TYPE_BOOL:
	    buffer_size = sizeof( SANE_Word );
	    buffer = malloc( buffer_size );
 	    memcpy( buffer, so.buffer,  sizeof( SANE_Word ));
       break;
       default:
            buffer = 0;
            buffer_size = 0;
    }
}

const KScanOption& KScanOption::operator= (const KScanOption& so )
{
   /* desc is stored by sane-lib and may be copied */
   if( this == &so ) return( *this );

   desc = so.desc;
   name = so.name;
   buffer_untouched = so.buffer_untouched;
   gamma = so.gamma;
   brightness = so.brightness;
   contrast = so.contrast;

   if( internal_widget ) delete internal_widget;
   internal_widget = so.internal_widget;

   if( buffer ) free (buffer);


   switch( desc->type )
   {
       case SANE_TYPE_INT:
       case SANE_TYPE_FIXED:
       case SANE_TYPE_STRING:  		
	    buffer_size = desc->size;
	    buffer = malloc( buffer_size );
	    memcpy( buffer, so.buffer, desc->size / sizeof( SANE_Word ));
       break;
       case SANE_TYPE_BOOL:
	    buffer_size = sizeof( SANE_Word );
	    buffer = malloc( buffer_size );
 	    memcpy( buffer, so.buffer,  sizeof( SANE_Word ));
       break;
       default:
            buffer = 0;
            buffer_size = 0;
    }
    return( *this );
}

void KScanOption::slWidgetChange( const char *t )
{
    debug( "Received WidgetChange (const char*)" );
    set( QString(t) );
    emit( guiChange( this ) );
    // emit( optionChanged( this ));
}

void KScanOption::slWidgetChange( void )
{
    debug( "Received WidgetChange (void)" );
    /* If Type is bool, the widget is a checkbox. */
    if( type() == BOOL )
    {
	bool b = ((QCheckBox*) internal_widget)->isChecked();
	set( b );
    }
    emit( guiChange( this ) );
    // emit( optionChanged( this ));

}

void KScanOption::slWidgetChange( int i )
{
    debug( "Received WidgetChange (int)" );
    set( i );
    emit( guiChange( this ) );
    // emit( optionChanged( this ));
}

/* this slot is called on a widget change, if a widget was created.
 * In normal case, it is internally connected, so the param so and
 * this are equal !
 */
void KScanOption::slRedrawWidget( KScanOption *so )
{
	// debug( "Checking widget %s", (const char*) so->getName());
	int     help = 0;
	QString string;
	
	QWidget *w = so->widget();
	
	if( so->valid() && w && so->getBuffer() )
	{
	 	switch( so->type( ) )
 		{
 		 	case BOOL:
 		 	if( so->get( &help ))
 		 		((QCheckBox*) w)->setChecked( (bool) help );
 	 		/* Widget Type is ToggleButton */
 	 		break;
 	 		case SINGLE_VAL:
 	 			/* Widget Type is Entry-Field - not implemented yet */
 	 			
 	 		break;
 	 		case RANGE:
 	 	  		/* Widget Type is Slider */
 	 	  		if( so->get( &help ))
				kdDebug() << "HELP: " << help;
	 	 	  	((KScanSlider*)w)->slSetSlider( help );
				 	 	  		
 	 		break;
 	 		case GAMMA_TABLE:
 	 	  		/* Widget Type is GammaTable */
 	 	    	// w = new QSlider( parent, "AUTO_GAMMA" );
 	 		break;
 	 		case STR_LIST:	 		
	 	  		 // w = comboBox( parent, text );
				((KScanCombo*)w)->slSetEntry( so->get() ); 	  		
 	 	  		/* Widget Type is Selection Box */
 	 		break;
 	 		case STRING:
 	 	   	 // w = entryField( parent, text );
				((KScanEntry*)w)->slSetEntry( so->get() ); 	  	 	
 	  	 		/* Widget Type is Selection Box */
 	    	        break;
 	  		default:
 	  			// w  = 0;
 	  		break;
 		}
	}
}

/* In this slot, the option queries the scanner for current values. */

void KScanOption::slReload( void )
{
	int  *num = option_dic[ getName() ];	
	desc = getOptionDesc( getName() );	
	
	if( !desc || !num  )
		return;
		
	if( widget() )
	{
	    if( !active() || !softwareSetable() )
	    {
                debug( "Disabling widget %s!", (const char*) getName());
 	 	widget()->setEnabled( false );
            }
 	    else
 	 	widget()->setEnabled( true );
	}
	
	/* first get mem if nothing is approbiate */
	if( !buffer )
	{
	    debug(" *********** getting without space **********" );
            // allocate memory
            switch( desc->type )
            {
  		case SANE_TYPE_INT:
  		case SANE_TYPE_FIXED:
	        case SANE_TYPE_STRING:  		
  			buffer_size = desc->size;
 			buffer = (void*) malloc ( buffer_size );
		break;
    	        case SANE_TYPE_BOOL:
    		        buffer_size = sizeof( SANE_Word );
	 		buffer = (void*) malloc ( buffer_size );
	    	break;
    	        default:
    		if( desc->size > 0 )
    		{
	      	        buffer = (void*) malloc ( desc->size );
	       	        buffer_size = desc->size;
	                ((char*) buffer)[desc->size -1] = 0;
	        }
            }
  	}
	
  	if( active())
  	{
  		if( (size_t) desc->size > buffer_size )
  		{
  			debug( "SERIOUS ERROR: Buffer to small: %d (required) - %d %s !",
  					 desc->size, buffer_size, (const char*) name );
  		}
  			
  		SANE_Status sane_stat = sane_control_option( scanner_handle, *num,
  		                        SANE_ACTION_GET_VALUE, buffer, 0 );

   	        if( sane_stat != SANE_STATUS_GOOD )
   	        {
    		    debug( "ERROR: Cant get value for %s: %s", (const char*) getName(),
    	   	    sane_strstatus( sane_stat ));
                }
     	        else
   	        {
   		    buffer_untouched = false;
   	        }
         }
}


KScanOption::~KScanOption()
{
   if( buffer ) free( buffer );
}

bool KScanOption::valid( void )
{
	return( desc && 1 );
}

bool KScanOption::autoSetable( void )
{
	/* Refresh description */
	desc = getOptionDesc( name );

		
 	return( desc && ((desc->cap & SANE_CAP_AUTOMATIC) > 0 ) );
}

bool KScanOption::active( void )
{
	bool ret = false;
	/* Refresh description */
	desc = getOptionDesc( name );
 	if( desc )
 		ret = SANE_OPTION_IS_ACTIVE( desc->cap );
 			
 	return( ret );
}

bool KScanOption::softwareSetable( void )
{
	/* Refresh description */
	desc = getOptionDesc( name );
 	return( desc && ( SANE_OPTION_IS_SETTABLE(desc->cap ) == SANE_TRUE ) );
}

KScan_Type KScanOption::type( void )
{
	KScan_Type ret = INVALID_TYPE;
	
 	if( valid() )
 	{
 		switch( desc->type )
   	{	
   		case SANE_TYPE_BOOL:
   			ret = BOOL;
   			break;
   		case SANE_TYPE_INT:
   		case SANE_TYPE_FIXED:
   			if( desc->constraint_type == SANE_CONSTRAINT_RANGE )
   			/* FIXME ! Dies scheint nicht wirklich so zu sein */
   			   if( desc->size == sizeof( SANE_Word ))
   			       ret = RANGE;
   			   else
   			       ret = GAMMA_TABLE;
   			else if( desc->constraint_type == SANE_CONSTRAINT_NONE )
   			   ret = SINGLE_VAL;
   			else if( desc->constraint_type == SANE_CONSTRAINT_WORD_LIST )
   			   ret = GAMMA_TABLE;
   			else
   			   ret = INVALID_TYPE;
   			break;
   		case SANE_TYPE_STRING:
   		    if( desc->constraint_type == SANE_CONSTRAINT_STRING_LIST )
	   			ret = STR_LIST;
	   		else
	   			ret = STRING;
   			break;
   		default:
   		  ret = INVALID_TYPE;
   			break;
   	}
 	}
 	return( ret ); 	
}


bool KScanOption::set( int val )
{
   if( ! desc ) return( false );
   bool ret = false;

   int word_size       = 0;
   QArray<SANE_Word> qa;
   SANE_Word sw        = SANE_TRUE;
   const SANE_Word sw1 = val;
   const SANE_Word sw2 = SANE_FIX( (double) val );

   switch( desc->type )
   {
      case SANE_TYPE_BOOL:
	
	 if( ! val )
	    sw = SANE_FALSE;
	 if( buffer ) {
	    memcpy( buffer, &sw, sizeof( SANE_Word ));
	    ret = true;
	 }
	 break;
      // Type int: Fill the whole buffer with that value
      case SANE_TYPE_INT:
	 word_size = desc->size / sizeof( SANE_Word );

	 qa.resize( word_size );
	 qa.fill( sw1 );
	
	 if( buffer ) {
	    memcpy( buffer, qa.data(), desc->size );
	    ret = true;
	 }
	 break;

      // Type fixed: Fill the whole buffer with that value
      case SANE_TYPE_FIXED:
	 word_size = desc->size / sizeof( SANE_Word );

	 qa.resize( word_size );
	 qa.fill( sw2 );
	
	 if( buffer ) {
	    memcpy( buffer, qa.data(), desc->size );
	    ret = true;
	 }
	 break;
      default:
	 debug( "Cant set %s with type int", (const char*) name );
   }
   if( ret )
   {
      buffer_untouched = false;
#if 0
      emit( optionChanged( this ));
#endif
   }

   return( ret );
}



bool KScanOption::set( double val )
{
   if( ! desc ) return( false );
   bool ret = false;
   int  word_size = 0;
   QArray<SANE_Word> qa;
   SANE_Word sw = SANE_FALSE;

   switch( desc->type )
   {
      case SANE_TYPE_BOOL:
	
	 if( val > 0 )
	    sw = SANE_TRUE;
	 if( buffer ) {
	    memcpy( buffer, &sw, sizeof( SANE_Word ));
	    ret = true;
	 }
	 break;
      // Type int: Fill the whole buffer with that value
      case SANE_TYPE_INT:
	 sw = (SANE_Word) val;
	 word_size = desc->size / sizeof( SANE_Word );

	 qa.resize( word_size );
	 qa.fill( sw );
	
	 if( buffer ) {
	    memcpy( buffer, qa.data(), desc->size );
	    ret = true;
	 }
	 break;

      // Type fixed: Fill the whole buffer with that value
      case SANE_TYPE_FIXED:
	 sw = SANE_FIX( val );
	 word_size = desc->size / sizeof( SANE_Word );

	 qa.resize( word_size );
	 qa.fill( sw );
	
	 if( buffer ) {
	    memcpy( buffer, qa.data(), desc->size );
	    ret = true;
	 }
	 break;
      default:
	 debug( "Cant set %s with type double", (const char*) name );
   }

   if( ret )
   {
      buffer_untouched = false;
#if 0
      emit( optionChanged( this ));
#endif
   }

   return( ret );
}



bool KScanOption::set( int *val, int size )
{
   if( ! desc || ! val ) return( false );
   bool ret = false;

   int word_size = desc->size / sizeof( SANE_Word );
   QArray<SANE_Word> qa( word_size );

   switch( desc->type )
   {
      case SANE_TYPE_INT:
	 for( int i = 0; i < word_size; i++ ){
	    if( i < size )
	       qa[i] = (SANE_Word) *(val++);
	    else
	       qa[i] = (SANE_Word) *val;
	 }
	 break;

      // Type fixed: Fill the whole buffer with that value
      case SANE_TYPE_FIXED:
	 for( int i = 0; i < word_size; i++ ){
	    if( i < size )
	       qa[i] = SANE_FIX((double)*(val++));
	    else
	       qa[i] = SANE_FIX((double) *val );
	 }
	 break;
      default:
	 debug( "Cant set %s with type int*", (const char*) name );
	
   }

   if( ret && buffer ) {
      memcpy( buffer, qa.data(), desc->size );
      ret = true;
   }

   if( ret )
   {
     buffer_untouched = false;
#if 0
     emit( optionChanged( this ));
#endif
   }

   return( ret );
}

bool KScanOption::set( const char *val )
{
 	return( set( QString(val) ));
}

bool KScanOption::set( QString strval )
{
   bool ret = false;
   QCString c_string( strval );

   if( ! desc ) return( false );

   /* On String-type the buffer gets malloced in Constructor */
   if( desc->type == SANE_TYPE_STRING )
   {
   	debug("Setting %s as String", (const char*) c_string );
   	
   	if( buffer_size >= c_string.length() )
   	{
   	     memset( buffer, 0, buffer_size );
             qstrncpy( (char*) buffer, (const char*) c_string, buffer_size );
             ret = true;
        }
        else
        {
      	     debug( "ERROR: Buffer for String %s too small: %d < %d",
      	             (const char*)c_string,
      		     buffer_size, c_string.length() );
        }
   } else {
      debug( "Cant set %s with type string", (const char*) name );
   }

   if( ret )
   {
     buffer_untouched = false;
#if 0
     emit( optionChanged( this ));
#endif
   }

   return( ret );
}


bool KScanOption::set( KGammaTable *gt )
{
   if( ! desc ) return( false );
   bool ret = true;
   int size = gt->tableSize();
   SANE_Word *run = gt->getTable();

   int word_size = desc->size / sizeof( SANE_Word );
   QArray<SANE_Word> qa( word_size );

   switch( desc->type )
   {
      case SANE_TYPE_INT:
	 for( int i = 0; i < word_size; i++ ){
	    if( i < size )
	       qa[i] = *run++;
	    else
	       qa[i] = *run;
	 }
	 break;

      // Type fixed: Fill the whole buffer with that value
      case SANE_TYPE_FIXED:
	 for( int i = 0; i < word_size; i++ ){
	    if( i < size )
	       qa[i] = SANE_FIX((double)*(run++));
	    else
	       qa[i] = SANE_FIX((double) *run );
	 }
	 break;
      default:
	 debug( "Cant set %s with type GammaTable", (const char*) name );
	 ret = false;
	
   }

   if( ret && buffer )
   {
      /* remember raw vals */
      gamma      = gt->getGamma();
      brightness = gt->getBrightness();
      contrast   = gt->getContrast();

      memcpy( buffer, qa.data(), desc->size );
      buffer_untouched = false;
#if 0
      emit( optionChanged( this ));
#endif
   }

   return( ret );
}

bool KScanOption::get( int *val )
{
	SANE_Word sane_word;
	double d;
	if( !valid() || !getBuffer())
		return( false );
			
   switch( desc->type )
   {
      case SANE_TYPE_BOOL:
	   	/* Buffer has a SANE_Word */
			sane_word = *((SANE_Word*)buffer);
			if( sane_word == SANE_TRUE )
				*val = 1;
			else
				*val = 0;
		break;
      /* Type int: Fill the whole buffer with that value */
      /* reading the first is OK */
      case SANE_TYPE_INT:
      	sane_word = *((SANE_Word*)buffer);
			*val = sane_word;
		break;

      // Type fixed: whole buffer filled with the same value
      case SANE_TYPE_FIXED:
	 		d = SANE_UNFIX(*(SANE_Word*)buffer);
         *val = (int)d;
	 	break;
      default:
	 		debug( "Cant get %s to type int", (const char*) name );
	 		return( false );
   }

   // debug( "option::get returns %d", *val );
   return( true );
}



const QString KScanOption::get( void )
{

   QCString retstr;
	
   if( desc->type == SANE_TYPE_STRING && valid() && getBuffer() )
   {
   	const char *cc = (const char*) getBuffer();
   	retstr = cc;
	   // retstr.sprintf( "%s", cc );
   }
   else
   {
    	debug( "Cant get %s to type String !", (const char*)getName());
   }
   debug( "option::get returns " + retstr );
   return( retstr );
}


/* Caller needs to have the space ;) */
bool KScanOption::get( KGammaTable *gt )
{
    if( gt )
    {
        gt->setAll( gamma, brightness, contrast );
        // gt->calcTable();
        return( true );
    }
    return( false );
}


QStrList KScanOption::getList( )
{
   if( ! desc ) return( false );
   const char	**sstring = 0;
   QStrList strList;

   if( desc->constraint_type == SANE_CONSTRAINT_STRING_LIST ) {
      sstring =  (const char**) desc->constraint.string_list;

      while( *sstring ) {
	 		debug( "This is in the stringlist: %s\n", *sstring );
			strList.append( *sstring );
	 		sstring++;
      }
   }
   if( desc->constraint_type == SANE_CONSTRAINT_WORD_LIST ) {
      const SANE_Int *sint =  desc->constraint.word_list;
      int amount_vals = *sint; sint ++;
      QString s;

      for( int i=0; i < amount_vals; i++ ) {
	 if( desc->type == SANE_TYPE_FIXED )
	    s.sprintf( "%f", SANE_UNFIX(*sint) );
	 else
	    s.sprintf( "%d", *sint );
	 sint++;
	 strList.append( s );
      }
   }
   return( strList );
}


bool KScanOption::getRange( double *min, double *max, double *q )
{
   if( !desc ) return( false );
   bool ret = true;

   if( desc->constraint_type == SANE_CONSTRAINT_RANGE) {
      const SANE_Range *r = desc->constraint.range;

      if( desc->type == SANE_TYPE_FIXED ) {
	  *min = (double) SANE_UNFIX( r->min );
 	  *max = (double) SANE_UNFIX( r->max );
	  *q   = (double) SANE_UNFIX( r->quant );
      } else {
	  *min = r->min ;
	  *max = r->max ;
	  *q   = r->quant ;
      }
   } else {
      debug( "getRange: No range type %s", desc->name );
      ret = false;
   }
   return( ret );
}

QWidget *KScanOption::createWidget( QWidget *parent, const char *w_desc,
                                    const char *tooltip )
{
	const char *text = 0;
	
	QStrList list;
 	if( ! valid() ) return( 0 );
 	QWidget *w = 0;
 	/* free the old widget */
 	if( internal_widget ) delete internal_widget;
 	internal_widget = 0;
 	
 	/* check for text */
 	text = w_desc;
 	if( ! text && desc) {
 		text = (const char*) desc->title; 	
 	}
 	
 	
 	switch( type( ) )
 	{
 	 	case BOOL:
 	 		/* Widget Type is ToggleButton */
 	 		w = new  QCheckBox( text, parent, "AUTO_TOGGLE_BUTTON" );
 	 		connect( w, SIGNAL(clicked()), this,
 	 		         SLOT(slWidgetChange()));
 	 	break;
 	 	case SINGLE_VAL:
 	 	   /* Widget Type is Entry-Field */
 	 	   w = 0; // new QEntryField(
 	 	break;
 	 	case RANGE:
 	 	   /* Widget Type is Slider */
 	 	    w = sliderWidg( parent, text );
 	 	break;
 	 	case GAMMA_TABLE:
 	 	   /* Widget Type is Slider */
 	 	    // w = KScanSlider( parent, text );
 	 	    w = 0; // No widget, needs to be a set !
 	 	break;
 	 	case STR_LIST:	 		
	 	    w = comboBox( parent, text );
 	 	   /* Widget Type is Selection Box */
 	 	break;
 	 	case STRING:
 	 	    w = entryField( parent, text );
 	  	   /* Widget Type is Selection Box */
 	        break;
 	  	default:
 	  	    w  = 0;
 	  	break;
 	}
 	
 	if( w )
 	{
 		internal_widget = w;
 		connect( this, SIGNAL( optionChanged( KScanOption*)),
 		               SLOT( slRedrawWidget( KScanOption* )));
 		const char *tt = tooltip;
 		if( ! tt && desc )
 			tt = desc->desc;
 			
 		if( tt )
	 		QToolTip::add( internal_widget, tt );
 	}
 	
 	/* Check if option is active */
 	if( ! active() || !softwareSetable())
 	{
 		debug( "Option is not active !" );
 	 	w->setEnabled( false );
 	}
	return( w );
}


QWidget *KScanOption::comboBox( QWidget *parent, const char *text )
{
 	QStrList list = getList();
 	
 	KScanCombo *cb = new KScanCombo( parent, text, list);

 	connect( cb, SIGNAL( valueChanged(const char*)), this,
		      SLOT( slWidgetChange(const char*)));

 	return( cb );
}


QWidget *KScanOption::entryField( QWidget *parent, const char *text )
{
	KScanEntry *ent = new KScanEntry( parent, text );
 	connect( ent, SIGNAL( valueChanged(const char*)), this,
		      SLOT( slWidgetChange(const char*)));
	
 	return( ent );
}


QWidget *KScanOption::sliderWidg( QWidget *parent, const char *text )
{
 	double min, max, quant;
 	getRange( &min, &max, &quant );

	KScanSlider *slider = new KScanSlider( parent, text, min, max );
   /* Connect to the options change Slot */
  	connect( slider, SIGNAL( valueChanged(int)), this,
		 SLOT( slWidgetChange(int)));

	return( slider );
}


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

    scanner_initialised = false;  // stays false until openDevice.
    scanStatus = SSTAT_SILENT;
    data = 0;
    sn = 0;
    img = 0;

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


QString KScanDevice::getScannerName()
{
  QString ret = "No scanner selected";

  if( scanner_initialised )
    {
      SANE_Device *scanner = scannerDevices[ scanner_name ];

      if( scanner ) {
	ret.sprintf( "%s %s %s", scanner->vendor, scanner->model, scanner->type );
      }	
    }
  return ( ret );
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
		 option_dic.insert ( (const char*) d->name, new_opt );
		 option_list.append( (const char*) d->name );
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

   const SANE_Option_Descriptor *d;

   QString s = option_list.first();

   while( !(s.isEmpty() || s.isNull()) ) {
      d = getOptionDesc( s );
      if( d && !(d->cap & SANE_CAP_ADVANCED) )
	 com_opt.append( s );
      s = option_list.next();
   }
   return( com_opt );
}

QStrList KScanDevice::getAdvancedOptions()
{
   QStrList com_opt;

   const SANE_Option_Descriptor *d;

   QString s = option_list.first();

   while( !(s.isEmpty() || s.isNull()) ) {
      d = getOptionDesc( s );
      if( d && !(d->cap & SANE_CAP_ADVANCED) )
	 com_opt.append( s );
      s = option_list.next();
   }
   return( com_opt );
}

KScanStat KScanDevice::apply( KScanOption *opt )
{
   KScanStat   stat = KSCAN_OK;
   int         *num = option_dic[ opt->getName() ];
   SANE_Int    result = 0;
   SANE_Status sane_stat = SANE_STATUS_GOOD;

   if ( opt->getName() == "preview" || opt->getName() == "mode" ) {
       sane_stat = sane_control_option( scanner_handle, *num,
					SANE_ACTION_SET_AUTO, 0,
					&result );
       return KSCAN_OK;
   }


   if( ! opt->initialised() || opt->getBuffer() == 0 )
   {
      debug( "Attempt to set Zero buffer of %s -> skipping !",
	     (const char*) opt->getName() );
   	
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
      if( opt->active() && opt->softwareSetable())
	 sane_stat = sane_control_option( scanner_handle, *num,
					  SANE_ACTION_SET_VALUE,
					  opt->getBuffer(),
					  &result );
      else
      {
	 debug( "Option %s is not active now !", (const char*) opt->getName());
	 result = 0;
      }
   }		

   if( sane_stat == SANE_STATUS_GOOD )
   {
      if( result & SANE_INFO_RELOAD_OPTIONS )
      {
#if 0		
	 debug( "Emitting sigOptionChanged()" );
	 stat = KSCAN_RELOAD;

	 emit( sigOptionsChanged() );
#endif	
      }

      if( result & SANE_INFO_RELOAD_PARAMS )
	 emit( sigScanParamsChanged() );

      if( result & SANE_INFO_INEXACT )
      {
	 debug( "Option <%s> was set inexact !",
		(const char*) opt->getName() );
      }
   } else {
      debug( "Setting of %s failed: %s", (const char*) opt->getName(),
	     sane_strstatus( sane_stat ) );
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
	 	so->slReload();
	 	so->slRedrawWidget(so);
	    }
	}
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
	KScanOption *so = 0;
 	double min, max, q;
 	
 	for( so = gui_elements.first(); so; so = gui_elements.next())
 	{
 	    debug( "apply <%s>", (const char*) so->getName());
 	    apply( so );
 	}
 	
 	KScanOption preview( SANE_NAME_PREVIEW );
 	preview.set( true );
 	apply( &preview );
 	
 	KScanOption mode( SANE_NAME_SCAN_MODE );
 	QString oldmode = mode.get();
 	debug( "Restoring to old mode: " + oldmode );
 	
 	KScanOption f_gray( SANE_NAME_GRAY_PREVIEW );
   	f_gray.set( forceGray );
	// apply( &f_gray );

 	/* Set scan resolution for preview. */	
 	KScanOption res( SANE_NAME_SCAN_RESOLUTION );
 	int resolution = 0;
 	res.get( &resolution);  /* remember old value */
 	if( dpi == 0 )
 	{
 	    /* No resolution argument */
	    res.getRange( &min, &max, &q );
	    if( min > MIN_PREVIEW_DPI )
	    	res.set( min );
	    else
	    	res.set( MIN_PREVIEW_DPI );
	}
	else
	{
	    res.set( dpi );
	}
	apply( &res );
 	
 	/* Start scanning */
 	KScanStat stat = acquire_data( true );

        /* Restore the old settings again */	
        f_gray.set( false );     apply( &f_gray );
 	preview.set( false );    apply( &preview );
 	res.set( resolution );   apply( &res );
 	mode.set( oldmode );     apply( &mode );
 	
 	/* Refresh the screen */
        slReloadAll(); 	
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
    debug("-------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+");
    debug(" Option-Name       | SOFT_SEL  | HARD_SEL  | SOFT_DET  | EMULATED  | AUTOMATIC | INACTIVE  | ADVANCED  |");
    debug("-------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+");

    while ( it.current() )
    {
      // debug( "%s -> %d", it.currentKey().latin1(), *it.current() );
	int descriptor = *it.current();

	const SANE_Option_Descriptor *d = sane_get_option_descriptor( scanner_handle, descriptor );

	if( d )
	  {
	    int cap = d->cap;
	    const char *fmt = " %17s |     %c     |     %c     |     %c     |     %c     |     %c     |     %c     |     %c     |";

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
    debug("-------------------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+");

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
  sane_cancel(scanner_handle);
  emit( sigScanProgress( 1000 ));

  debug( "Slot ScanFinished hit !" );

  if( data )
    {
      delete data;
      data = 0;
    }

  if( status == KSCAN_OK )
    {
      if( scanningPreview )
	emit( sigNewPreview( img ));
      else
	emit( sigNewImage( img ));
    }	


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

  // int 	rest_bytes = 0;
  while( 1 )
  {
      sane_stat =
	sane_read( scanner_handle, data + rest_bytes, sane_scan_param.bytes_per_line, &bytes_written);
      int	red = 0;
      int       green = 0;
      int       blue = 0;

      if( sane_stat != SANE_STATUS_GOOD )
      {
	/** any other error **/
	debug( "sane_read returned with error: %s, %d bytes left", sane_strstatus( sane_stat ),
	       bytes_written );
	break;
      }

      if( ! bytes_written )
      {
	// debug( "No bytes written -> leaving out !" );
	break;  /** No more data -> leave ! **/
      }
      overall_bytes += bytes_written;
      // cout << "Bytes read: " << bytes_written << " Rest bytes: "
      //	 << rest_bytes << "\n";
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
	  debug( "Heavy ERROR: No Format type" );
	  break;
	} /* switch */

	
      if( (sane_scan_param.lines > 0) && (sane_scan_param.lines * pixel_y> 0) )
	emit( sigScanProgress( (int)(1000.0 / (double) sane_scan_param.lines *
				     (double)pixel_y) ));

      if( scanStatus == SSTAT_STOP_NOW )
	{
	  debug( "Stopping the scan progress !" );
	  break;
	}

  } /* while( 1 ) */


  KScanStat stat = KSCAN_OK;
  /** Comes here if scanning is finished or has an error **/
  if( sane_stat == SANE_STATUS_EOF)
    {
      if ( sane_scan_param.last_frame)
        {
	  /** Alles ist gut, das bild ist fertig **/
          debug("last frame reached - scan successfull");
	  scanStatus = SSTAT_SILENT;
	  emit( sigScanFinished( stat ));
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

      stat = KSCAN_CANCELLED;
      emit( sigScanFinished( stat ));

      /* hmmm - how could this happen ? */
    }
} /* end of fkt */
