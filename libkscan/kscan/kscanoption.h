/***************************************************************************
	     kscanoption.h - Scanner option class
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


#ifndef _KSCANOPTION_H_
#define _KSCANOPTION_H_

#include <qwidget.h>
#include <qdict.h>
#include <qwidget.h>
#include <qobject.h>
#include <qstrlist.h>
#include <qsocketnotifier.h>

extern "C" {
#include <sane.h>
#include <saneopts.h>
}


typedef enum { KSCAN_OK, KSCAN_ERROR, KSCAN_ERR_NO_DEVICE,
	       KSCAN_ERR_BLOCKED, KSCAN_ERR_NO_DOC, KSCAN_ERR_PARAM,
               KSCAN_ERR_OPEN_DEV, KSCAN_ERR_CONTROL, KSCAN_ERR_EMPTY_PIC,
               KSCAN_ERR_MEMORY, KSCAN_ERR_SCAN, KSCAN_UNSUPPORTED,
               KSCAN_RELOAD, KSCAN_CANCELLED }
KScanStat;

typedef enum { INVALID_TYPE, BOOL, SINGLE_VAL, RANGE, GAMMA_TABLE, STR_LIST, STRING }
KSANE_Type;

class KGammaTable;


/**
 *  This is KSCANOption, a class which holds a single scanner option.
 *
 *  @short KSCANOption
 *  @author Klaas Freitag
 *  @version 0.1alpha
 *
 */

/**
 *  KSCANOption
 *
 *  is a help class for accessing the scanner options.
 **/


class KScanOption:public QObject
{
  Q_OBJECT
public:
  KScanOption( const char *new_name );
  KScanOption( const KScanOption& so );
  ~KScanOption();

  bool initialised( void ){ return( ! buffer_untouched );};
  bool valid( void ) const;
  bool autoSetable( void );
  bool commonOption( void );
  bool active( void );
  bool softwareSetable();
  KSANE_Type type( void );

  bool set( int val );
  bool set( double val );
  bool set( int *p_val, int size );
  bool set( QString );
  bool set( bool b ){ if( b ) return(set( (int)(1==1) )); else return( set( (int) (1==0) )); }
  bool set( KGammaTable  *gt );
  bool set( const char* );
	
  bool get( int* ) const;
  bool get( KGammaTable* ) const;
  const QString get( void ) const;

  QWidget *createWidget( QWidget *parent, const char *w_desc=0,
			 const char *tooltip=0  );

  /* Operators */
  const KScanOption& operator= (const KScanOption& so );

  // Possible Values
  QStrList   getList() const;
  bool       getRange( double*, double*, double* ) const;

  QString    getName() const { return( name ); }
  void *     getBuffer() const { return( buffer ); }
  QWidget   *widget( ) const { return( internal_widget ); }
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
   **/
  KSANE_Type typeToSet( const char* name );

  /**
   *  returns a string describing the unit of given the option.
   *  @return the unit description, e.g. mm
   *  @param name: the name of a option from a returned option-List
   **/
  QString unitOf( const char *name );

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
  void		  slWidgetChange( const char* );
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
  bool       initOption( const char *new_name );
  void       *allocBuffer( long );

  QWidget    *entryField ( QWidget *parent, const char *text );
  QWidget    *KSaneSlider( QWidget *parent, const char *text );
  QWidget    *comboBox   ( QWidget *parent, const char *text );
	
  const      SANE_Option_Descriptor *desc;
  QString    name;
  void       *buffer;
  QWidget    *internal_widget;
  bool       buffer_untouched;
  size_t     buffer_size;

  /* For gamma-Tables remeber gamma, brightness, contrast */
  int        gamma, brightness, contrast;
};



#endif
