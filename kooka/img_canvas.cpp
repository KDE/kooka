/***************************************************************************
                          img_canvas.cpp  -  description                              
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
 *   Some code in this module was taken from a mail which was posted
 *   in the Qt-interest Archive by Christian Hernoux
 *   <christian.hernoux@univ-rouen.fr>. It is the redrawContents
 *   function which is *real cool* (I didnt get managed the redraw
 *   until I found his code) and the tiff-support.
 */
#include <qapplication.h>
#include <iostream.h>

#include <qfiledlg.h>
#include <qstring.h>
#include <qmsgbox.h>
#include <qscrollview.h>
#include <qpopupmenu.h>
#include <qlabel.h>
#include <qdict.h>

#include <kpixmapio.h>
#include <kdebug.h>

#define __IMG_CANVAS_CPP__
#include "img_canvas.h"
#include "miscwidgets.h"
#include "resource.h"

#define MIN(x,y) (x<y?x:y)


extern QDict<QPixmap> icons;


inline void debug_rect( const char *name, QRect *r )
{
	kdDebug() << (name ? name: "NONAME") << ": " << r->x() << ", " << r->y() << ", " << r->width() << ", " << r->height() << endl;
}



ImageCanvas::ImageCanvas(QWidget *parent, 
			 const QImage *start_image,
			 const char *name 	):
  QScrollView( parent, name )

{
  brightness       = 0;
  scale_factor     = 100; // means orignal size
  contrast         = 0;
  moving           = MOVE_NONE;
  gamma            = 100;
  maintain_aspect  = true;
  selected         = new QRect;
  timer_id         = 0;
  pmScaled         = 0;

  image            = start_image;

  QSize img_size;

  if( image && ! image->isNull() )
  {
    img_size = image->size();
    pmScaled = new QPixmap( img_size );

    KPixmapIO io;
    *pmScaled = io.convertToPixmap(*image);

    acquired = true;
  } else {
    img_size = size();
  }

  createContextMenu();
  update_scaled_pixmap();
  
  // timer-Start and stop
  connect( this, SIGNAL( newRect()), SLOT( newRectSlot()));
  connect( this, SIGNAL( noRect()),  SLOT( noRectSlot()));

  //zoomOut();scrollview/scrollview
  viewport()->setCursor( crossCursor );
  cr1 = 0;
  viewport()->setMouseTracking(TRUE);
  viewport()->setBackgroundMode(PaletteBackground);
  show();

}

ImageCanvas::~ImageCanvas()
{
    if( selected )    delete selected;
    if( pmScaled )    delete pmScaled;
    if( contextMenu ) delete contextMenu;
    noRectSlot();  /* Switches timer off */
 	
}

void ImageCanvas::deleteView( QImage *delimage )
{
    if( delimage == image )
        newImage( 0 );
}

void ImageCanvas::newImage( QImage *new_image )
{
  // dont free old image -> not yours
   image = new_image;

   if( pmScaled )
   {
        delete pmScaled;
        pmScaled = 0;
   }

   if( selected )
   {
      selected->setLeft( 0 );
      selected->setRight( 0 );
      selected->setTop( 0 );
      selected->setBottom( 0 );
      noRectSlot();
   }

   if( image )
   {						
	if( image->depth() == 1 ) {
		pmScaled = new QPixmap( image->size(), 1 );
	} else {
		int i = QPixmap::defaultDepth();
		pmScaled = new QPixmap( image->size(), i);
	}

	KPixmapIO io;
	*pmScaled = io.convertToPixmap(*image);

    	
    	acquired = true;
    	if( scale_factor != 0 )
            scale_factor = 100;
    	update_scaled_pixmap();
    	setContentsPos(0,0);
   } else {
  	kdDebug() << "New image called without image => deleting!" << endl;
  	image = 0;
	acquired = false;
   }


   kdDebug() << "going to repaint!" << endl;
   repaint( true );
}


