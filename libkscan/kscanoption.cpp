/***************************************************************************
	     kscanoption.cpp -  Scanner option class
                             -------------------
    begin                : Wed Oct 13 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : Klaas.Freitag@SuSE.de

    $Id$
 ***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


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
#include "kscanoptset.h"


#define MIN_PREVIEW_DPI 20

/* switch to show some from time to time usefull alloc-messages */
#undef MEM_DEBUG

#undef APPLY_IN_SITU

// global device list


/**  Not really pretty to have these global variables in a lib.
 *   Should be solved by KScanOption being a friend to KScanDevice ?
 *   TODO 
 **/
bool        scanner_initialised = false;
SANE_Handle scanner_handle      = 0;
QDict<int>  option_dic;

extern KScanOptSet gammaTables;

/** inline-access to the option descriptor, object to changes with global vars. **/
inline const SANE_Option_Descriptor *getOptionDesc( const char *name )
{
   
   int *idx = option_dic[ name ];
   const SANE_Option_Descriptor *d = 0;
   // debug( "<< for option %s >>", name );
   if ( idx && *idx > 0 )
   {
      d = sane_get_option_descriptor( scanner_handle, *idx );
      // debug( "retrieving Option %s", d->name );
   }
   else
   {
      kdDebug() << "no option descriptor for <" << name << ">" << endl;
      // debug( "Name survived !" );
   }
   // debug( "<< leaving option %s >>", name );
   
   return( d );
}



/* ************************************************************************ */
/* KScan Option                                                             */
/* ************************************************************************ */
KScanOption::KScanOption( const char *new_name ) :
   QObject()
{

   if( initOption( new_name ) )
   {
      int  *num = option_dic[ getName() ];	
      if( !num || !buffer )
	 return;

      SANE_Status sane_stat = sane_control_option( scanner_handle, *num,
						   SANE_ACTION_GET_VALUE, buffer, 0 );

      if( sane_stat == SANE_STATUS_GOOD )
      {
	 buffer_untouched = false;
      }
   }
   else
   {
      kdDebug() << "Had problems to create KScanOption - initOption failed !" << endl;
   }
}


