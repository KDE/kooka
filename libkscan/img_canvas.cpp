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

#include "img_canvas.h"
#include "img_canvas.moc"

#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qstyle.h>
#include <qevent.h>

#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kmenu.h>
#ifdef USE_KPIXMAPIO
#include <kpixmapio.h>
#endif

#include "imgscaledialog.h"


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

    QList<QRect> highlightRects;
};


ImageCanvas::ImageCanvas(QWidget *parent, const QImage *start_image)
    : Q3ScrollView(parent),
      mContextMenu(NULL)
{
    setObjectName("ImageCanvas");

    kDebug();

    d = new ImageCanvasPrivate();

    scale_factor     = 100; // means original size
    maintain_aspect  = true;
    selected.setWidth(0);
    selected.setHeight(0);


    timer_id         = 0;

    image            = start_image;
    moving 	   = MOVE_NONE;

    QSize img_size;

    if (image!=NULL && !image->isNull())
    {
#ifdef USE_KPIXMAPIO
        pmScaled = pixIO.convertToPixmap(*image);
#else
        pmScaled = QPixmap::fromImage(*image);
#endif
	kDebug() << "size from image is" << pmScaled.size();
        acquired = true;
    }
    else
    {
	kDebug() << "initial size is " << size();
    }

    update_scaled_pixmap();

    viewport()->setCursor(Qt::CrossCursor);
    cr1 = 0;
    cr2 = 0;
    viewport()->setMouseTracking(true);
    //viewport()->setBackgroundMode(PaletteBackground);

    // TODO: this required to draw the marching ants rectangle on its timer
    // event, but is non-portable and not recommended.  Should do that in
    // drawContents() instead.
    viewport()->setAttribute(Qt::WA_PaintOutsidePaintEvent);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    show();
}

ImageCanvas::~ImageCanvas()
{
    kDebug();
    stopMarqueeTimer();
    delete d;
}