void ImageCanvas::createContextMenu( void )
{
   contextMenu = new QPopupMenu();

   contextMenu->insertItem( *icons["mini-fitwidth"], I18N("fit to width"),  ID_FIT_WIDTH );
   contextMenu->insertItem( *icons["mini-fitheight"], I18N("fit to height"), ID_FIT_HEIGHT );
   contextMenu->insertItem( I18N("set zoom..."),   ID_POP_ZOOM );
   contextMenu->insertItem( I18N("original size"), ID_ORIG_SIZE );
   contextMenu->setCheckable( true );
   connect( contextMenu, SIGNAL( activated(int)), this, SLOT(handle_popup(int)));


}


void ImageCanvas::showContextMenu( QPoint p )
{
    if( ! contextMenu ) return;

    QSize is;
    if( image ) is = image->size();

    if( is.width() > 0 && is.height() > 0 )
    {
        // debug ( "opening popup!" );
        contextMenu->move( p );
        contextMenu->show();
    }
}


void ImageCanvas::handle_popup( int item )
{
   if( item < ID_POP_ZOOM || item > ID_ORIG_SIZE ) return;
   double scale;

   ImgScaleDialog *zoomDia;
   if( ! image ) return;


   switch( item )
   {
      case ID_POP_ZOOM:
          zoomDia = new ImgScaleDialog( this, getScaleFactor() );
          if( zoomDia->exec() )
          {
              int sf = zoomDia->getSelected();
	      QApplication::setOverrideCursor(waitCursor);
              setScaleFactor( sf );
              repaint( true);
          }
          delete zoomDia;
	  QApplication::restoreOverrideCursor();
	 // emit( scalingRequested());
	 break;
      case ID_ORIG_SIZE:
	 QApplication::setOverrideCursor(waitCursor);
	 setScaleFactor(100);
	 QApplication::restoreOverrideCursor();
      break;
      case ID_FIT_WIDTH:
	 QApplication::setOverrideCursor(waitCursor);
	 scale = 100*(width()-2) / image->width();
	 kdDebug() << "Scale ist " << scale << endl;
	 if( (scale/100 * image->height()) > height() )
	 {
	    /* substract for scrollbar */
	    scale = 100*(width() -25)/image->width();
	 }
	 setScaleFactor( scale );
	 repaint( true );
	 QApplication::restoreOverrideCursor();
      break;
      case ID_FIT_HEIGHT:
	 QApplication::setOverrideCursor(waitCursor);

	 scale = 100*(height()-2) / image->height();
	 kdDebug() << "Scale ist " << scale << endl;
	 if( (scale/100 * image->width()) > width() )
	 {
	    /* substract for scrollbar */
	    scale = 100*(height() -25)/image->height();
	 }
	 setScaleFactor( scale );
	 repaint( true );
	 QApplication::restoreOverrideCursor();
	 break;
      case ID_POP_CLOSE:
	 emit( closingRequested());
	 break;

      default: break;
   }

}

/**
 *   Returns the selected rect in tenth of percent, not in absolute
 *   sizes, eg. 500 means 50.0 % of original width/height.
 *   That makes it easier to work with different scales and units
 */
QRect ImageCanvas::sel( void )
{
    QRect retval;
	
    if( selected )
    {
	/* Get the size in real image pixels */
		
	// debug_rect( "PRE map", selected );
   	QRect mapped = inv_scale_matrix.map( (const QRect) *selected );
   	// debug_rect( "Postmap", &mapped );
   	if( mapped.x() > 0 )
	    retval.setLeft((int) (1000.0/( (double)image->width() / (double)mapped.x())));
	
	if( mapped.y() > 0 )
	    retval.setTop((int) (1000.0/( (double)image->height() / (double)mapped.y())));
	
	if( mapped.width() > 0 )
	    retval.setWidth((int) (1000.0/( (double)image->width() / (double)mapped.width())));
	
	if( mapped.height() > 0 )	
	    retval.setHeight((int)(1000.0/( (double)image->height() / (double)mapped.height())));
	
     }   	
   	// debug_rect( "sel() return", &retval );
     return( retval );
   
}

