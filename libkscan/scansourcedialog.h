/***************************************************************************
                          scansourcedialog.h  -  description                              
                             -------------------                                         
    begin                : Sun Jan 16 2000                                           
    copyright            : (C) 2000 by Klaas Freitag                         
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#ifndef SCANSOURCEDIALOG_H
#define SCANSOURCEDIALOG_H
#include <qwidget.h>
#include <kdialogbase.h>
#include <qstrlist.h>
#include <qstring.h>
#include <qcombobox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

/**
  *@author Klaas Freitag
  */

typedef enum { ADF_OFF, ADF_SCAN_ALONG, ADF_SCAN_ONCE } ADF_BEHAVE;

class KScanCombo;

class ScanSourceDialog : public KDialogBase
{
   Q_OBJECT
public: 
   ScanSourceDialog( QWidget *parent, const QStrList, ADF_BEHAVE );
   ~ScanSourceDialog();

   // void 	fillWithSources( QStrList *list );	
   QString 	getText( void ) const;

   ADF_BEHAVE 	getAdfBehave( void ) const
      { return( adf ); }
	
	
public slots:
   void        	slNotifyADF( int );	
   void    	slChangeSource( int );
   int          sourceAdfEntry( void ) const;
   void         slSetSource( const QString source );
   
private:

   KScanCombo    *sources;
   QButtonGroup  *bgroup;
   QRadioButton  *rb0, *rb1;
   ADF_BEHAVE    adf;
   bool          adf_enabled;

   class ScanSourceDialogPrivate;
   ScanSourceDialogPrivate *d;
};

#endif