bool KScanOption::initOption( const char *new_name )
{
   desc = 0;
   if( ! new_name ) return( false );

   name = new_name;
   desc = getOptionDesc( name.local8Bit() );
   buffer = 0;
   internal_widget = 0;
   buffer_untouched = true;
   buffer_size = 0;
   
   if( desc )
   {
  		
	/* Gamma-Table - initial values */
	gamma = 0; /* marks as unvalid */	
	brightness = 0;
	contrast = 0;
	gamma = 100;
	
  	// allocate memory
  	switch( desc->type )
  	{
  	    case SANE_TYPE_INT:
  	    case SANE_TYPE_FIXED:
	    case SANE_TYPE_STRING:  		
 		buffer = allocBuffer( desc->size );
	    break;
    	    case SANE_TYPE_BOOL:
	 	buffer = allocBuffer( sizeof( SANE_Word ) );
	    break;
            default:
    		buffer_size = 0;
	        buffer = 0;
  	}

	KScanOption *gtOption = gammaTables[ new_name ];
	if( gtOption )
	{
	   kdDebug() << "Is older GammaTable!" << endl;
	   KGammaTable gt;
	   gtOption->get( &gt );
	   
	   gamma = gt.getGamma();
	   contrast = gt.getContrast();
	   brightness = gt.getBrightness();
	}
	else
	{
	   kdDebug() << "Is NOT older GammaTable!" << endl;
	}
    }
   
   return( desc > 0 );
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

   if( so.buffer_untouched ) kdDebug() << "Buffer of source is untouched!" << endl;
   // debug("Here Duplication for %s, reserving %d bytes", (const char*)  name, desc->size );
   
   /* the widget si not copied ! */
   internal_widget = 0;

   switch( desc->type )
   {
       case SANE_TYPE_INT:
       case SANE_TYPE_FIXED:
       case SANE_TYPE_STRING:  		
	    buffer = allocBuffer( desc->size );
	    // desc->size / sizeof( SANE_Word )
	    memcpy( buffer, so.buffer, buffer_size  );
       break;
       case SANE_TYPE_BOOL:
	    buffer = allocBuffer( sizeof( SANE_Word ) );
 	    memcpy( buffer, so.buffer,  buffer_size );
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

   if( buffer ) delete( (char*)buffer);

   switch( desc->type )
   {
       case SANE_TYPE_INT:
       case SANE_TYPE_FIXED:
       case SANE_TYPE_STRING:  		
	    buffer = allocBuffer( desc->size );
	    memcpy( buffer, so.buffer, buffer_size );
       break;
       case SANE_TYPE_BOOL:
	    buffer = allocBuffer( sizeof( SANE_Word ) );
 	    memcpy( buffer, so.buffer,  buffer_size );
       break;
       default:
            buffer = 0;
            buffer_size = 0;
    }
    return( *this );
}

void KScanOption::slWidgetChange( const char *t )
{
    kdDebug() << "Received WidgetChange for " << getName() << "const char*" << endl;
    set( QString(t) );
    emit( guiChange( this ) );
    // emit( optionChanged( this ));
}

void KScanOption::slWidgetChange( void )
{
    kdDebug() << "Received WidgetChange for " << getName() << " (void)" << endl;
    /* If Type is bool, the widget is a checkbox. */
    if( type() == BOOL )
    {
	bool b = ((QCheckBox*) internal_widget)->isChecked();
	kdDebug() << "Setting bool: " << b << endl;
	set( b );
    }
    emit( guiChange( this ) );
    // emit( optionChanged( this ));

}

void KScanOption::slWidgetChange( int i )
{
    kdDebug() << "Received WidgetChange for " << getName() << " (int)" << endl;
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
  // qDebug( "Checking widget %s", (const char*) so->getName());
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
	  ((KScanEntry*)w)->slSetEntry( so->get().local8Bit() ); 	  	 	
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
   desc = getOptionDesc( getName().local8Bit());	
	
   if( !desc || !num  )
      return;
		
   if( widget() )
   {
      kdDebug() << "constraint is " << desc->cap << endl;
      if( !active() )
	 kdDebug() << desc->name << " is not active now" << endl;
      
      if( !softwareSetable() )
	 kdDebug() << desc->name << " is not software setable" << endl;

      if( !active() || !softwareSetable() )
      {
	 kdDebug() << "Disabling widget " << getName() << " !" << endl;
	 widget()->setEnabled( false );
      }
      else
	 widget()->setEnabled( true );
   }
	
   /* first get mem if nothing is approbiate */
   if( !buffer )
   {
      kdDebug() << " *********** getting without space **********" << endl;
      // allocate memory
      switch( desc->type )
      {
	 case SANE_TYPE_INT:
	 case SANE_TYPE_FIXED:
	 case SANE_TYPE_STRING:  		
	    buffer = allocBuffer( desc->size );
	    break;
	 case SANE_TYPE_BOOL:
	    buffer = allocBuffer( sizeof( SANE_Word ) );
	    break;
	 default:
	    if( desc->size > 0 )
	    {
	       buffer = allocBuffer( desc->size );
	    }
      }
   }
#if 0	
   if( active())
   {
      if( (size_t) desc->size > buffer_size )
      {
	 qDebug( "SERIOUS ERROR: Buffer to small: %d (required) - %d %s !",
		desc->size, buffer_size, (const char*) name );
      }
  			
      SANE_Status sane_stat = sane_control_option( scanner_handle, *num,
						   SANE_ACTION_GET_VALUE, buffer, 0 );

      if( sane_stat != SANE_STATUS_GOOD )
      {
	 kdDebug() << "ERROR: Cant get value for " << getName() << ": " << sane_strstatus( sane_stat ) << endl;
      }
      else
      {
	 buffer_untouched = false;
	 kdDebug() << "Setting buffer untouched to FALSE" << endl;
      }
   }
#endif   
}


KScanOption::~KScanOption()
{
#ifdef MEM_DEBUG
   qDebug( "M: FREEing %d byte of option <%s>", buffer_size, (const char*) getName());
#endif
   if( buffer ) delete ( (char*) buffer );
}

bool KScanOption::valid( void ) const
{
  return( desc && 1 );
}

bool KScanOption::autoSetable( void )
{
  /* Refresh description */
  desc = getOptionDesc( name.local8Bit() );

  return( desc && ((desc->cap & SANE_CAP_AUTOMATIC) > 0 ) );
}

bool KScanOption::commonOption( void )
{
  /* Refresh description */
  desc = getOptionDesc( name.local8Bit() );

  return( desc && ((desc->cap & SANE_CAP_ADVANCED) == 0) );
}


bool KScanOption::active( void )
{
  bool ret = false;
  /* Refresh description */
  desc = getOptionDesc( name.local8Bit() );
  if( desc )
    ret = SANE_OPTION_IS_ACTIVE( desc->cap );
 			
  return( ret );
}

bool KScanOption::softwareSetable( void )
{
  /* Refresh description */
  desc = getOptionDesc( name.local8Bit() );
  if( desc )
  {
     if( SANE_OPTION_IS_SETTABLE(desc->cap) == SANE_TRUE )
	return( true );
  }
  return( false );
}


KSANE_Type KScanOption::type( void )
{
   KSANE_Type ret = INVALID_TYPE;
	
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
	 kdDebug() << "Cant set " << name << "  with type int" << endl;
   }
   if( ret )
   {
      buffer_untouched = false;
#ifdef APPLY_IN_SITU
      applyVal();
#endif
      
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
	 kdDebug() << "Cant set " << name << " with type double" << endl;
   }

   if( ret )
   {
      buffer_untouched = false;
#ifdef APPLY_IN_SITU
      applyVal();
#endif
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
      kdDebug() << "Cant set " << name << " with type int*" << endl;
	
    }

  if( ret && buffer ) {
    memcpy( buffer, qa.data(), desc->size );
    ret = true;
  }

  if( ret )
    {
      buffer_untouched = false;
#ifdef APPLY_IN_SITU
      applyVal();
#endif
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
   QCString c_string( strval.local8Bit() );

   if( ! desc ) return( false );

   /* On String-type the buffer gets malloced in Constructor */
   if( desc->type == SANE_TYPE_STRING )
   {
      kdDebug() << "Setting " << c_string << " as String" << endl;
   	
      if( buffer_size >= c_string.length() )
      {
	 memset( buffer, 0, buffer_size );
	 qstrncpy( (char*) buffer, (const char*) c_string, buffer_size );
	 ret = true;
      }
      else
      {
	 kdDebug() << "ERROR: Buffer for String " << c_string << " too small: " << buffer_size << "  < " << c_string.length() << endl;
      }
   } else {
      kdDebug() << "Cant set " << name << " with type string" << endl;
   }

   if( ret )
   {
      buffer_untouched = false;
#ifdef APPLY_IN_SITU
      applyVal();
#endif
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
	 kdDebug() << "Cant set " << name << " with type GammaTable" << endl;
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
#ifdef APPLY_IN_SITU
      applyVal();
#endif
#if 0
      emit( optionChanged( this ));
#endif
   }

   return( ret );
}

bool KScanOption::get( int *val ) const
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
      kdDebug() << "Cant get " << name << " to type int" << endl;
      return( false );
    }

  // qDebug( "option::get returns %d", *val );
  return( true );
}