bool ImageCanvas::selectedImage( QImage *retImg )
{
   QRect r = sel();
   bool  result  = false;
   
   const QImage* entireImg = this->rootImage();

   if( entireImg )
   {
      QSize s = entireImg->size();

      int x = (s.width()  * r.x())/1000;
      int y = (s.height() * r.y())/1000;
      int w = (s.width()  * r.width())/1000;
      int h = (s.height() * r.height())/1000;

      if( w > 0 && h > 0 ) {
	 *retImg = entireImg->copy( x, y, w, h );
	 result = true;
      }
   }
   return(result);
   
}


void ImageCanvas::drawContents( QPainter * p, int clipx, int clipy, int clipw, int cliph )
{
    if( !pmScaled ) return;
    int x1 = 0;
    int y1 = 0;

    int x2 = pmScaled->width();
    int y2 = pmScaled->height();	

    if (x1 < clipx) x1=clipx;
    if (y1 < clipy) y1=clipy;
    if (x2 > clipx+clipw-1) x2=clipx+clipw-1;
    if (y2 > clipy+cliph-1) y2=clipy+cliph-1;

    // Paint using the small coordinates...
    // p->scale( used_xscaler, used_yscaler );
    // p->scale( used_xscaler, used_yscaler );
    if ( x2 >= x1 && y2 >= y1 ) {
    	p->drawPixmap( x1, y1, *pmScaled, x1, y1 ); //, clipw, cliph);
        // p->setBrush( red );
        // p->drawRect( x1, y1, clipw, cliph );
    }

        // p->fillRect(x1, y1, x2-x1+1, y2-y1+1, red);
 	
}

void ImageCanvas::timerEvent(QTimerEvent *)
{
   if(moving!=MOVE_NONE || !acquired ) return;
   cr1++;
   QPainter p(viewport());
   // p.setWindow( contentsX(), contentsY(), contentsWidth(), contentsHeight());
   drawAreaBorder(&p);
}

void ImageCanvas::newRectSlot( QRect newSel )
{
   if( ! selected ) selected = new QRect;
   QRect to_map;

   QPainter p(viewport());
   drawAreaBorder(&p,TRUE);

   selected->setWidth(0);
   selected->setHeight(0);
   emit( noRect() );
   int h = image->width();
   kdDebug() << "ImageCanvas got selection Rect: W=" << newSel.width() << ", H=" << newSel.height() << endl;
   to_map.setWidth(h * newSel.width() / 1000.0);
   to_map.setX( h * newSel.x() / 1000.0 );

   h = image->height();
   to_map.setHeight( h * newSel.height() / 1000.0 );
   to_map.setY( h * newSel.y() / 1000.0 );

   *selected = scale_matrix.map( to_map );
   emit( newRect( sel() ));
   newRectSlot();
}



void ImageCanvas::newRectSlot( )
{
   // printf( "Timer switched on !\n" );
   if( timer_id == 0 )
      timer_id = startTimer( 100 );
}

void ImageCanvas::noRectSlot( void )
{
   // printf( "Timer switched off !\n" );
   if( timer_id ) {
      killTimer( timer_id );
      timer_id = 0;
   }
}

void ImageCanvas::viewportMousePressEvent(QMouseEvent *ev)
{
   if( ! acquired || ! image ) return;
	
   if(ev->button()==LeftButton )
   {

        int cx = contentsX(), cy = contentsY();
        int x = lx = ev->x(),y = ly = ev->y();

	int ix, iy;
	scale_matrix.map( image->width(), image->height(), &ix, &iy );
     	if( x > ix-cx  || y > iy-cy ) return;

     	if( moving == MOVE_NONE )
     	{
		QPainter p( viewport());
		drawAreaBorder(&p,TRUE);
		moving = classifyPoint( x+cx ,y+cy);
    
		if(moving == MOVE_NONE)
		{ //Create new area
	   	    selected->setCoords( x+cx, y+cy, x+cx, y+cy );
	            moving = MOVE_BOTTOM_RIGHT;
		}
		drawAreaBorder(&p);
         }
         else printf("MouseReleaseEvent() missed\n");
    }
    else if( ev->button() == RightButton )
          showContextMenu( mapToGlobal(QPoint( ev->x(), ev->y())));
}


