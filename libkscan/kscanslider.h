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

class KScanSlider : public QFrame  {
	Q_OBJECT
public: 
	KScanSlider( QWidget *parent, const char *text, double min, double max );
	~KScanSlider();

public slots:
	void		slSetSlider( int );
	void		slSliderChange( int );
	void		setEnabled( bool b );

   int value( void ) const
      { return( slider->value()); }
signals:
	void		valueChanged( int );
		
private:
	int		slider_val;
	QSlider		*slider;
	QLabel		*l1, *numdisp;	
};


class KScanEntry : public QFrame {
	Q_OBJECT
public:
	KScanEntry( QWidget *parent, const char *text );
	// ~KScanEntry();

public slots:
	void		slSetEntry( const char* );
	void		slEntryChange( const QString& );
	void		setEnabled( bool b ){ if( entry) entry->setEnabled( b ); }

   QString text( void ) const
      { return ( entry->text() ); }
signals:
	void		valueChanged( const char* );
		
private:
	QLineEdit 		*entry;
	
};

	
class KScanCombo : public QHBox
{
   Q_OBJECT
public:
   KScanCombo( QWidget *parent, const QString text, const QStrList& list );
   // ~KScanCombo();

   QString      currentText( void ) const;
   QString      text( int i ) const;
   int  	count( void ) const;

public slots:
   void		slSetEntry( const QString &);
   void		slComboChange( const QString & );
   void		setEnabled( bool b){ if(combo) combo->setEnabled( b ); };
   void         slSetIcon( const QPixmap&, const QString );
   void         setCurrentItem( int i );

private slots:
   void         slFireActivated( int);
  
signals:
   void		valueChanged( const char* );
   void         activated(int);
   
private:
   QComboBox	*combo;
   QStrList	combolist;
};

#endif
