/***************************************************************************
                          ksaneslider.h  -  description                              
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


#ifndef KSANESLIDER_H
#define KSANESLIDER_H

#include <qframe.h>
#include <qslider.h>
#include <qlineedit.h>
#include <qstrlist.h>
#include <qcombobox.h>
#include <qlabel.h>
/**
  *@author Klaas Freitag
  */

class KSANESlider : public QFrame  {
	Q_OBJECT
public: 
	KSANESlider( QWidget *parent, const char *text, double min, double max );
	~KSANESlider();

public slots:
	void 			slSetSlider( int );
	void        slSliderChange( int );
	void        setEnabled( bool b );
signals:
	void 	 	   valueChanged( int );
		
private:
	int         slider_val;
	QSlider 		*slider;
	QLabel      *l1, *numdisp;	
};


class KSANEEntry : public QFrame {
	Q_OBJECT
public:
	KSANEEntry( QWidget *parent, const char *text );
	// ~KSANEEntry();

public slots:
	void 			slSetEntry( const char* );
	void        slEntryChange( const QString& );
	void        setEnabled( bool b ){ if( entry) entry->setEnabled( b ); }
signals:
	void 	 	   valueChanged( const char* );
		
private:
	QLineEdit 		*entry;
	
};

	
class KSANECombo : public QFrame {
	Q_OBJECT
public:
	KSANECombo( QWidget *parent, const char *text, QStrList list );
	// ~KSANECombo();

public slots:
	void 			slSetEntry( const QString &);
	void        slComboChange( const QString & );
	void        setEnabled( bool b){ if(combo) combo->setEnabled( b ); };
	
signals:
	void 	 	   valueChanged( const char* );
		
private:
	QComboBox 	*combo;
	QStrList    combolist;
	
};

#endif