void ImageCanvas::viewportMouseReleaseEvent(QMouseEvent *ev)
{
  if(ev->button()!=LeftButton || !acquired ) return;

  //// debug( "Mouse Release at %d/%d", ev->x(), ev->y());
  if(moving!=MOVE_NONE) {
    QPainter p(viewport());
    drawAreaBorder(&p,TRUE);
    moving = MOVE_NONE;
    *selected = selected->normalize();

    if(selected->width () < MIN_AREA_WIDTH ||
       selected->height() < MIN_AREA_HEIGHT)
    {
       selected->setWidth(0);
       selected->setHeight(0);
       emit noRect();
       return;
    }
    
    drawAreaBorder(&p);
    emit newRect( sel() );
    emit newRect( );
  } else printf("mousePressEvent() missed\n");
}      


void ImageCanvas::viewportMouseMoveEvent(QMouseEvent *ev)
{
   if( ! acquired || !image) return;
   
   static cursor_type ps = HREN;
   int x = ev->x(), y = ev->y();
   int cx = contentsX(), cy = contentsY();

   // debug("Mouse Coord x: %d, y: %d | cx: %d, cy: %d", x,y, cx, cy );
   // dont draw out of the scaled Pixmap
   if( x < 0 ) x = 0;
   int ix, iy;
	scale_matrix.map( image->width(), image->height(), &ix, &iy );
   if( x >= ix ) return;

   if( y < 0 ) y = 0;
   if( y >= iy )  return;

   // debug( "cx, cy: %dx%d, x,y: %d, %d",
   //		 cx, cy, x, y );

#if 0
   if( moving == MOVE_NONE )
      moving = classifyPoint(x, y);
#endif
  switch( moving!=MOVE_NONE ? moving:classifyPoint(x+cx,y+cy) ) {
  case MOVE_NONE:
    if(ps!=CROSS) {
      viewport()->setCursor(crossCursor);
      ps = CROSS;
    }
    break;
  case MOVE_LEFT:
  case MOVE_RIGHT:
    if(ps!=HSIZE) {
      viewport()->setCursor(sizeHorCursor);
      ps = HSIZE;
    }
    break;
  case MOVE_TOP:
  case MOVE_BOTTOM:
    if(ps!=VSIZE) {
      viewport()->setCursor(sizeVerCursor);
      ps = VSIZE;
    }
    break;
  case MOVE_TOP_LEFT:
  case MOVE_BOTTOM_RIGHT:
    if(ps!=FDIAG) {
      viewport()->setCursor(sizeFDiagCursor);
      ps = FDIAG;
    }
    break;
  case MOVE_TOP_RIGHT:
  case MOVE_BOTTOM_LEFT:
    if(ps!=BDIAG) {
      viewport()->setCursor(sizeBDiagCursor);
      ps = BDIAG;
    }
    break;
  case MOVE_WHOLE:
    if(ps!=ALL) {
      viewport()->setCursor(sizeAllCursor);
      ps = ALL;
    }
  }
  //At ButtonRelease : normalize + clip
  if( moving!=MOVE_NONE ) {
  	 int mx, my;
    QPainter p(viewport());
    drawAreaBorder(&p,TRUE);
    switch(moving) {
    case MOVE_NONE: //Just to make compiler happy
    case MOVE_TOP_LEFT:
      selected->setLeft( x + cx );
    case MOVE_TOP:
      selected->setTop( y + cy );
      break;
    case MOVE_TOP_RIGHT:
      selected->setTop( y + cy );
    case MOVE_RIGHT:
      selected->setRight( x + cx );
      break;
    case MOVE_BOTTOM_LEFT:
      selected->setBottom( y + cy );
    case MOVE_LEFT:
      selected->setLeft( x + cx );
      break;
    case MOVE_BOTTOM_RIGHT:
      selected->setRight( x + cx );
    case MOVE_BOTTOM:
      selected->setBottom( y + cy );
      break;
    case MOVE_WHOLE:
    	if( selected )
    	{
    		// lx is the Last x-Koord from the run before (global)
    		mx = x-lx; my = y-ly;
    		/* Check if rectangle would run out of the image on right and bottom */
    		if( selected->x()+ selected->width()+mx >= ix-cx )
    		{
    			mx =  ix -cx - selected->width() - selected->x();
    			kdDebug() << "runs out !" << endl;
    		}
    		if( selected->y()+ selected->height()+my >= iy-cy )
    		{
    			my =  iy -cy - selected->height() - selected->y();
    			kdDebug() << "runs out !" << endl;
    		}

    		/* Check if rectangle would run out of the image on left and top */    			    			
    		if( selected->x() +mx < 0 )
    			mx =  -selected->x();
    		if( selected->y()+ +my < 0 )
    			my =  -selected->y();
			    		
    		x = mx+lx; y = my+ly;

    		selected->moveBy( mx, my );

    	}
    }
    drawAreaBorder(&p);
    lx = x;
    ly = y;
  }
}


