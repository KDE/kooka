/*
 *   kscan - a scanning program
 *   Copyright (C) 1998 Ivan Shvedunov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __MISCWIDGETS_H__
#define __MISCWIDGETS_H__

#include <qwidget.h>
#include <qlineedit.h>
#include <qlcdnum.h>
#include <qlabel.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qslider.h>
#include <stdlib.h>
#include <qframe.h>
#include <qpainter.h>
#include <qtabdialog.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>


class ImageHistogram: public QFrame {
	Q_OBJECT
public:
	ImageHistogram( const QImage *ps, int pcolor = 0,
		       QWidget *parent = NULL,char *name = NULL):
	  QFrame(parent,name),
	  img((QImage*) ps),color(pcolor)
	  { setFrameStyle(Box|Sunken); count(); }
public slots:
        void redraw()
	{ count(); repaint(); }
private:
	QImage  *img;
	int hits[256];
	int maxhits;
	int color;
	void count();
	void drawContents(QPainter *p);
};


class ImgScaleDialog : public QDialog {
   Q_OBJECT
public:
   ImgScaleDialog( QWidget *parent, int curr_sel = 100,
		   int last_custom = 100, const char *name = 0 );

public slots:
 void enableAndFocus( bool b ) {
    leCust->setEnabled( b ); leCust->setFocus();
 }

   void setSelValue( int val );
   int  getSelected( void ){ return( selected ); }
signals:
   void customScaleChange( int );
public slots:
   void customChanged( const QString& );
 private:
   QLineEdit *leCust;
   int selected;

   
// xslots:
#if 0
   void accept( ){ ;}
   void reject( ){ ;}
#endif
};


#endif
