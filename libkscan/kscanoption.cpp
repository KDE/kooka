/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>  

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
#include <qregexp.h>

#include <kdebug.h>

#include <unistd.h>
#include "kgammatable.h"
#include "kscandevice.h"
#include "kscanslider.h"
#include "kscanoptset.h"


// #define MIN_PREVIEW_DPI 20
// this is nowhere used!? Additionally, it's defined to 75 in kscandevice.cpp

/* switch to show some from time to time useful alloc-messages */
#undef MEM_DEBUG

#undef APPLY_IN_SITU



/** inline-access to the option descriptor, object to changes with global vars. **/
inline const SANE_Option_Descriptor *getOptionDesc( const QCString& name )
{
   int *idx = (*KScanDevice::option_dic)[ name ];
   
   const SANE_Option_Descriptor *d = 0;
   // debug( "<< for option %s >>", name );
   if ( idx && *idx > 0 )
   {
      d = sane_get_option_descriptor( KScanDevice::scanner_handle, *idx );
      // debug( "retrieving Option %s", d->name );
   }
   else
   {
      kdDebug(29000) << "no option descriptor for <" << name << ">" << endl;
      // debug( "Name survived !" );
   }
   // debug( "<< leaving option %s >>", name );

   return( d );
}



/* ************************************************************************ */
/* KScan Option                                                             */
/* ************************************************************************ */
KScanOption::KScanOption( const QCString& new_name ) :
   QObject()
{
   if( initOption( new_name ) )
   {
      int  *num = (*KScanDevice::option_dic)[ getName() ];	
      if( !num || !buffer )
	 return;

      SANE_Status sane_stat = sane_control_option( KScanDevice::scanner_handle, *num,
						   SANE_ACTION_GET_VALUE,
						   buffer, 0 );

      if( sane_stat == SANE_STATUS_GOOD )
      {
	 buffer_untouched = false;
      }
   }
   else
   {
      kdDebug(29000) << "Had problems to create KScanOption - initOption failed !" << endl;
   }
}


bool KScanOption::initOption( const QCString& new_name )
{
   desc = 0;
   if( new_name.isEmpty() ) return( false );

   name = new_name;
   desc = getOptionDesc( name );
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

	KScanOption *gtOption = (*KScanDevice::gammaTables)[ new_name ];
	if( gtOption )
	{
	   kdDebug(29000) << "Is older GammaTable!" << endl;
	   KGammaTable gt;
	   gtOption->get( &gt );
	
	   gamma = gt.getGamma();
	   contrast = gt.getContrast();
	   brightness = gt.getBrightness();
	}
	else
	{
	   // kdDebug(29000) << "Is NOT older GammaTable!" << endl;
	}
    }

   return desc;
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

   /* intialise */
   buffer = 0;
   buffer_size = 0;

   /* the widget is not copied ! */
   internal_widget = 0;
   
   if( ! ( desc && name ) )
   {
      kdWarning( 29000) << "Trying to copy a not healthy option (no name nor desc)" << endl;
      return;
   }
   
   if( so.buffer_untouched )
      kdDebug(29000) << "Buffer of source is untouched!" << endl;

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
	  kdWarning( 29000 ) << "unknown option type in copy constructor" << endl;
	  break;
    }
}