void ImageCanvas::setScaleFactor( int i )
{
  // debug( "Setting Scalefactor to %d", i );
  scale_factor = i;
  update_scaled_pixmap();
}


void ImageCanvas::resizeEvent( QResizeEvent * event )
{
	QScrollView::resizeEvent( event );
	update_scaled_pixmap();
	
}

void ImageCanvas::update_scaled_pixmap( void )
{

  if( !pmScaled || !image)
  { 	// debug( "Pixmap px is null in Update_scaled" );
	return;
  }

  int scale = scale_factor;
  // debug( "Rescaling with Factor %d", scale );
  if( scale == 0 )
  {
     // do scaling to window-size
     used_yscaler = ((float)viewport()-> height()) / ((float)image->height());
     used_xscaler = ((float)viewport()-> width())  / ((float)image->width());
  }
  else
  {
     // scale as in scale in percent given (eg 100 = original size )
     used_xscaler = (double) scale / 100;
     used_yscaler = (double) scale / 100;
  }

  // reconvert the selection to orig size
  if( selected ) {
     *selected = inv_scale_matrix.map( (const QRect) *selected );
  }  

  scale_matrix.reset();                         // transformation matrix
  inv_scale_matrix.reset();

  
  if( !scale && maintain_aspect  ) {
     // printf( "Skaler: x: %f, y: %f\n", x_scaler, y_scaler );
     used_xscaler = used_yscaler <  used_xscaler ? 
		used_yscaler : used_xscaler;
     used_yscaler = used_xscaler;
  }

  scale_matrix.scale( used_xscaler, used_yscaler );  // define scale factors
  inv_scale_matrix = scale_matrix.invert();	// for redraw of selection

  if( selected ) {
     *selected = scale_matrix.map( (const QRect )*selected );
  }
  // px->convertFromImage( *image );
  KPixmapIO io;
  *pmScaled = io.convertToPixmap(*image);

  *pmScaled = pmScaled->xForm( scale_matrix );  // create scaled pixmap

  /* Resizing to 0,0 never may be dropped, otherwise there are problems
   * with redrawing of new images.
   */
  resizeContents( 0,0 );
  resizeContents( image->width() * used_xscaler,
                  image->height() * used_yscaler );

}




void ImageCanvas::drawHAreaBorder(QPainter &p,int x1,int x2,int y,int r)
{
	if( ! acquired || !image ) return;
	
  if(moving!=MOVE_NONE) cr2 = 0;
  int inc = 1;
  int cx = contentsX(), cy = contentsY();
  if(x2 < x1) inc = -1;

  if(!r) {
    if(cr2 & 4) p.setPen(black);
    else p.setPen(white);
  } else if(!acquired) p.setPen(QPen(QColor(150,150,150)));

  for(;;) {
    if(rect().contains(QPoint(x1,y))) {
      if( r && acquired ) {
	int re_x1, re_y;
	inv_scale_matrix.map( x1+cx, y+cy, &re_x1, &re_y );
	re_x1 = MIN( image->width()-1, re_x1 );
	re_y = MIN( image->height()-1, re_y );

	p.setPen( QPen( QColor( image->pixel(re_x1, re_y))));
      }	
      p.drawPoint(x1,y);
    }
    if(!r) {
      cr2++;
      cr2 &= 7;
      if(!(cr2&3)) {
			if(cr2&4) p.setPen(black);
			else p.setPen(white);
      }
    }
    if(x1==x2) break;
    x1 += inc;
  }

}

