/* This file is part of the KDE Project
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <stdlib.h>

#include <kpopupmenu.h>
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kcmenumngr.h>
#ifdef USE_KPIXMAPIO
#include <kpixmapio.h>
#endif

#include <qscrollview.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qstyle.h>

#include "imgscaledialog.h"

#include "img_canvas.h"
#include "img_canvas.moc"


#undef HOLD_SELECTION


class ImageCanvas::ImageCanvasPrivate
{
public:
    ImageCanvasPrivate()
        : keepZoom(false),
	  readOnly(false),
          scaleKind( UNSPEC ),
          defaultScaleKind( FIT_ORIG )
        {}

    bool         keepZoom;  /* keep the zoom settings if images change */
    bool         readOnly;
    ScaleKinds   scaleKind;
    ScaleKinds   defaultScaleKind;

    QValueList<QRect> highlightRects;
};


ImageCanvas::ImageCanvas(QWidget *parent,
			 const QImage *start_image,
			 const char *name 	):
   QScrollView( parent, name ),
   m_contextMenu(0)
{
    kdDebug(29000) << k_funcinfo << endl;

    d = new ImageCanvasPrivate();

    scale_factor     = 100; // means original size
    maintain_aspect  = true;
    selected.setWidth(0);
    selected.setHeight(0);

    timer_id         = 0;
    pmScaled         = NULL;

    image            = start_image;
    moving 	   = MOVE_NONE;

    QSize img_size;

    if( image && ! image->isNull() )
    {
        img_size = image->size();
        pmScaled = new QPixmap( img_size );
	kdDebug(29000) << "    size from image is " << img_size << endl;

#ifdef USE_KPIXMAPIO
        *pmScaled = pixIO.convertToPixmap(*image);
#else
        pmScaled->convertFromImage( *image );
#endif

        acquired = true;
    } else {
        img_size = size();
	kdDebug(29000) << "    initial size is " << img_size << endl;
    }


    update_scaled_pixmap();

    viewport()->setCursor( crossCursor );
    cr1 = 0;
    cr2 = 0;
    viewport()->setMouseTracking(TRUE);
    viewport()->setBackgroundMode(PaletteBackground);
    show();

}

ImageCanvas::~ImageCanvas()
{
    kdDebug(29000) << k_funcinfo << endl;

    stopMarqueeTimer();
    if (pmScaled!=NULL) delete pmScaled;
    delete d;
}


void ImageCanvas::deleteView(const QImage *delimage)
{
    if (delimage==rootImage())
    {
        kdDebug(29000) << k_funcinfo << endl;
        newImage(NULL);
   }
}


void ImageCanvas::newImageHoldZoom(const QImage *new_image)
{
    newImage(new_image,true);
}


void ImageCanvas::newImage(const QImage *new_image,bool hold_zoom)
{
    image = new_image;					// don't free old image, not ours

    if (pmScaled!=NULL)					// delete old scaled pixmap
    {
        delete pmScaled;
        pmScaled = NULL;
    }

#ifdef HOLD_SELECTION
    QRect oldSelected = selected;
    kdDebug(29000) << k_funcinfo << "original selection " << oldSelected << " null=" << oldSelected.isEmpty() << endl;
    kdDebug(29000) << "   w=" << selected.width() << " h=" << selected.height() << endl;
#endif

    stopMarqueeTimer();					// also clears selection
    d->highlightRects.clear();				// throw away all highlights

    if (image!=NULL)					// handle the new image
    {
        kdDebug(29000) << k_funcinfo << "new image size is " << image->size() << endl;
        if (image->depth()==1) pmScaled = new QPixmap(image->size(),1);
        else pmScaled = new QPixmap(image->size(),QPixmap::defaultDepth());
#ifdef USE_KPIXMAPIO
        *pmScaled = pixIO.convertToPixmap(*image);
#else
        pmScaled->convertFromImage(*image);
#endif
        acquired = true;

        if (!d->keepZoom && !hold_zoom) setScaleKind(defaultScaleKind());
    
        update_scaled_pixmap();
        setContentsPos(0,0);
#ifdef HOLD_SELECTION
        if (!oldSelected.isNull())
        {
            selected = oldSelected;
            kdDebug(29000) << k_funcinfo << "restored selection " << selected << endl;
            startMarqueeTimer();
        }
#endif
    }
    else
    {
        kdDebug(29000) << k_funcinfo << " no new image" << endl;
        acquired = false;
        resizeContents(0,0);
    }

    repaint(true);
}