const QString KScanOption::configLine( void )
{
   QCString strval = this->get();
   kdDebug(29000) << "configLine returns <" << strval << ">" << endl;
   return( strval );
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

   delete internal_widget;
   internal_widget = so.internal_widget;

   if( buffer ) {
      delete [] buffer;
      buffer = 0;
   }

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

void KScanOption::slWidgetChange( const QCString& t )
{
    kdDebug(29000) << "Received WidgetChange for " << getName() << " (const QCString&)" << endl;
    set( t );
    emit( guiChange( this ) );
    // emit( optionChanged( this ));
}

void KScanOption::slWidgetChange( void )
{
    kdDebug(29000) << "Received WidgetChange for " << getName() << " (void)" << endl;
    /* If Type is bool, the widget is a checkbox. */
    if( type() == BOOL )
    {
	bool b = ((QCheckBox*) internal_widget)->isChecked();
	kdDebug(29000) << "Setting bool: " << b << endl;
	set( b );
    }
    emit( guiChange( this ) );
    // emit( optionChanged( this ));

}

void KScanOption::slWidgetChange( int i )
{
    kdDebug(29000) << "Received WidgetChange for " << getName() << " (int)" << endl;
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
   int  *num = (*KScanDevice::option_dic)[ getName() ];	
   desc = getOptionDesc( getName() );	
	
   if( !desc || !num  )
      return;
		
   if( widget() )
   {
      kdDebug(29000) << "constraint is " << desc->cap << endl;
      if( !active() )
	 kdDebug(29000) << desc->name << " is not active now" << endl;

      if( !softwareSetable() )
	 kdDebug(29000) << desc->name << " is not software setable" << endl;

      if( !active() || !softwareSetable() )
      {
	 kdDebug(29000) << "Disabling widget " << getName() << " !" << endl;
	 widget()->setEnabled( false );
      }
      else
	 widget()->setEnabled( true );
   }
	
   /* first get mem if nothing is approbiate */
   if( !buffer )
   {
      kdDebug(29000) << " *********** getting without space **********" << endl;
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

   if( active())
   {
      if( (size_t) desc->size > buffer_size )
      {
	 kdDebug(29000) << "ERROR: Buffer to small" << endl;
      }
      else
      {
	 SANE_Status sane_stat = sane_control_option( KScanDevice::scanner_handle, *num,
						      SANE_ACTION_GET_VALUE, buffer, 0 );

	 if( sane_stat != SANE_STATUS_GOOD )
	 {
	    kdDebug(29000) << "ERROR: Cant get value for " << getName() << ": " << sane_strstatus( sane_stat ) << endl;
	 } else {
	    buffer_untouched = false;
	    kdDebug(29000) << "Setting buffer untouched to FALSE" << endl;
	 }
      }
   }
}


KScanOption::~KScanOption()
{
#ifdef MEM_DEBUG
   qDebug( "M: FREEing %d byte of option <%s>", buffer_size, (const char*) getName());
#endif
}

bool KScanOption::valid( void ) const
{
  return( desc && 1 );
}

bool KScanOption::autoSetable( void )
{
  /* Refresh description */
  desc = getOptionDesc( name );

  return( desc && ((desc->cap & SANE_CAP_AUTOMATIC) > 0 ) );
}

bool KScanOption::commonOption( void )
{
  /* Refresh description */
  desc = getOptionDesc( name );

  return( desc && ((desc->cap & SANE_CAP_ADVANCED) == 0) );
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
  if( desc )
  {
     if( SANE_OPTION_IS_SETTABLE(desc->cap) == SANE_TRUE )
	return( true );
  }
  return( false );
}


KSANE_Type KScanOption::type( void ) const
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
	    {
	       /* FIXME ! Dies scheint nicht wirklich so zu sein */
	       if( desc->size == sizeof( SANE_Word ))
		  ret = RANGE;
	       else
		  ret = GAMMA_TABLE;
	    }
	    else if( desc->constraint_type == SANE_CONSTRAINT_NONE )
	    {
	       ret = SINGLE_VAL;
	    }
	    else if( desc->constraint_type == SANE_CONSTRAINT_WORD_LIST )
	    {
		ret = STR_LIST;
		// ret = GAMMA_TABLE;
	    }
	    else
	    {
	       ret = INVALID_TYPE;
	    }
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
   QMemArray<SANE_Word> qa;
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
	 kdDebug(29000) << "Cant set " << name << "  with type int" << endl;
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
   QMemArray<SANE_Word> qa;
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
	 kdDebug(29000) << "Cant set " << name << " with type double" << endl;
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
    bool   ret    = false;
    int    offset = 0;

    int word_size = desc->size / sizeof( SANE_Word ); /* add 1 in case offset is needed */
    QMemArray<SANE_Word> qa( 1+word_size );
#if 0
    if( desc->constraint_type == SANE_CONSTRAINT_WORD_LIST )
    {
	/* That means that the first entry must contain the size */
	kdDebug(29000) << "Size: " << size << ", word_size: " << word_size << ", descr-size: "<< desc->size << endl;
	qa[0] = (SANE_Word) 1+size;
	kdDebug(29000) << "set length field to " << qa[0] <<endl;
	offset = 1;
    }
#endif

    switch( desc->type )
    {
	case SANE_TYPE_INT:
	    for( int i = 0; i < word_size; i++ )
	    {
		if( i < size )
		    qa[offset+i] = (SANE_Word) *(val++);
		else
		    qa[offset+i] = (SANE_Word) *val;
	    }
	    ret = true;
	    break;

	    // Type fixed: Fill the whole buffer with that value
	case SANE_TYPE_FIXED:
	    for( int i = 0; i < word_size; i++ )
	    {
		if( i < size )
		    qa[offset+i] = SANE_FIX((double)*(val++));
		else
		    qa[offset+i] = SANE_FIX((double) *val );
	    }
	    ret = true;
	    break;
	default:
	    kdDebug(29000) << "Cant set " << name << " with type int*" << endl;
	
    }

    if( ret && buffer ) {
	int copybyte = desc->size;
	
	if( offset )
	    copybyte += sizeof( SANE_Word );

	kdDebug(29000) << "Copying " << copybyte << " byte to options buffer" << endl;
	
	memcpy( buffer, qa.data(), copybyte );
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

bool KScanOption::set( const QCString& c_string )
{
   bool ret = false;
   int  val = 0;
   
   if( ! desc ) return( false );

   /* Check if it is a gammatable. If it is, convert to KGammaTable and call
    *  the approbiate set method.
    */
   QRegExp re( "\\d+, \\d+, \\d+" );
   re.setMinimal(true);
   
   if( QString(c_string).contains( re ))
   {
      QStringList relist = QStringList::split( ", ", QString(c_string) );
      
      int brig = (relist[0]).toInt();
      int contr = (relist[1]).toInt();
      int gamm = (relist[2]).toInt();

      KGammaTable gt( brig, contr, gamm );
      ret = set( &gt );
      kdDebug(29000) << "Setting GammaTable with int vals " << brig << "|" << contr << "|" << gamm << endl;
      
      return( ret );
   }

   
   /* On String-type the buffer gets malloced in Constructor */
   switch( desc->type )
   {
       case SANE_TYPE_STRING:
	   kdDebug(29000) << "Setting " << c_string << " as String" << endl;
   	
	   if( buffer_size >= c_string.length() )
	   {
	       memset( buffer, 0, buffer_size );
	       qstrncpy( (char*) buffer, (const char*) c_string, buffer_size );
	       ret = true;
	   }
	   else
	   {
	       kdDebug(29000) << "ERROR: Buffer for String " << c_string << " too small: " << buffer_size << "  < " << c_string.length() << endl;
	   }
	   break;
       case SANE_TYPE_INT:
       case SANE_TYPE_FIXED:
	   kdDebug(29000) << "Type is INT or FIXED, try to set value <" << c_string << ">" << endl;
	   val = c_string.toInt( &ret );
	   if( ret ) 
	       set( &val, 1 );
	   else
	       kdDebug(29000) << "Conversion of string value failed!" << endl;
	   
	   break;
      case SANE_TYPE_BOOL:
	 kdDebug(29000) << "Type is BOOL, setting value <" << c_string << ">" << endl;
	 val = 0;
	 if( c_string == "true" ) val = 1;
	 set( val );
	 break;
       default:
	   kdDebug(29000) << "Type of " << name << " is " << desc->type << endl;
	   kdDebug(29000) << "Cant set " << name << " with type string" << endl;
	   break;
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
   kdDebug(29000) << "Returning " << ret << endl;
   return( ret );
}


bool KScanOption::set( KGammaTable *gt )
{
   if( ! desc ) return( false );
   bool ret = true;
   int size = gt->tableSize();
   SANE_Word *run = gt->getTable();

   int word_size = desc->size / sizeof( SANE_Word );
   QMemArray<SANE_Word> qa( word_size );
   kdDebug(29000) << "KScanOption::set for Gammatable !" << endl;
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
	 kdDebug(29000) << "Cant set " << name << " with type GammaTable" << endl;
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
      kdDebug(29000) << "Cant get " << name << " to type int" << endl;
      return( false );
    }

  // qDebug( "option::get returns %d", *val );
  return( true );
}



QCString KScanOption::get( void ) const
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
	 kdDebug(29000) << "Cant get " << getName() << " to type String !" << endl;
	 retstr = "unknown";
   }

   /* Handle gamma-table correctly */
   ;
   if( type() == GAMMA_TABLE )
   {
      retstr.sprintf( "%d, %d, %d", gamma, brightness, contrast );
   }

   kdDebug(29000) << "option::get returns " << retstr << endl;
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



bool KScanOption::getRangeFromList( double *min, double *max, double *q ) const
{
   if( !desc ) return( false );
   bool ret = true;

   if ( desc->constraint_type == SANE_CONSTRAINT_WORD_LIST ) {
	    // Try to read resolutions from word list
	    kdDebug(29000) << "Resolutions are in a word list" << endl;
            const SANE_Int *sint =  desc->constraint.word_list;
            int amount_vals = *sint; sint ++;
	    double value;
	    *min = 0;
	    *max = 0;
	    *q = -1; // What is q?

            for( int i=0; i < amount_vals; i++ ) {
		if( desc->type == SANE_TYPE_FIXED ) {
		    value = (double) SANE_UNFIX( *sint );
		} else {
		    value = *sint;
		}
		if ((*min > value) || (*min == 0)) *min = value;
		if ((*max < value) || (*max == 0)) *max = value;
		
		if( min != 0 && max != 0 && max > min ) {
		    double newq = max - min;
		    *q = newq;
		}
		sint++;
            }
   } else {
      kdDebug(29000) << "getRangeFromList: No list type " << desc->name << endl;
      ret = false;
    }
    return( ret );
}



bool KScanOption::getRange( double *min, double *max, double *q ) const
{
   if( !desc ) return( false );
   bool ret = true;

   if( desc->constraint_type == SANE_CONSTRAINT_RANGE ||
	   desc->constraint_type == SANE_CONSTRAINT_WORD_LIST ) {
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
      kdDebug(29000) << "getRange: No range type " << desc->name << endl;
      ret = false;
   }
   return( ret );
}

QWidget *KScanOption::createWidget( QWidget *parent, const QString& w_desc,
                                    const QString& tooltip )
{
  QStrList list;
  if( ! valid() )
  {
     kdDebug(29000) << "The option is not valid!" << endl;
     return( 0 );
  }
  QWidget *w = 0;
  /* free the old widget */
  delete internal_widget;
  internal_widget = 0;
 	
  /* check for text */
  QString text = w_desc;
  if( text.isEmpty() && desc ) {
    text = QString::fromLocal8Bit( desc->title );
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
      kdDebug(29000) << "can not create widget for SINGLE_VAL!" << endl;
      break;
    case RANGE:
      /* Widget Type is Slider */
      w = KSaneSlider( parent, text );
      break;
    case GAMMA_TABLE:
      /* Widget Type is Slider */
      // w = KSaneSlider( parent, text );
      kdDebug(29000) << "can not create widget for GAMMA_TABLE!" << endl;
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
       kdDebug(29000) << "returning zero for default widget creation!" << endl;
      w  = 0;
      break;
    }
 	
  if( w )
    {
      internal_widget = w;
      connect( this, SIGNAL( optionChanged( KScanOption*)),
	       SLOT( slRedrawWidget( KScanOption* )));
      QString tt = tooltip;
      if( tt.isEmpty() && desc )
	tt = QString::fromLocal8Bit( desc->desc );
 			
      if( !tt.isEmpty() )
	QToolTip::add( internal_widget, tt );
    }
 	
  /* Check if option is active, setEnabled etc. */
  slReload();
  if( w ) slRedrawWidget( this );
  return( w );
}


QWidget *KScanOption::comboBox( QWidget *parent, const QString& text )
{
  QStrList list = getList();

  KScanCombo *cb = new KScanCombo( parent, text, list);

  connect( cb, SIGNAL( valueChanged( const QCString& )), this,
	   SLOT( slWidgetChange( const QCString& )));

  return( cb );
}


QWidget *KScanOption::entryField( QWidget *parent, const QString& text )
{
  KScanEntry *ent = new KScanEntry( parent, text );
  connect( ent, SIGNAL( valueChanged( QCString )), this,
	   SLOT( slWidgetChange( QCString )));
	
  return( ent );
}


QWidget *KScanOption::KSaneSlider( QWidget *parent, const QString& text )
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
   int *idx = (*KScanDevice::option_dic)[ name ];

   if( *idx == 0 ) return( false );
   if( ! buffer )  return( false );

   SANE_Status stat = sane_control_option ( KScanDevice::scanner_handle, *idx,
					    SANE_ACTION_SET_VALUE, buffer,
					    0 );
   if( stat != SANE_STATUS_GOOD )
   {
      kdDebug(29000) << "Error in in situ appliance " << getName() << ": " << sane_strstatus( stat ) << endl;
      res = false;
   }
   else
   {
      kdDebug(29000) << "IN SITU appliance " << getName() << ": OK" << endl;

   }
   return( res );
}	
#include "kscanoption.moc"