void ImageCanvas::drawVAreaBorder(QPainter &p, int x, int y1, int y2, int r )
{
	if( ! acquired || !image ) return;
  if( moving!=MOVE_NONE ) cr2 = 0;
  int inc = 1;
  if( y2 < y1 ) inc = -1;

  int cx = contentsX(), cy = contentsY();


  if( !r ) {
    if( cr2 & 4 ) p.setPen(black);
    else 
      p.setPen(white);
  } else 
    if( !acquired ) p.setPen( QPen( QColor(150,150,150) ) );
  
  for(;;) {
    if(rect().contains( QPoint(x,y1) )) {
      if( r && acquired ) {
	int re_y1, re_x;
	inv_scale_matrix.map( x+cx, y1+cy, &re_x, &re_y1 );
	re_x = MIN( image->width()-1, re_x );
	re_y1 = MIN( image->height()-1, re_y1 );

	p.setPen( QPen( QColor( image->pixel( re_x, re_y1) )));
      }
      p.drawPoint(x,y1);
    }

    if(!r) {
      cr2++;
      cr2 &= 7;
      if(!(cr2&3)) {
			if(cr2&4) p.setPen(black);
			else p.setPen(white);
      }
    }
    if(y1==y2) break;
    y1 += inc;
  }

}

void ImageCanvas::drawAreaBorder(QPainter *p,int r )
{
   if(selected->isNull()) return;

   cr2 = cr1;
   int xinc = 1;
   if( selected->right() < selected->left()) xinc = -1;
   int yinc = 1;
   if( selected->bottom() < selected->top()) yinc = -1;

   if( selected->width() )
      drawHAreaBorder(*p,
		      selected->left() - contentsX(),
		      selected->right()- contentsX(),
		      selected->top() - contentsY(), r );
   if( selected->height() ) {
      drawVAreaBorder(*p,
		      selected->right() - contentsX(),
		      selected->top()- contentsY()+yinc,
		      selected->bottom()- contentsY(),r);
   if(selected->width()) {
      drawHAreaBorder(*p,
	  	      selected->right()-xinc- contentsX(),
                      selected->left()- contentsX(),
                      selected->bottom()- contentsY(),r);
      drawVAreaBorder(*p,
	     	       selected->left()- contentsX(),
                       selected->bottom()-yinc- contentsY(),
		       selected->top()- contentsY()+yinc,r);
      }
   }
}

// x and y come as real pixmap-coords, not contents-coords
preview_state ImageCanvas::classifyPoint(int x,int y)
{
  if(selected->isEmpty()) return MOVE_NONE;

  QRect a = selected->normalize();

  int top = 0,left = 0,right = 0,bottom = 0;
  int lx = a.left()-x, rx = x-a.right();
  int ty = a.top()-y, by = y-a.bottom();

  if( a.width() > delta*2+2 )
    lx = abs(lx), rx = abs(rx);

  if(a.height()>delta*2+2)
    ty = abs(ty), by = abs(by);

  if( lx>=0 && lx<=delta ) left++;
  if( rx>=0 && rx<=delta ) right++;
  if( ty>=0 && ty<=delta ) top++;
  if( by>=0 && by<=delta ) bottom++;
  if( y>=a.top() &&y<=a.bottom() ) {
    if(left) {
      if(top) return MOVE_TOP_LEFT;
      if(bottom) return MOVE_BOTTOM_LEFT;
      return MOVE_LEFT;
    }
    if(right) {
      if(top) return MOVE_TOP_RIGHT;
      if(bottom) return MOVE_BOTTOM_RIGHT;
      return MOVE_RIGHT;
    }
  }
  if(x>=a.left()&&x<=a.right()) {
    if(top) return MOVE_TOP;
    if(bottom) return MOVE_BOTTOM;
    if(selected->contains(QPoint(x,y))) return MOVE_WHOLE;
  }
  return MOVE_NONE;
}





#include "img_canvas.moc"
