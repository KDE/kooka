/***************************************************************************
                   miscwidgets.h - some usefull dialogs and widgets.
                             -------------------                                         
    begin                : ?
    copyright            : (C) 1999 by Klaas Freitag
                               based on work of Ivan Shvedunov          
    email                : freitag@suse.de

    $Id$
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
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
#include <qimage.h>
#include <qpixmap.h>
#include <qslider.h>
#include <stdlib.h>
#include <qframe.h>
#include <qpainter.h>
#include <qcheckbox.h>
#include <kdialogbase.h>
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



/* ----------------------------------------------------------------------
 * The ImgScaleDialg is a small dialog to be used by the image canvas. It
 * allows the user to select a zoom factor in percent, either in steps
 * or as a custom value.
 */
class ImgScaleDialog : public KDialogBase{
   Q_OBJECT
public:
   ImgScaleDialog( QWidget *parent, int curr_sel = 100,
		   const char *name = 0 );

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
};


/**
 *  A small dialog that allows the user to enter a string. Currently
 *  used to ask the user for a new directory name for the packager.
 */
class EntryDialog : public KDialogBase {
public: 
   EntryDialog( QWidget *parent, QString caption, const QString text );
	~EntryDialog();
	
	QString getText( void );
	
private:
	QLineEdit *entry;
};



#endif
