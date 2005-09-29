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

#ifndef _KSCANOPTION_H_
#define _KSCANOPTION_H_

#include <qwidget.h>
#include <qdict.h>
#include <qobject.h>
#include <qstrlist.h>
#include <qsocketnotifier.h>
#include <qdatastream.h>

extern "C" {
#include <sane/sane.h>
#include <sane/saneopts.h>
}


typedef enum { KSCAN_OK, KSCAN_ERROR, KSCAN_ERR_NO_DEVICE,
	       KSCAN_ERR_BLOCKED, KSCAN_ERR_NO_DOC, KSCAN_ERR_PARAM,
               KSCAN_ERR_OPEN_DEV, KSCAN_ERR_CONTROL, KSCAN_ERR_EMPTY_PIC,
               KSCAN_ERR_MEMORY, KSCAN_ERR_SCAN, KSCAN_UNSUPPORTED,
               KSCAN_RELOAD, KSCAN_CANCELLED, KSCAN_OPT_NOT_ACTIVE }
KScanStat;

typedef enum { INVALID_TYPE, BOOL, SINGLE_VAL, RANGE, GAMMA_TABLE, STR_LIST, STRING }
KSANE_Type;

class KGammaTable;


/**
 *  This is KScanOption, a class which holds a single scanner option.
 *
 *  @short KScanOption
 *  @author Klaas Freitag
 *  @version 0.1alpha
 *
 *  is a help class for accessing the scanner options.
 **/


class KScanOption : public QObject
{
  Q_OBJECT

public:
  /**
   * creates a new option object for the named option. After that, the
   * option is valid and contains the correct value retrieved from the
   * scanner.
   **/
  KScanOption( const QCString& new_name );

  /**
   * creates a KScanOption from another
   **/
  KScanOption( const KScanOption& so );
  ~KScanOption();

   /**
    * tests if the option is initialised. It is initialised, if the value
    * for the option was once read from the scanner
    **/
  bool initialised( void ){ return( ! buffer_untouched );}

   /**
    * checks if the option is valid, means that the option is known by the scanner
    **/
  bool valid( void ) const;

   /**
    * checks if the option is auto setable, what means, the scanner can cope
    * with the option automatically.
    **/
  bool autoSetable( void );

   /**
    * checks if the option is a so called common option. All frontends should
    * provide gui elements to change them.
    **/
  bool commonOption( void );

   /**
    * checks if the option is active at the moment. Note that this changes
    * on runtime due to the settings of mode, resolutions etc.
    **/
  bool active( void );

   /**
    * checks if the option is setable by software. Some scanner options can not
    * be set by software.
    **/
  bool softwareSetable();

   /**
    * returns the type the option is. Type is one of
    * INVALID_TYPE, BOOL, SINGLE_VAL, RANGE, GAMMA_TABLE, STR_LIST, STRING
    **/
  KSANE_Type type( void ) const;

   /**
    * set the option depending on the type
    **/
  bool set( int val );
  bool set( double val );
  bool set( int *p_val, int size );
  bool set( const QCString& );
  bool set( bool b ){ if( b ) return(set( (int)(1==1) )); else return( set( (int) (1==0) )); }
  bool set( KGammaTable  *gt );

   /**
    * retrieves the option value, depending on the type.
    **/
  bool get( int* ) const;
  bool get( KGammaTable* ) const;
  QCString get( void ) const;

   /**
    * This function creates a widget for the scanner option depending
    * on the type of the option.
    *
    * For boolean options, a checkbox is generated. For ranges, a KSaneSlider
    * is generated.
    *
    * For a String list such as mode etc., a KScanCombo is generated.
    *
    * For option type string and gamma table, no widget is generated yet.
    *
    * The widget is maintained completely by the kscanoption object.
    *
    **/

  QWidget *createWidget( QWidget *parent,
			 const QString& w_desc = QString::null,
			 const QString& tooltip = QString::null );

  /* Operators */
  const KScanOption& operator= (const KScanOption& so );

   const QString configLine( void );

   
  // Possible Values
  QStrList    getList() const;
  bool        getRangeFromList( double*, double*, double* ) const;
  bool        getRange( double*, double*, double* ) const;

  QCString    getName() const { return( name ); }
  void *      getBuffer() const { return( buffer ); }
  QWidget     *widget( ) const { return( internal_widget ); }
  /**
   *  returns the type of the selected option.
   *  This might be SINGLE_VAL, VAL_LIST, STR_LIST, GAMMA_TABLE,
   *  RANGE or BOOL
   *
   *  You may use the information returned to decide, in which way
   *  the option is to set.
   *
   *  A SINGLE_VAL is returned in case the value is represented by a
   *  single number, e.g. the resoltion.
   *
   *  A VAL_LIST is returned in case the value needs to be set by
   *  a list of numbers. You need to check the size to find out,
   *  how many numbers you have to
   *  @param name: the name of a option from a returned option-List
   *  return a option type.

   ### not implemented at all?
   **/
  KSANE_Type typeToSet( const QCString& name );

  /**
   *  returns a string describing the unit of given the option.
   *  @return the unit description, e.g. mm
   *  @param name: the name of a option from a returned option-List

   ###  not implemented at all?
   **/
  QString unitOf( const QCString& name );

public slots:
  void       slRedrawWidget( KScanOption *so );
  /**
   *	 that slot makes the option to reload the buffer from the scanner.
   */
  void       slReload( void );

protected slots:
  /**
   *  this slot is called if an option has a gui element (not all have)
   *  and if the value of the widget is changed.
   *  This is an internal slot.
   **/
  void		  slWidgetChange( void );
  void		  slWidgetChange( const QCString& );
  void		  slWidgetChange( int );
	
  signals:
  /**
   *  Signal emitted if a option changed for different reasons.
   *  The signal should be connected by outside objects.
   **/
  void      optionChanged( KScanOption*);
  /**
   *  Signal emitted if the option is set by a call to the set()
   *  member of the option. In the slot needs to be checked, if
   *  a widget exists, it needs to be set to the new value.
   *  This is a more internal signal
   **/
  void      optionSet( void );

  /**
   *  Signal called when user changes a gui - element
   */
  void      guiChange( KScanOption* );

private:
  bool       applyVal( void );
  bool       initOption( const QCString& new_name );
  void       *allocBuffer( long );

  QWidget    *entryField ( QWidget *parent, const QString& text );
  QWidget    *KSaneSlider( QWidget *parent, const QString& text );
  QWidget    *comboBox   ( QWidget *parent, const QString& text );
	
  const      SANE_Option_Descriptor *desc;
  QCString    name;
  void       *buffer;
  QWidget    *internal_widget;
  bool       buffer_untouched;
  size_t     buffer_size;

  /* For gamma-Tables remember gamma, brightness, contrast */
  int        gamma, brightness, contrast;

   class KScanOptionPrivate;
   KScanOptionPrivate *d;
};



#endif