QSize ImageCanvas::sizeHint() const
{
   return( QSize( 2, 2 ));
}

void ImageCanvas::enableContextMenu( bool wantContextMenu )
{
   if( wantContextMenu )
   {
      if( ! m_contextMenu )
      {
	 m_contextMenu = new KPopupMenu(this, "IMG_CANVAS");

	 KContextMenuManager::insert( viewport(), m_contextMenu );
      }
   }
   else
   {
      /* remove all items */
      if( m_contextMenu ) m_contextMenu->clear();
      /* contextMenu is not deleted because there is no way to remove
       * it from the contextMenuManager
       */
   }

}

void ImageCanvas::handle_popup( int item )
{
   if( item < ID_POP_ZOOM || item > ID_ORIG_SIZE ) return;

   if( ! image ) return;
   ImgScaleDialog *zoomDia  = 0;

   switch( item )
   {
      case ID_POP_ZOOM:

          zoomDia = new ImgScaleDialog( this, getScaleFactor() );
          if( zoomDia->exec() )
          {
              int sf = zoomDia->getSelected();
	      setScaleKind(ZOOM);
              setScaleFactor( sf );
          }
          delete zoomDia;
	  zoomDia = 0;
	 break;
      case ID_ORIG_SIZE:
	 setScaleKind( FIT_ORIG );
         break;
      case ID_FIT_WIDTH:
          setScaleKind( FIT_WIDTH );
          break;
      case ID_FIT_HEIGHT:
          setScaleKind( FIT_HEIGHT );
	 break;
      case ID_POP_CLOSE:
	 emit( closingRequested());
	 break;

      default: break;
   }
   update_scaled_pixmap();
   repaint();
}


/**
 *   Returns the selected rect in tenth of percent, not in absolute
 *   sizes, eg. 500 means 50.0 % of original width/height.
 *   That makes it easier to work with different scales and units
 */