void ImageCanvas::deleteView(const QImage *delimage)
{
    if (delimage==rootImage())
    {
        kDebug();
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

#ifdef HOLD_SELECTION
    QRect oldSelected = selected;
    kDebug() << "original selection" << oldSelected << "null=" << oldSelected.isEmpty();
    kDebug() << "   w=" << selected.width() << "h=" << selected.height();
#endif

    stopMarqueeTimer();					// also clears selection
    d->highlightRects.clear();				// throw away all highlights

    if (image!=NULL)					// handle the new image
    {
        kDebug() << "new image size is" << image->size();

        //if (image->depth()==1) pmScaled = new QPixmap(image->size(),1);
        //else pmScaled = new QPixmap(image->size(),QPixmap::defaultDepth());
#ifdef USE_KPIXMAPIO
        *pmScaled = pixIO.convertToPixmap(*image);
#else
        pmScaled = QPixmap::fromImage(*image);
#endif
        acquired = true;

        if (!d->keepZoom && !hold_zoom) setScaleKind(defaultScaleKind());
    
        update_scaled_pixmap();
        setContentsPos(0,0);
#ifdef HOLD_SELECTION
        if (!oldSelected.isNull())
        {
            selected = oldSelected;
            kDebug() << "restored selection" << selected;
            startMarqueeTimer();
        }
#endif
    }
    else
    {
        kDebug() << "no new image";
        acquired = false;
        resizeContents(0,0);
        pmScaled = QPixmap();				// ensure old image disappears
    }

    // TODO: in '3' this was repaint(true) which erased the contents first.  Still needed?
    repaint();
}


QSize ImageCanvas::sizeHint() const
{
   return (QSize(32, 32));
}




//  Context menu
//  ------------
//
//  We don't actually populate the menu or handle its actions here.
//  The menu is populated with the parent application's actions via
//  KookaView::connectViewerAction(), and the application handles the
//  triggered actions.  Those which need some work from us are passed to
//  slotUserAction() with the appropriate UserAction key.

KMenu *ImageCanvas::contextMenu()
{
    if (mContextMenu==NULL)				// menu not created yet
    {
        mContextMenu = new KMenu(this);
        kDebug() << "context menu enabled";
    }

    return (mContextMenu);
}


void ImageCanvas::contentsContextMenuEvent(QContextMenuEvent *ev)
{
    if (mContextMenu==NULL) return;			// menu not enabled
							// but allowed if no image loaded
    mContextMenu->popup(ev->globalPos());
    ev->accept();
}


void ImageCanvas::slotUserAction(int act)
{
    if (image==NULL) return;				// no action if no image loaded

    switch (act)
    {
case ImageCanvas::UserActionZoom:
        {
            ImgScaleDialog zoomDia(this, getScaleFactor());
            if (zoomDia.exec())
            {
                int sf = zoomDia.getSelected();
                setScaleKind(ZOOM);
                setScaleFactor(sf);
            }
        }
        break;

case ImageCanvas::UserActionOrigSize:
        setScaleKind(FIT_ORIG);
        break;

case ImageCanvas::UserActionFitWidth:
        setScaleKind(FIT_WIDTH);
        break;

case ImageCanvas::UserActionFitHeight:
        setScaleKind(FIT_HEIGHT);
        break;

case ImageCanvas::UserActionClose:
        emit closingRequested();
        break;
    }

    update_scaled_pixmap();
    repaint();
}


/**
 *   Returns the selected rect in tenth of percent, not in absolute
 *   sizes, eg. 500 means 50.0 % of original width/height.
 *   That makes it easier to work with different scales and units
 */
QRect ImageCanvas::sel() const
{
    QRect retval;
    retval.setCoords(0, 0, 0, 0);

    if (image!=NULL && selected.width()>MIN_AREA_WIDTH
        && selected.height()>MIN_AREA_HEIGHT )
    {
	/* Get the size in real image pixels */

   	QRect mapped = inv_scale_matrix.mapRect(selected);
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
    if (pmScaled.isNull()) return;

    int x1 = 0;
    int y1 = 0;

    int x2 = pmScaled.width();
    int y2 = pmScaled.height();

    if (x1 < clipx) x1=clipx;
    if (y1 < clipy) y1=clipy;
    if (x2 > clipx+clipw-1) x2=clipx+clipw-1;
    if (y2 > clipy+cliph-1) y2=clipy+cliph-1;

    // Paint using the small coordinates...
    // p->scale( used_xscaler, used_yscaler );
    // p->scale( used_xscaler, used_yscaler );
    if ( x2 >= x1 && y2 >= y1 ) {
    	p->drawPixmap(x1, y1, pmScaled, x1, y1, -1, -1); //, clipw, cliph);
        // p->setBrush( red );
        // p->drawRect( x1, y1, clipw, cliph );
    }

        // p->fillRect(x1, y1, x2-x1+1, y2-y1+1, red);

}

void ImageCanvas::timerEvent(QTimerEvent *)
{
   if (!isVisible()) return;				// we're not visible
   if (!acquired) return;				// no image acquired
   if (moving!=MOVE_NONE) return;			// mouse operation in progress

   cr1++;
   QPainter p(viewport());
   // p.setWindow( contentsX(), contentsY(), contentsWidth(), contentsHeight());
   drawAreaBorder(&p);
}


void ImageCanvas::setSelectionRect(const QRect &rect)
{
   kDebug() << "rect=" << rect;

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

       kDebug() << "Image size is" << w << "x" << h;

       rw = static_cast<int>(w*rect.width()/1000.0+0.5);
       rx = static_cast<int>(w*rect.x()/1000.0+0.5);
       ry = static_cast<int>(h*rect.y()/1000.0+0.5);
       rh = static_cast<int>(h*rect.height()/1000.0+0.5);
       to_map.setRect(rx,ry,rw,rh);

       selected = scale_matrix.mapRect(to_map);
       kDebug() << "to_map=" << to_map << "selected=" << selected;

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
    if (ev->button()!=Qt::LeftButton || !acquired) return;
    if (moving==MOVE_NONE) return;

    QPainter p(viewport());
    drawAreaBorder(&p,true);
    moving = MOVE_NONE;
    selected = selected.normalized();

    if (selected.width()<MIN_AREA_WIDTH || selected.height()<MIN_AREA_HEIGHT)
    {
        stopMarqueeTimer();				// also clears selection
        kDebug() << "no selection";
        emit newRect(QRect());
    }
    else
    {
        drawAreaBorder(&p);

        kDebug() << "new selection" << sel();
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
      viewport()->setCursor(Qt::CrossCursor);
      ps = CROSS;
    }
    break;
  case MOVE_LEFT:
  case MOVE_RIGHT:
    if(ps!=HSIZE) {
      viewport()->setCursor(Qt::SizeHorCursor);
      ps = HSIZE;
    }
    break;
  case MOVE_TOP:
  case MOVE_BOTTOM:
    if(ps!=VSIZE) {
      viewport()->setCursor(Qt::SizeVerCursor);
      ps = VSIZE;
    }
    break;
  case MOVE_TOP_LEFT:
  case MOVE_BOTTOM_RIGHT:
    if(ps!=FDIAG) {
      viewport()->setCursor(Qt::SizeFDiagCursor);
      ps = FDIAG;
    }
    break;
  case MOVE_TOP_RIGHT:
  case MOVE_BOTTOM_LEFT:
    if(ps!=BDIAG) {
      viewport()->setCursor(Qt::SizeBDiagCursor);
      ps = BDIAG;
    }
    break;
  case MOVE_WHOLE:
    if(ps!=ALL) {
      viewport()->setCursor(Qt::SizeAllCursor);
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

    		selected.translate( mx, my );

    }
    drawAreaBorder(&p);
    lx = x;
    ly = y;
  }
}


void ImageCanvas::setScaleFactor( int i )
{
   kDebug() <<  "Setting Scalefactor to" << i;
   scale_factor = i;
   if( i == 0 ){
       kDebug() << "Setting Dynamic Scaling";
       setScaleKind(DYNAMIC);
   }
   update_scaled_pixmap();
}


void ImageCanvas::resizeEvent(QResizeEvent *ev)
{
    Q3ScrollView::resizeEvent(ev);
    update_scaled_pixmap();
}


void ImageCanvas::update_scaled_pixmap( void )
{
    resizeContents( 0,0 );
    updateScrollBars();
    if (pmScaled.isNull() || image==NULL) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    //kDebug() << "Updating scaled pixmap, dynamic=" << (scaleKind()==DYNAMIC);
    QSize noSBSize( visibleWidth(), visibleHeight());
    const int sbWidth = kapp->style()->pixelMetric( QStyle::PM_ScrollBarExtent );

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
            kDebug() << "FIT WIDTH scrollbar to subtract:" << sbWidth;
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

            kDebug() << "FIT HEIGHT scrollbar to subtract:" << sbWidth;
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
        selected = inv_scale_matrix.mapRect(selected);

    scale_matrix.reset();                         // transformation matrix
    inv_scale_matrix.reset();


    if( scaleKind() == DYNAMIC && maintain_aspect  ) {
        // printf( "Skaler: x: %f, y: %f\n", x_scaler, y_scaler );
        used_xscaler = used_yscaler <  used_xscaler ?
                       used_yscaler : used_xscaler;
        used_yscaler = used_xscaler;
    }

    scale_matrix.scale( used_xscaler, used_yscaler );  // define scale factors
    inv_scale_matrix = scale_matrix.inverted();	// for redraw of selection

    selected = scale_matrix.mapRect(selected );

#ifdef USE_KPIXMAPIO
    *pmScaled = pixIO.convertToPixmap(*image);
    *pmScaled = pmScaled->xForm( scale_matrix );  // create scaled pixmap
#else
    pmScaled = QPixmap::fromImage(image->transformed(scale_matrix));
#endif

    /* Resizing to 0,0 never may be dropped, otherwise there are problems
     * with redrawing of new images.
     */
    resizeContents( static_cast<int>(image->width() * used_xscaler),
                    static_cast<int>(image->height() * used_yscaler ) );

    QApplication::restoreOverrideCursor();
}


void ImageCanvas::drawHAreaBorder(QPainter &p,int x1,int x2,int y,bool remove)
{
    if (!acquired || image==NULL) return;


  if(moving!=MOVE_NONE) cr2 = 0;
  int inc = 1;
  int cx = contentsX(), cy = contentsY();
  if(x2 < x1) inc = -1;

  if(!remove) {
    if(cr2 & 4) p.setPen(Qt::black);
    else p.setPen(Qt::white);
  } else if(!acquired) p.setPen(QPen(QColor(150,150,150)));

  for(;;) {
    if(rect().contains(QPoint(x1,y))) {
      if( remove && acquired ) {
	int re_x1, re_y;
	inv_scale_matrix.map( x1+cx, y+cy, &re_x1, &re_y );
	re_x1 = qMin( image->width()-1, re_x1 );
	re_y = qMin( image->height()-1, re_y );

	p.setPen( QPen( QColor( image->pixel(re_x1, re_y))));
      }
      p.drawPoint(x1,y);
    }
    if(!remove) {
      cr2++;
      cr2 &= 7;
      if(!(cr2&3)) {
			if(cr2&4) p.setPen(Qt::black);
			else p.setPen(Qt::white);
      }
    }
    if(x1==x2) break;
    x1 += inc;
  }

}

void ImageCanvas::drawVAreaBorder(QPainter &p, int x, int y1, int y2, bool remove )
{
    if (!acquired || image==NULL) return;

  if( moving!=MOVE_NONE ) cr2 = 0;
  int inc = 1;
  if( y2 < y1 ) inc = -1;

  int cx = contentsX(), cy = contentsY();


  if( !remove ) {
    if( cr2 & 4 ) p.setPen(Qt::black);
    else
      p.setPen(Qt::white);
  } else
    if( !acquired ) p.setPen( QPen( QColor(150,150,150) ) );

  for(;;) {
    if(rect().contains( QPoint(x,y1) )) {
      if( remove && acquired ) {
	int re_y1, re_x;
	inv_scale_matrix.map( x+cx, y1+cy, &re_x, &re_y1 );
	re_x = qMin( image->width()-1, re_x );
	re_y1 = qMin( image->height()-1, re_y1 );

	p.setPen( QPen( QColor( image->pixel( re_x, re_y1) )));
      }
      p.drawPoint(x,y1);
    }

    if(!remove) {
      cr2++;
      cr2 &= 7;
      if(!(cr2&3)) {
			if(cr2&4) p.setPen(Qt::black);
			else p.setPen(Qt::white);
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

  QRect a = selected.normalized();

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


void ImageCanvas::setReadOnly( bool ro )
{
    d->readOnly = ro;
    emit( imageReadOnly(ro) );
}

bool ImageCanvas::readOnly()
{
    return d->readOnly;
}

// TODO: inline these 3
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
    if (w==0 && h==0 && d==0)
    {
        if (image!=NULL)
        {
            w = image->width();
            h = image->height();
            d = image->depth();
        }
        else return ("-");
    }
    return (i18n("%1x%2 pixel, %3 bit", w, h, d));
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
        return i18n("Zoom to %1 %%", getScaleFactor());
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

    int idx = d->highlightRects.indexOf(saveRect);

    QRect targetRect = scale_matrix.mapRect(rect);

    QPainter p(&pmScaled);

    p.setPen(pen);
    p.drawLine( targetRect.x(), targetRect.y()+targetRect.height(),
                targetRect.x()+targetRect.width(), targetRect.y()+targetRect.height() );

    updateContents(targetRect.x()-1, targetRect.y()-1,
                   targetRect.width()+2, targetRect.height()+2 );

    if( ensureVis )
    {
        QPoint p = targetRect.center();
        ensureVisible( p.x(), p.y(), 10+targetRect.width()/2, 10+targetRect.height()/2 );
    }

    return idx;
}

void ImageCanvas::removeHighlight(int idx)
{
    if (idx>=d->highlightRects.count())
    {
        kDebug() << "Not a valid index";
        return;
    }

    /* take the rectangle from the stored highlight rects and map it to the viewer scaling */
    QRect sourceRect = d->highlightRects.takeAt(idx);
    QRect targetRect = scale_matrix.mapRect(sourceRect);

    // Create a small pixmap with a copy of the region in question of the original
    // image.  More efficient to do the transformation on a QImage, and convert
    // to QPixmap afterwards.
    QPixmap scaledPix = QPixmap::fromImage(image->copy(sourceRect).transformed(scale_matrix));
    /* and finally draw it */
    QPainter p(&pmScaled);
    p.drawPixmap( targetRect, scaledPix );

    /* update the viewers contents */
    updateContents(targetRect.x()-1, targetRect.y()-1,
                   targetRect.width()+2, targetRect.height()+2 );
}


void ImageCanvas::toggleAspect(int aspect_in_mind)
{
    maintain_aspect = aspect_in_mind;
    repaint();
}
