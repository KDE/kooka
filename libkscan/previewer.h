/***************************************************************************
                          previewer.h  -  description                              
                             -------------------                                         
    begin                : Thu Jun 22 2000                                           
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


#ifndef PREVIEWER_H
#define PREVIEWER_H

#include <qwidget.h>
#include <qlayout.h>
#include <qimage.h>
#include <qrect.h>
#include <qcombobox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qpoint.h>

#include <kruler.h>

/**
  *@author Klaas Freitag
  */
class ImageCanvas;

class Previewer : public QWidget
{
   Q_OBJECT
public: 
   Previewer(QWidget *parent=0, const char *name=0);
   ~Previewer();
	
   ImageCanvas *getImageCanvas( void ){ return( img_canvas ); }
	
public slots:
   void newImage( QImage* );
   void slFormatChange( int id );
   void slOrientChange(int);
   void slSetDisplayUnit( KRuler::MetricStyle unit );
   void setScanSize( int w, int h, KRuler::MetricStyle unit );
   void slCustomChange( void );
   void slNewDimen(QRect r);

signals:
   void newRect( QRect );
   void noRect( void );
   void setScanWidth(const QString&);
   void setScanHeight(const QString&);

private:
   QPoint calcPercent( int, int );
	
   QHBoxLayout *layout;
   ImageCanvas *img_canvas;
   QComboBox   *pre_format_combo;
   QRect       last_custom;
   QArray<QCString> format_ids;
   QButtonGroup * bgroup;
   QRadioButton * rb1;
   QRadioButton * rb2;	
	
   int landscape_id, portrait_id;
   double overallWidth, overallHeight;
   KRuler::MetricStyle sizeUnit;
   KRuler::MetricStyle displayUnit;
   bool isCustom;

   class PreviewerPrivate;
   PreviewerPrivate *d;
};

#endif