QRect ImageCanvas::sel( void )
{
    QRect retval;
    retval.setCoords(0, 0, 0, 0);

    if (image!=NULL && selected.width()>MIN_AREA_WIDTH
        && selected.height()>MIN_AREA_HEIGHT )
    {
	/* Get the size in real image pixels */

   	QRect mapped = inv_scale_matrix.map(selected);
   	if( mapped.x() > 0 )
	    retval.setLeft((int) (1000.0/( (double)image->width() / (double)mapped.x())));

	if( mapped.y() > 0 )
	    retval.setTop((int) (1000.0/( (double)image->height() / (double)mapped.y())));

	if( mapped.width() > 0 )
	    retval.setWidth((int) (1000.0/( (double)image->width() / (double)mapped.width())));

	if( mapped.height() > 0 )
	    retval.setHeight((int)(1000.0/( (double)image->height() / (double)mapped.height())));

     }

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
	 if (retImg!=NULL) *retImg = entireImg->copy( x, y, w, h );
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


void ImageCanvas::setSelectionRect(const QRect &rect)
{
   kdDebug(29000) << k_funcinfo << "rect=" << rect << endl;

   QRect to_map;
   QPainter p(viewport());
   drawAreaBorder(&p,true);
   stopMarqueeTimer();					// also clears selection

   if (!rect.isValid()) return;				// no (i.e. full) selection

   if (image!=NULL)					// have got an image,
   {							// so set the selection
       int rx, ry, rw, rh;
       int w = image->width();
       int h = image->height();

       kdDebug(29000) << "    Image size is " << w << "x" << h << endl;

       rw = static_cast<int>(w*rect.width()/1000.0+0.5);
       rx = static_cast<int>(w*rect.x()/1000.0+0.5);
       ry = static_cast<int>(h*rect.y()/1000.0+0.5);
       rh = static_cast<int>(h*rect.height()/1000.0+0.5);
       to_map.setRect(rx,ry,rw,rh);

       selected = scale_matrix.map(to_map);
       kdDebug(29000) << "    to_map=" << to_map << " selected=" << selected << endl;

       startMarqueeTimer();
   }
}


void ImageCanvas::startMarqueeTimer()
{
    if (timer_id==0) timer_id = startTimer(100);
}


void ImageCanvas::stopMarqueeTimer()
{
    if (timer_id!=0)
    {
        killTimer(timer_id);
        timer_id = 0;
    }

    //selected.setCoords(0,0,0,0);
    selected = QRect();					// clear the selection
}


void ImageCanvas::viewportMousePressEvent(QMouseEvent *ev)
{
   if (!acquired || image==NULL) return;
   if (ev->button()!=Qt::LeftButton) return;

   int cx = contentsX();
   int cy = contentsY();

   int x = ev->x();
   int y = ev->y();

   int ix,iy;
   scale_matrix.map(image->width(),image->height(),&ix,&iy);
   if (x>(ix-cx) || y>(iy-cy)) return;

   lx = x; ly = y;
   if (moving!=MOVE_NONE) return;

   QPainter p(viewport());
   drawAreaBorder(&p,true);
   moving = classifyPoint(x+cx,y+cy);
   if (moving==MOVE_NONE)				// Create new area
   {
       selected.setCoords(x+cx,y+cy,x+cx,y+cy);
       moving = MOVE_BOTTOM_RIGHT;
   }

   drawAreaBorder(&p);
}


void ImageCanvas::viewportMouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button()!=LeftButton || !acquired) return;
    if (moving==MOVE_NONE) return;

    QPainter p(viewport());
    drawAreaBorder(&p,true);
    moving = MOVE_NONE;
    selected = selected.normalize();

    if (selected.width()<MIN_AREA_WIDTH || selected.height()<MIN_AREA_HEIGHT)
    {
        stopMarqueeTimer();				// also clears selection
        kdDebug(29000) << k_funcinfo << "no selection" << endl;
        emit newRect(QRect());
    }
    else
    {
        drawAreaBorder(&p);

        kdDebug(29000) << k_funcinfo << "new selection " << sel() << endl;
        emit newRect(sel());
        startMarqueeTimer();
    }
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
    drawAreaBorder(&p,true);
    switch(moving) {
    case MOVE_NONE: //Just to make compiler happy
      break;
    case MOVE_TOP_LEFT:
      selected.setLeft( x + cx );
    case MOVE_TOP: // fall through
      selected.setTop( y + cy );
      break;
    case MOVE_TOP_RIGHT:
      selected.setTop( y + cy );
    case MOVE_RIGHT: // fall through
      selected.setRight( x + cx );
      break;
    case MOVE_BOTTOM_LEFT:
      selected.setBottom( y + cy );
    case MOVE_LEFT: // fall through
      selected.setLeft( x + cx );
      break;
    case MOVE_BOTTOM_RIGHT:
      selected.setRight( x + cx );
    case MOVE_BOTTOM: // fall through
      selected.setBottom( y + cy );
      break;
    case MOVE_WHOLE:
    		// lx is the Last x-Koord from the run before (global)
    		mx = x-lx; my = y-ly;
    		/* Check if rectangle would run out of the image on right and bottom */
    		if( selected.x()+ selected.width()+mx >= ix-cx )
    		{
    			mx =  ix -cx - selected.width() - selected.x();
    			//kdDebug(29000) << "runs out !" << endl;
    		}
    		if( selected.y()+ selected.height()+my >= iy-cy )
    		{
    			my =  iy -cy - selected.height() - selected.y();
    			//kdDebug(29000) << "runs out !" << endl;
    		}

    		/* Check if rectangle would run out of the image on left and top */
    		if( selected.x() +mx < 0 )
    			mx =  -selected.x();
    		if( selected.y()+ +my < 0 )
    			my =  -selected.y();

    		x = mx+lx; y = my+ly;

    		selected.moveBy( mx, my );

    }
    drawAreaBorder(&p);
    lx = x;
    ly = y;
  }
}