const QString KScanOption::get( void ) const 
{

   QCString retstr;

   SANE_Word sane_word;

   
   if( !valid() || !getBuffer())
      return( "parametererror" );
			
   switch( desc->type )
   {
      case SANE_TYPE_BOOL:
	 sane_word = *((SANE_Word*)buffer);
	 if( sane_word == SANE_TRUE )
	    retstr = "true";
	 else	
	    retstr = "false";
	 break;
      case SANE_TYPE_STRING:
	 retstr = (const char*) getBuffer();
	 // retstr.sprintf( "%s", cc );
	 break;
      case SANE_TYPE_INT:
	 sane_word = *((SANE_Word*)buffer);
	 retstr.setNum( sane_word );
	 break;
      case SANE_TYPE_FIXED:
	 sane_word = (SANE_Word) SANE_UNFIX(*(SANE_Word*)buffer);
	 retstr.setNum( sane_word );
	 break;
	 
      default:
	 kdDebug() << "Cant get " << getName() << " to type String !" << endl;
	 retstr = "unknown";
   }
   kdDebug() << "option::get returns " << retstr << endl;
   return( retstr );
}


/* Caller needs to have the space ;) */
bool KScanOption::get( KGammaTable *gt ) const 
{
    if( gt )
    {
        gt->setAll( gamma, brightness, contrast );
        // gt->calcTable();
        return( true );
    }
    return( false );
}


QStrList KScanOption::getList( ) const
{
   if( ! desc ) return( false );
   const char	**sstring = 0;
   QStrList strList;

   if( desc->constraint_type == SANE_CONSTRAINT_STRING_LIST ) {
      sstring =  (const char**) desc->constraint.string_list;

      while( *sstring )
      {
	 // qDebug( "This is in the stringlist: %s", *sstring );
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
	 strList.append( s.local8Bit() );
      }
   }
   return( strList );
}


bool KScanOption::getRange( double *min, double *max, double *q ) const
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
      kdDebug() << "getRange: No range type " << desc->name << endl;
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
      w = KSaneSlider( parent, text );
      break;
    case GAMMA_TABLE:
      /* Widget Type is Slider */
      // w = KSaneSlider( parent, text );
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
 	
  /* Check if option is active, setEnabled etc. */
  slReload();
  if( w ) slRedrawWidget( this );
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


QWidget *KScanOption::KSaneSlider( QWidget *parent, const char *text )
{
  double min, max, quant;
  getRange( &min, &max, &quant );

  KScanSlider *slider = new KScanSlider( parent, text, min, max );
  /* Connect to the options change Slot */
  connect( slider, SIGNAL( valueChanged(int)), this,
	   SLOT( slWidgetChange(int)));

  return( slider );
}




void *KScanOption::allocBuffer( long size )
{
  if( size < 1 ) return( 0 );

#ifdef MEM_DEBUG
  qDebug( "M: Reserving %ld bytes of mem for <%s>", size, (const char*) getName() );
#endif
  
  void *r = new char[ size ];
  buffer_size = size;
  
  if( r ) memset( r, 0, size );

  return( r );
  
}



bool KScanOption::applyVal( void )
{
   bool res = true;
   int *idx = option_dic[ name ];

   if( *idx == 0 ) return( false );
   if( ! buffer )  return( false );

   SANE_Status stat = sane_control_option ( scanner_handle, *idx,
					    SANE_ACTION_SET_VALUE, buffer,
					    0 );
   if( stat != SANE_STATUS_GOOD )
   {
      kdDebug() << "Error in in situ appliance " << getName() << ": " << sane_strstatus( stat ) << endl;
      res = false;
   }
   else
   {
      kdDebug() << "IN SITU appliance " << getName() << ": OK" << endl;

   }
   return( res );
}	
#include "kscanoption.moc"