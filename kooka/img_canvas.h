/***************************************************************************
                          img_canvas.h  -  description                              
                             -------------------                                         
    begin                : Sun Dec 12 1999                                           
    copyright            : (C) 1999 by Klaas Freitag                         
    email                : freitag@suse.de
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

#ifndef __IMG_CANVAS_H__
#define __IMG_CANVAS_H__

#include <qwidget.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qcursor.h>
#include <qrect.h>
#include <stdlib.h>
#include <qsize.h>
#include <qwmatrix.h>
#include <qscrollview.h>
#include <qstrlist.h>
#include <qpopupmenu.h>

enum preview_state {
	MOVE_NONE,
	MOVE_TOP_LEFT,
	MOVE_TOP_RIGHT,
	MOVE_BOTTOM_LEFT,
	MOVE_BOTTOM_RIGHT,
	MOVE_LEFT,
	MOVE_RIGHT,
	MOVE_TOP,
	MOVE_BOTTOM,
	MOVE_WHOLE
};

enum cursor_type {
	CROSS,
	VSIZE,
	HSIZE,
	BDIAG,
	FDIAG,
	ALL,
	HREN
};

const int MIN_AREA_WIDTH = 3;
const int MIN_AREA_HEIGHT = 3;
const int delta = 3;
#ifdef __PREVIEW_CPP__
int max_dpi = 600;
#else
extern int max_dpi;
#endif



class ImageCanvas: public QScrollView
{
   Q_OBJECT
public:
   ImageCanvas( QWidget *parent = 0,
		const QImage *start_image = 0,
		const char *name = 0);
   ~ImageCanvas( );
   int getBrightness() 	        { return brightness; }
   int getContrast() 	        { return contrast; }
   int getGamma() 		{ return gamma; }

   bool hasImage( void ) 	{ return acquired; }
   QRect sel( void );
   int getScaleFactor()         { return( scale_factor ); }
   
   const QImage *rootImage( void )
                                { return( image );}  

   enum{ ID_POP_ZOOM, ID_POP_CLOSE, ID_FIT_WIDTH,
	    ID_FIT_HEIGHT, ID_ORIG_SIZE } PopupIDs;
				    
   bool selectedImage( QImage* );
   
public slots:
   void setBrightness(int b)
      { brightness = b; }
   void setContrast(int c)
      { contrast = c; }
   void setGamma(int c)
      { gamma = c;  }
   void toggleAspect( int aspect_in_mind )
      {
	 maintain_aspect = aspect_in_mind;
	 repaint();
      }	
   void newImage( QImage *new_image );
   void deleteView( QImage *);
   void newRectSlot();
   void newRectSlot( QRect newSel );
   void noRectSlot( void );
   void setScaleFactor( int i );

   void handle_popup(int item );

signals:
   void noRect( void );
   void newRect( void );
   void newRect( QRect );
   void scalingRequested();
   void closingRequested();

protected:
   void drawContents( QPainter * p, int clipx, int clipy, int clipw, int cliph );

   void timerEvent(QTimerEvent *);
   void viewportMousePressEvent(QMouseEvent *);
   void viewportMouseReleaseEvent(QMouseEvent *);
   void viewportMouseMoveEvent(QMouseEvent *);
	
   void resizeEvent( QResizeEvent * event );
	
private:
   QStrList      urls;

   void          createContextMenu( void );
   void          showContextMenu( QPoint p );
   
   const QImage	*image;

   QWMatrix	 scale_matrix;
   QWMatrix	 inv_scale_matrix;
   QPixmap       *pmScaled;
   float	 used_yscaler;
   float	 used_xscaler;
   QPopupMenu    *contextMenu;
   bool		 maintain_aspect;

   int	         timer_id;
   int	         scale_factor;
   QRect 	 *selected;
   int 		 gamma,brightness,contrast;
   preview_state moving;
   int 		 cr1,cr2;
   int 		 lx,ly;
   bool 	 acquired;

   /* private functions for the running ant */
   void drawHAreaBorder(QPainter &p,int x1,int x2,int y,int r = FALSE);
   void drawVAreaBorder(QPainter &p,int x,int y1,int y2,int r = FALSE);
   void drawAreaBorder(QPainter *p,int r = FALSE);
   void update_scaled_pixmap( void );
	
   preview_state classifyPoint(int x,int y);
};

#endif
