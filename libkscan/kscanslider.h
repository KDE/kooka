/***************************************************************************
                          kscanslider.h  -  description
                             -------------------
    begin                : Wed Jan 5 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef KSCANSLIDER_H
#define KSCANSLIDER_H

#include <qframe.h>
#include <qslider.h>
#include <qlineedit.h>
#include <qstrlist.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qhbox.h>
/**
  *@author Klaas Freitag
  */

class KScanSlider : public QFrame
{
   Q_OBJECT
   Q_PROPERTY( int slider_val READ value WRITE slSetSlider )
      
public:
   KScanSlider( QWidget *parent, const QString& text,
		double min, double max );
   ~KScanSlider();


   int value( ) const
      { return( slider->value()); }
   
public slots:
   void		slSetSlider( int );
   void		slSliderChange( int );
   void		setEnabled( bool b );

   signals:
   void		valueChanged( int );
		
private:
   QSlider	*slider;
   QLabel	*l1, *numdisp;

   class KScanSliderPrivate;
   KScanSliderPrivate *d;
   
};


class KScanEntry : public QFrame
{
   Q_OBJECT
   Q_PROPERTY( QString text READ text WRITE slEntryChange )
      
public:
   KScanEntry( QWidget *parent, const QString& text );
   // ~KScanEntry();

   QString text( ) const;

public slots:
   void		slSetEntry( const QString& );
   void		setEnabled( bool b ){ if( entry) entry->setEnabled( b ); }
   void		slEntryChange( const QString& );
      
protected slots:
   void         slReturnPressed( void );

signals:
   void		valueChanged( const QCString& );
   void         returnPressed( const QCString& );
   
private:
   QLineEdit 	*entry;

   class KScanEntryPrivate;
   KScanEntryPrivate *d;
	
};

	
class KScanCombo : public QHBox
{
   Q_OBJECT
   Q_PROPERTY( QString cbEntry READ currentText WRITE slSetEntry )
      
public:
   KScanCombo( QWidget *parent, const QString& text, const QStrList& list );
   // ~KScanCombo();

   QString      currentText( ) const;
   QString      text( int i ) const;
   int  	count( ) const;

public slots:
   void		slSetEntry( const QString &);
   void		slComboChange( const QString & );
   void		setEnabled( bool b){ if(combo) combo->setEnabled( b ); };
   void         slSetIcon( const QPixmap&, const QString& );
   void         setCurrentItem( int i );

private slots:
   void         slFireActivated( int);

signals:
   void		valueChanged( const QCString& );
   void         activated(int);

private:
   QComboBox	*combo;
   QStrList	combolist;

   class KScanComboPrivate;
   KScanComboPrivate *d;
};

#endif