void ImageCanvas::setScaleFactor( int i )
{
   kdDebug(29000) <<  "Setting Scalefactor to " << i << endl;
   scale_factor = i;
   if( i == 0 ){
       kdDebug(29000) << "Setting Dynamic Scaling!" << endl;
       setScaleKind(DYNAMIC);
   }
   update_scaled_pixmap();
}


void ImageCanvas::resizeEvent( QResizeEvent * event )
{
	QScrollView::resizeEvent( event );
	update_scaled_pixmap();

}

void ImageCanvas::update_scaled_pixmap( void )
{
    resizeContents( 0,0 );
    updateScrollBars();
    if( !pmScaled || !image)
    { 	// debug( "Pixmap px is null in Update_scaled" );
	return;
    }

    QApplication::setOverrideCursor(waitCursor);

    kdDebug(29000) << "Updating scaled_pixmap" << endl;
    if( scaleKind() == DYNAMIC )
        kdDebug(29000) << "Scaling DYNAMIC" << endl;
    QSize noSBSize( visibleWidth(), visibleHeight());
    const int sbWidth = kapp->style().pixelMetric( QStyle::PM_ScrollBarExtent );

    // if( verticalScrollBar()->visible() ) noSBSize.width()+=sbWidth;
    // if( horizontalScrollBar()->visible() ) noSBSize.height()+=sbWidth;

    switch( scaleKind() )
    {
    case DYNAMIC:
        // do scaling to window-size
        used_yscaler = ((double)viewport()-> height()) / ((double)image->height());
        used_xscaler = ((double)viewport()-> width())  / ((double)image->width());
        scale_factor = 0;
        break;
    case FIT_ORIG:
        used_yscaler = used_xscaler = 1.0;
        scale_factor = 100;
        break;
    case FIT_WIDTH:
        used_xscaler = used_yscaler = double(noSBSize.width()) / double(image->width());
        if( used_xscaler * image->height() >= noSBSize.height() )
        {
            /* substract for scrollbar */
            used_xscaler = used_yscaler = double(noSBSize.width() - sbWidth) /
                           double(image->width());
            kdDebug(29000) << "FIT WIDTH scrollbar to substract: " << sbWidth << endl;
        }
        scale_factor = static_cast<int>(100*used_xscaler);
        break;
    case FIT_HEIGHT:
        used_yscaler = used_xscaler = double(noSBSize.height())/double(image->height());

        // scale = int(100.0 * noSBSize.height() / image->height());
        if( used_xscaler * image->width() >= noSBSize.width() )
        {
            /* substract for scrollbar */
            used_xscaler = used_yscaler = double(noSBSize.height() - sbWidth) /
                           double(image->height());

            kdDebug(29000) << "FIT HEIGHT scrollbar to substract: " << sbWidth << endl;
            // scale = int(100.0*(noSBSize.height() -sbWidth) / image->height());
        }
        scale_factor = static_cast<int>(100*used_xscaler);

        break;
    case ZOOM:
        used_xscaler = used_yscaler = double(getScaleFactor())/100.0;
        scale_factor = static_cast<int>(100*used_xscaler);
        break;
    default:
        break;
    }

    // reconvert the selection to orig size
        selected = inv_scale_matrix.map(selected);

    scale_matrix.reset();                         // transformation matrix
    inv_scale_matrix.reset();


    if( scaleKind() == DYNAMIC && maintain_aspect  ) {
        // printf( "Skaler: x: %f, y: %f\n", x_scaler, y_scaler );
        used_xscaler = used_yscaler <  used_xscaler ?
                       used_yscaler : used_xscaler;
        used_yscaler = used_xscaler;
    }

    scale_matrix.scale( used_xscaler, used_yscaler );  // define scale factors
    inv_scale_matrix = scale_matrix.invert();	// for redraw of selection

        selected = scale_matrix.map(selected );

#ifdef USE_KPIXMAPIO
    *pmScaled = pixIO.convertToPixmap(*image);
#else
    pmScaled->convertFromImage( *image );
#endif

    *pmScaled = pmScaled->xForm( scale_matrix );  // create scaled pixmap

    /* Resizing to 0,0 never may be dropped, otherwise there are problems
     * with redrawing of new images.
     */
    resizeContents( static_cast<int>(image->width() * used_xscaler),
                    static_cast<int>(image->height() * used_yscaler ) );

    QApplication::restoreOverrideCursor();
}


void ImageCanvas::drawHAreaBorder(QPainter &p,int x1,int x2,int y,bool remove)
{
	if( ! acquired || !image ) return;


  if(moving!=MOVE_NONE) cr2 = 0;
  int inc = 1;
  int cx = contentsX(), cy = contentsY();
  if(x2 < x1) inc = -1;

  if(!remove) {
    if(cr2 & 4) p.setPen(black);
    else p.setPen(white);
  } else if(!acquired) p.setPen(QPen(QColor(150,150,150)));

  for(;;) {
    if(rect().contains(QPoint(x1,y))) {
      if( remove && acquired ) {
	int re_x1, re_y;
	inv_scale_matrix.map( x1+cx, y+cy, &re_x1, &re_y );
	re_x1 = QMIN( image->width()-1, re_x1 );
	re_y = QMIN( image->height()-1, re_y );

	p.setPen( QPen( QColor( image->pixel(re_x1, re_y))));
      }
      p.drawPoint(x1,y);
    }
    if(!remove) {
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

void ImageCanvas::drawVAreaBorder(QPainter &p, int x, int y1, int y2, bool remove )
{
	if( ! acquired || !image ) return;
  if( moving!=MOVE_NONE ) cr2 = 0;
  int inc = 1;
  if( y2 < y1 ) inc = -1;

  int cx = contentsX(), cy = contentsY();


  if( !remove ) {
    if( cr2 & 4 ) p.setPen(black);
    else
      p.setPen(white);
  } else
    if( !acquired ) p.setPen( QPen( QColor(150,150,150) ) );

  for(;;) {
    if(rect().contains( QPoint(x,y1) )) {
      if( remove && acquired ) {
	int re_y1, re_x;
	inv_scale_matrix.map( x+cx, y1+cy, &re_x, &re_y1 );
	re_x = QMIN( image->width()-1, re_x );
	re_y1 = QMIN( image->height()-1, re_y1 );

	p.setPen( QPen( QColor( image->pixel( re_x, re_y1) )));
      }
      p.drawPoint(x,y1);
    }

    if(!remove) {
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

void ImageCanvas::drawAreaBorder(QPainter *p,bool remove )
{
   if(selected.isNull()) return;

   cr2 = cr1;
   int xinc = 1;
   if( selected.right() < selected.left()) xinc = -1;
   int yinc = 1;
   if( selected.bottom() < selected.top()) yinc = -1;

   if( selected.width() )
      drawHAreaBorder(*p,
		      selected.left() - contentsX(),
		      selected.right()- contentsX(),
		      selected.top() - contentsY(), remove );
   if( selected.height() ) {
      drawVAreaBorder(*p,
		      selected.right() - contentsX(),
		      selected.top()- contentsY()+yinc,
		      selected.bottom()- contentsY(),remove);
   if(selected.width()) {
      drawHAreaBorder(*p,
	  	      selected.right()-xinc- contentsX(),
                      selected.left()- contentsX(),
                      selected.bottom()- contentsY(),remove);
      drawVAreaBorder(*p,
	     	       selected.left()- contentsX(),
                       selected.bottom()-yinc- contentsY(),
		       selected.top()- contentsY()+yinc,remove);
      }
   }
}

// x and y come as real pixmap-coords, not contents-coords
preview_state ImageCanvas::classifyPoint(int x,int y)
{
  if(selected.isEmpty()) return MOVE_NONE;

  QRect a = selected.normalize();

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
    if(selected.contains(QPoint(x,y))) return MOVE_WHOLE;
  }
  return MOVE_NONE;
}


int ImageCanvas::getBrightness() const
{
   return brightness;
}

int ImageCanvas::getContrast() const
{
   return contrast;
}

int ImageCanvas::getGamma() const
{
   return gamma;
}

int ImageCanvas::getScaleFactor() const
{
   return( scale_factor );
}

const QImage *ImageCanvas::rootImage( )
{
   return( image );
}

void ImageCanvas::setReadOnly( bool ro )
{
    d->readOnly = ro;
    emit( imageReadOnly(ro) );
}

bool ImageCanvas::readOnly()
{
    return d->readOnly;
}

void ImageCanvas::setBrightness(int b)
{
   brightness = b;
}

void ImageCanvas::setContrast(int c)
{
   contrast = c;
}

void ImageCanvas::setGamma(int c)
{
   gamma = c;
}

void ImageCanvas::setKeepZoom( bool k )
{
    d->keepZoom = k;
}

ImageCanvas::ScaleKinds ImageCanvas::scaleKind()
{
    if( d->scaleKind == UNSPEC )
        return defaultScaleKind();
    else
        return d->scaleKind;
}

ImageCanvas::ScaleKinds ImageCanvas::defaultScaleKind()
{
    return d->defaultScaleKind;
}

void ImageCanvas::setScaleKind( ScaleKinds k )
{
    if( k == d->scaleKind ) return; // no change, return
    d->scaleKind = k;

    emit scalingChanged(scaleKindString());
}

void ImageCanvas::setDefaultScaleKind( ScaleKinds k )
{
    d->defaultScaleKind = k;
}

const QString ImageCanvas::imageInfoString( int w, int h, int d )
{
    if( w == 0 && h == 0 && d == 0 )
    {
        if( image )
        {
            w = image->width();
            h = image->height();
            d = image->depth();
        }
        else
            return QString("-");
    }
    return i18n("%1x%2 pixel, %3 bit").arg(w).arg(h).arg(d);
}


const QString ImageCanvas::scaleKindString()
{
    switch( scaleKind() )
    {
    case DYNAMIC:
        return i18n("Fit window best");
        break;
    case FIT_ORIG:
        return i18n("Original size");
        break;
    case FIT_WIDTH:
        return i18n("Fit Width");
        break;
    case FIT_HEIGHT:
        return i18n("Fit Height");
        break;
    case ZOOM:
        return i18n("Zoom to %1 %%").arg( QString::number(getScaleFactor()));
        break;
    default:
        return i18n("Unknown scaling!");
        break;
    }
}


int ImageCanvas::highlight( const QRect& rect, const QPen& pen, const QBrush&, bool ensureVis  )
{
    QRect saveRect;
    saveRect.setRect( rect.x()-2, rect.y()-2, rect.width()+4, rect.height()+4 );
    d->highlightRects.append( saveRect );

    int idx = d->highlightRects.findIndex(saveRect);

    QRect targetRect = scale_matrix.map( rect );

    QPainter p( pmScaled );

    p.setPen(pen);
    p.drawLine( targetRect.x(), targetRect.y()+targetRect.height(),
                targetRect.x()+targetRect.width(), targetRect.y()+targetRect.height() );

    p.flush();
    updateContents(targetRect.x()-1, targetRect.y()-1,
                   targetRect.width()+2, targetRect.height()+2 );

    if( ensureVis )
    {
        QPoint p = targetRect.center();
        ensureVisible( p.x(), p.y(), 10+targetRect.width()/2, 10+targetRect.height()/2 );
    }

    return idx;
}

void ImageCanvas::removeHighlight( int idx )
{
    if( (unsigned) idx >= d->highlightRects.count() )
    {
        kdDebug(29000) << "removeHighlight: Not a valid index" << endl;
        return;
    }

    /* take the rectangle from the stored highlight rects and map it to the viewer scaling */
    QRect r = d->highlightRects[idx];
    d->highlightRects.remove(r);
    QRect targetRect = scale_matrix.map( r );

    /* create a small pixmap with a copy of the region in question of the original image */
    QPixmap origPix;
    origPix.convertFromImage( image->copy(r) );
    /* and scale it */
    QPixmap scaledPix = origPix.xForm( scale_matrix );
    /* and finally draw it */
    QPainter p( pmScaled );
    p.drawPixmap( targetRect, scaledPix );
    p.flush();

    /* update the viewers contents */
    updateContents(targetRect.x()-1, targetRect.y()-1,
                   targetRect.width()+2, targetRect.height()+2 );


}
