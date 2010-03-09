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

#include "imagecanvas.h"
#include "imagecanvas.moc"

#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qstyle.h>
#include <qevent.h>
#include <qlabel.h>
#include <qscrollbar.h>

#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kmenu.h>

#include "imgscaledialog.h"


#undef HOLD_SELECTION


const int MIN_AREA_WIDTH = 3;
const int MIN_AREA_HEIGHT = 3;
const int DELTA = 3;
const int TIMER_INTERVAL = 100;



//  ImageCanvasWidget -- Displays a scaled pixmap and any number of
//  highlight rectangles over the top of it.

// TODO: this needs to handle the selection rectangle too, as otherwise it
// erases the highlights (see ImageCanvas::draw*AreaBorder() for why).  Also
// needs to be done in a paint event, so that the Qt::WA_PaintOutsidePaintEvent
// can be eliminated.

class ImageCanvasWidget : public QWidget
{
public:
    ImageCanvasWidget(QWidget *parent);

    void setScaleMatrix(const QMatrix &m)	{ mScaleMatrix = m; };
    void setPixmap(const QPixmap &p)		{ mPixmap = p; };

    int addHighlightRect(const QRect &r);
    void removeHighlightRect(int idx);
    void removeHighlightRect(const QRect &r);
    void removeAllHighlights();
    void setHighlightStyle(ImageCanvas::HighlightStyle style, const QPen &pen, const QBrush &brush);

    QSize sizeHint() const			{ return (mPixmap.size()); };

protected:
    void paintEvent(QPaintEvent *ev);

private:
    QList<QRect> mHighlightRects;
    QMatrix mScaleMatrix;
    QPixmap mPixmap;
    QPen mPen;
    QBrush mBrush;
    ImageCanvas::HighlightStyle mStyle;
};


ImageCanvasWidget::ImageCanvasWidget(QWidget *parent)
    : QWidget(parent)
{
}


int ImageCanvasWidget::addHighlightRect(const QRect &r)
{
    int idx = mHighlightRects.indexOf(r);
    if (idx>-1) return (idx);				// already present

    mHighlightRects.append(r);
    update();

    return (mHighlightRects.count()-1);			// index of just added
}


void ImageCanvasWidget::removeHighlightRect(int idx)
{
    if (idx>=mHighlightRects.count())
    {
        kDebug() << "Invalid index" << idx;
        return;
    }

    mHighlightRects.removeAt(idx);
    update();
}


void ImageCanvasWidget::removeHighlightRect(const QRect &r)
{
    mHighlightRects.removeOne(r);
    update();
}


void ImageCanvasWidget::removeAllHighlights()
{
    mHighlightRects.clear();
    update();
}


void ImageCanvasWidget::setHighlightStyle(ImageCanvas::HighlightStyle style,
                                          const QPen &pen, const QBrush &brush)
{
    mStyle = style;
    mPen = pen;
    mBrush = brush;
}


void ImageCanvasWidget::paintEvent(QPaintEvent *ev)
{
    QPainter p(this);

    style()->drawItemPixmap(&p, contentsRect(), Qt::AlignLeft|Qt::AlignTop, mPixmap);

    //kDebug() << "painting" << mHighlightRects.count() << "highlights";
    if (!mHighlightRects.isEmpty())
    {
        for (QList<QRect>::const_iterator it = mHighlightRects.constBegin();
             it!=mHighlightRects.constEnd(); ++it)
        {
            QRect r = (*it);
            r.adjust(-4, -4, 4, 4);			// expand a bit before scaling
            QRect targetRect = mScaleMatrix.mapRect(r);

            p.setPen(mPen);
            p.setBrush(mBrush);
            switch (mStyle)
            {
case ImageCanvas::Box:
                p.drawRect(targetRect);
                break;

case ImageCanvas::Underline:
                p.drawLine(targetRect.left(), targetRect.bottom(), targetRect.right(), targetRect.bottom());
                break;
            }
        }
    }
}



//  ImageCanvas -- Scrolling area containing the pixmap/highlight widget.
//  Selected areas are drawn over the top of that.

ImageCanvas::ImageCanvas(QWidget *parent, const QImage *start_image)
    : QScrollArea(parent)
{
    setObjectName("ImageCanvas");

    kDebug();

    mContextMenu = NULL;
    mTimerId = 0;

    mKeepZoom = false;
    mReadOnly = false;
    mScaleType = ImageCanvas::ScaleUnspecified;
    mDefaultScaleType = ImageCanvas::ScaleOriginal;
    mAcquired = false;					// no image yet
    mScaleFactor = 100;					// means original size
    mMaintainAspect = true;

    mSelected.setWidth(0);
    mSelected.setHeight(0);

    mImage = start_image;
    mMoving = ImageCanvas::MoveNone;
    mCurrentCursor = Qt::ArrowCursor;

    if (mImage!=NULL && !mImage->isNull())
    {
	kDebug() << "size of image is" << mImage->size();
        mAcquired = true;
    }

    mPixmapLabel = new ImageCanvasWidget(this);
    setWidget(mPixmapLabel);
    updateScaledPixmap();

    setCursorShape(Qt::CrossCursor);
    mAreaState1 = mAreaState2 = 0;
    widget()->setMouseTracking(true);			// both of these are needed
    setMouseTracking(true);

    // TODO: this is required to draw the marching ants rectangle on its timer
    // event, but is non-portable and not recommended.  Should do that in
    // a paint event instead.
    widget()->setAttribute(Qt::WA_PaintOutsidePaintEvent);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    show();
}


ImageCanvas::~ImageCanvas()
{
    kDebug();
    stopMarqueeTimer();
}


//  Setting the image
//  -----------------

void ImageCanvas::newImageHoldZoom(const QImage *new_image)
{
    newImage(new_image, true);
}


void ImageCanvas::newImage(const QImage *new_image, bool hold_zoom)
{
    mImage = new_image;					// don't free old image, not ours

#ifdef HOLD_SELECTION
    QRect oldSelected = mSelected;
    kDebug() << "original selection" << oldSelected << "null=" << oldSelected.isEmpty();
    kDebug() << "   w=" << mSelected.width() << "h=" << mSelected.height();
#endif

    stopMarqueeTimer();					// also clears selection
    mPixmapLabel->removeAllHighlights();		// throw away all highlights

    if (mImage!=NULL)					// handle the new image
    {
        kDebug() << "new image size is" << mImage->size();
        mAcquired = true;

        if (!mKeepZoom && !hold_zoom) setScaleType(defaultScaleType());
    
        updateScaledPixmap();
#ifdef HOLD_SELECTION
        if (!oldSelected.isNull())
        {
            mSelected = oldSelected;
            kDebug() << "restored selection" << mSelected;
            startMarqueeTimer();
        }
#endif
    }
    else
    {
        kDebug() << "no new image";
        mAcquired = false;
        mScaledPixmap = QPixmap();			// ensure old image disappears
        mPixmapLabel->setPixmap(mScaledPixmap);
        mPixmapLabel->adjustSize();
    }

    repaint();
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


void ImageCanvas::contextMenuEvent(QContextMenuEvent *ev)
{
    if (mContextMenu==NULL) return;			// menu not enabled
							// but allowed if no image loaded
    mContextMenu->popup(ev->globalPos());
    ev->accept();
}


void ImageCanvas::slotUserAction(int act)
{
    if (mImage==NULL) return;				// no action if no image loaded

    switch (act)
    {
case ImageCanvas::UserActionZoom:
        {
            ImgScaleDialog zoomDia(this, mScaleFactor);
            if (zoomDia.exec())
            {
                int sf = zoomDia.getSelected();
                setScaleType(ImageCanvas::ScaleZoom);
                setScaleFactor(sf);
            }
        }
        break;

case ImageCanvas::UserActionOrigSize:
        setScaleType(ImageCanvas::ScaleOriginal);
        break;

case ImageCanvas::UserActionFitWidth:
        setScaleType(ImageCanvas::ScaleFitWidth);
        break;

case ImageCanvas::UserActionFitHeight:
        setScaleType(ImageCanvas::ScaleFitHeight);
        break;

case ImageCanvas::UserActionClose:
        emit closingRequested();
        break;
    }

    updateScaledPixmap();
    repaint();
}


//  Selected rectangle
//  ------------------
//
//  The rectangle is returned and set in units of 1/10 of a percent,
//  not in absolute sizes - eg. 500 means 50.0% of original width/height.
//  That makes it easier to work with different scales and units

//  TODO: these conversions, though, are introducing some inaccuracy somewhere.
//  May need to work in pixels instead.

QRect ImageCanvas::selectedRect() const
{
    QRect retval;
    retval.setCoords(0, 0, 0, 0);

    if (mImage!=NULL && mSelected.width()>MIN_AREA_WIDTH
        && mSelected.height()>MIN_AREA_HEIGHT )
    {
	/* Get the size in real image pixels */

   	QRect mapped = mInvScaleMatrix.mapRect(mSelected);
   	if( mapped.x() > 0 )
	    retval.setLeft((int) (1000.0/( (double)mImage->width() / (double)mapped.x())));

	if( mapped.y() > 0 )
	    retval.setTop((int) (1000.0/( (double)mImage->height() / (double)mapped.y())));

	if( mapped.width() > 0 )
	    retval.setWidth((int) (1000.0/( (double)mImage->width() / (double)mapped.width())));

	if( mapped.height() > 0 )
	    retval.setHeight((int)(1000.0/( (double)mImage->height() / (double)mapped.height())));
     }

     return (retval);
}


void ImageCanvas::setSelectionRect(const QRect &rect)
{
   kDebug() << "rect=" << rect;

   QRect to_map;
   QPainter p(widget());
   drawAreaBorder(&p,true);
   stopMarqueeTimer();					// also clears selection

   if (!rect.isValid()) return;				// no (i.e. full) selection

   if (mImage!=NULL)					// have got an image,
   {							// so set the selection
       int rx, ry, rw, rh;
       int w = mImage->width();
       int h = mImage->height();

       kDebug() << "Image size is" << w << "x" << h;

       rw = static_cast<int>(w*rect.width()/1000.0+0.5);
       rx = static_cast<int>(w*rect.x()/1000.0+0.5);
       ry = static_cast<int>(h*rect.y()/1000.0+0.5);
       rh = static_cast<int>(h*rect.height()/1000.0+0.5);
       to_map.setRect(rx,ry,rw,rh);

       mSelected = mScaleMatrix.mapRect(to_map);
       kDebug() << "to_map=" << to_map << "selected=" << mSelected;

       startMarqueeTimer();
   }
}


//  Selected image
//  --------------

QImage ImageCanvas::selectedImage() const
{
    if (!mAcquired || mImage==NULL || mImage->isNull()) return (QImage());
							// no image available
    const QRect r = selectedRect();
    const QSize s = mImage->size();

    int w = (s.width()*r.width())/1000;
    int h = (s.height()*r.height())/1000;
    if (w<=0 || h<=0) return (QImage());		// null selection

    int x = (s.width()*r.x())/1000;
    int y = (s.height()*r.y())/1000;
    return (mImage->copy(x, y, w, h));			// extract and return selection
}


//  Animation timer
//  ---------------

void ImageCanvas::timerEvent(QTimerEvent *)
{
   if (!isVisible()) return;				// we're not visible
   if (!mAcquired) return;				// no image acquired
   if (mMoving!=ImageCanvas::MoveNone) return;		// mouse operation in progress

   mAreaState1++;
   QPainter p(widget());
   drawAreaBorder(&p);
}


void ImageCanvas::startMarqueeTimer()
{
    if (mTimerId==0) mTimerId = startTimer(TIMER_INTERVAL);
}


void ImageCanvas::stopMarqueeTimer()
{
    if (mTimerId!=0)
    {
        killTimer(mTimerId);
        mTimerId = 0;
    }

    mSelected = QRect();				// clear the selection
}


//  Mouse events
//  ------------

inline int ImageCanvas::contentsX() const { return (horizontalScrollBar()->value()); }
inline int ImageCanvas::contentsY() const { return (verticalScrollBar()->value());   }


void ImageCanvas::mousePressEvent(QMouseEvent *ev)
{
    if (!mAcquired || mImage==NULL) return;
    if (ev->button()!=Qt::LeftButton) return;

    int x = ev->x()+contentsX();			// pixmap coordinates
    int y = ev->y()+contentsY();
							// ignore unless within pixmap
    if (x<0 || x>mScaledPixmap.width() || y<0 || y>mScaledPixmap.height()) return;

    mLastX = x;
    mLastY = y;
    if (mMoving!=ImageCanvas::MoveNone) return;

    QPainter p(widget());
    drawAreaBorder(&p, true);

    mMoving = classifyPoint(x, y);
    if (mMoving==ImageCanvas::MoveNone)			// starting new area
    {
        mSelected.setCoords(x, y, x, y);
        mMoving = ImageCanvas::MoveNew;
    }

    drawAreaBorder(&p);
}


void ImageCanvas::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button()!=Qt::LeftButton || !mAcquired) return;
    if (mMoving==ImageCanvas::MoveNone) return;

    QPainter p(widget());
    drawAreaBorder(&p, true);
    mMoving = ImageCanvas::MoveNone;

    mSelected = mSelected.normalized();
    if (mSelected.width()<MIN_AREA_WIDTH || mSelected.height()<MIN_AREA_HEIGHT)
    {
        stopMarqueeTimer();				// also clears selection
        kDebug() << "no selection";
        emit newRect(QRect());
    }
    else
    {
        drawAreaBorder(&p);

        kDebug() << "new selection" << selectedRect();
        emit newRect(selectedRect());
        startMarqueeTimer();
    }

    mouseMoveEvent(ev);					// update cursor shape
}


void ImageCanvas::mouseMoveEvent(QMouseEvent *ev)
{
    if (!mAcquired || mImage==NULL) return;

    int x = ev->x()+contentsX();			// pixmap coordinates
    int y = ev->y()+contentsY();

    int ix = mScaledPixmap.width();			// pixmap overall size
    int iy = mScaledPixmap.height();

    // Limit drag rectangle to the scaled pixmap
    if (x<0) x = 0;
    if (x>=ix) x = ix-1;
    if (y<0) y = 0;
    if (y>=iy) y = iy-1;

    switch (mMoving!=ImageCanvas::MoveNone ? mMoving : classifyPoint(x, y))
    {
case ImageCanvas::MoveNone:
        setCursorShape(Qt::CrossCursor);
        break;

case ImageCanvas::MoveLeft:
case ImageCanvas::MoveRight:
        setCursorShape(Qt::SizeHorCursor);
        break;

case ImageCanvas::MoveTop:
case ImageCanvas::MoveBottom:
        setCursorShape(Qt::SizeVerCursor);
        break;

case ImageCanvas::MoveTopLeft:
case ImageCanvas::MoveBottomRight:
        setCursorShape(Qt::SizeFDiagCursor);
        break;

case ImageCanvas::MoveTopRight:
case ImageCanvas::MoveBottomLeft:
        setCursorShape(Qt::SizeBDiagCursor);
        break;

case ImageCanvas::MoveWhole:
        setCursorShape(Qt::SizeAllCursor);
        break;

case ImageCanvas::MoveNew:
        setCursorShape(Qt::ArrowCursor);
        break;
    }

    // Update the selection rectangle
    if (mMoving!=ImageCanvas::MoveNone)
    {
         QPainter p(widget());
         drawAreaBorder(&p, true);

         switch (mMoving)
         {
case ImageCanvas::MoveNone:
             break;

case ImageCanvas::MoveTopLeft:
             mSelected.setLeft(x);
							// fall through
case ImageCanvas::MoveTop:
             mSelected.setTop(y);
             break;

case ImageCanvas::MoveTopRight:
             mSelected.setTop(y);
							// fall through
case ImageCanvas::MoveRight:
             mSelected.setRight(x);
             break;

case ImageCanvas::MoveBottomLeft:
             mSelected.setBottom(y);
							// fall through
case ImageCanvas::MoveLeft:
             mSelected.setLeft(x);
             break;

case ImageCanvas::MoveNew:
case ImageCanvas::MoveBottomRight:
             mSelected.setRight(x);
							// fall through
case ImageCanvas::MoveBottom:
             mSelected.setBottom(y);
             break;

case ImageCanvas::MoveWhole:
             // mLastX/Y are the last coordinates from the event before
             int mx = x-mLastX;
             int my = y-mLastY;

             // Check if rectangle would be outside the image on right/bottom
             if ((mSelected.x()+mSelected.width()+mx)>=ix)
             {
                 mx = ix-mSelected.width()-mSelected.x();
             }
             if ((mSelected.y()+mSelected.height()+my)>=iy)
             {
                 my = iy-mSelected.height()-mSelected.y();
             }

             // Check if rectangle would be outside the image on left/top
             if ((mSelected.x()+mx)<0) mx = -mSelected.x();
             if ((mSelected.y()+my)<0) my = -mSelected.y();

             x = mx+mLastX;
             y = my+mLastY;
             mSelected.translate(mx, my);
         }

         drawAreaBorder(&p);
         mLastX = x;
         mLastY = y;
    }
}


void ImageCanvas::resizeEvent(QResizeEvent *ev)
{
    QScrollArea::resizeEvent(ev);
    updateScaledPixmap();
}


//  Scaling settings
//  ----------------

void ImageCanvas::setScaleFactor(int i)
{
   kDebug() << "to" << i;
   mScaleFactor = i;
   if (i==0) setScaleType(ImageCanvas::ScaleDynamic);

   updateScaledPixmap();
}


ImageCanvas::ScaleType ImageCanvas::scaleType() const
{
    if (mScaleType!=ImageCanvas::ScaleUnspecified) return (mScaleType);
    else return (defaultScaleType());
}


void ImageCanvas::setScaleType(ScaleType type)
{
    if (type==mScaleType ) return;			// no change

    kDebug() << "to" << type;
    mScaleType = type;
    emit scalingChanged(scaleTypeString());
}


const QString ImageCanvas::scaleTypeString() const
{
    switch (scaleType())
    {
case ImageCanvas::ScaleDynamic:		return (i18n("Fit Best"));
case ImageCanvas::ScaleOriginal:	return (i18n("Original size"));
case ImageCanvas::ScaleFitWidth:	return (i18n("Fit Width"));
case ImageCanvas::ScaleFitHeight:	return (i18n("Fit Height"));
case ImageCanvas::ScaleZoom:	        return (i18n("Zoom to %1%%", mScaleFactor));
default:				return (i18n("Unknown"));
    }
}


//  Scale the image to a pixmap for display
//  ---------------------------------------

void ImageCanvas::updateScaledPixmap()
{
    if (mImage==NULL || mImage->isNull()) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);	// scaling can be slow

    const int iw = mImage->width();			// original image size
    const int ih = mImage->height();
    const int aw = width()-2*frameWidth();		// available display size
    const int ah = height()-2*frameWidth();
							// width/height of scroll bar
    const int sbSize = kapp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

    double xscale, yscale;				// calculated scale factors

    switch (scaleType())
    {
case ImageCanvas::ScaleDynamic:
        xscale = double(aw)/double(iw);
        yscale = double(ah)/double(ih);
        mScaleFactor = 0;

        if (mMaintainAspect)
        {
            xscale = yscale<xscale ? yscale : xscale;
            yscale = xscale;
        }
        break;

default:
        kDebug() << "Unknown scale type" << scaleType();
							// fall through
case ImageCanvas::ScaleOriginal:
        yscale = xscale = 1.0;
        mScaleFactor = 100;
        break;

case ImageCanvas::ScaleFitWidth:
        xscale = yscale = double(aw)/double(iw);
        if ((yscale*ih)>=ah)				// will there be a scroll bar?
        {
            // account for vertical scroll bar
            xscale = yscale = double(aw-sbSize)/double(iw);
            kDebug() << "FIT WIDTH scrollbar to subtract:" << sbSize;
        }
        mScaleFactor = static_cast<int>(100*xscale);
        break;

case ImageCanvas::ScaleFitHeight:
        yscale = xscale = double(ah)/double(ih);
        if ((xscale*iw)>=aw)				// will there be a scroll bar?
        {
            // account for horizontal scroll bar
            yscale = xscale = double(ah-sbSize)/double(ih);
            kDebug() << "FIT HEIGHT scrollbar to subtract:" << sbSize;
        }
        mScaleFactor = static_cast<int>(100*yscale);
        break;

case ImageCanvas::ScaleZoom:
        xscale = yscale = double(mScaleFactor)/100.0;
        mScaleFactor = static_cast<int>(100*xscale);
        break;
    }

    mSelected = mInvScaleMatrix.mapRect(mSelected);	// save selection in image pixels

    mScaleMatrix.reset();
    mScaleMatrix.scale(xscale, yscale);			// to map image -> display
    mInvScaleMatrix.reset();
    mInvScaleMatrix = mScaleMatrix.inverted();		// to map display -> image

    mSelected = mScaleMatrix.mapRect(mSelected);	// restore selection at new scale

    mScaledPixmap = QPixmap::fromImage(mImage->transformed(mScaleMatrix));
    mPixmapLabel->setPixmap(mScaledPixmap);		// set new pixmap to display
    mPixmapLabel->setScaleMatrix(mScaleMatrix);		// set scale for highlights
    mPixmapLabel->adjustSize();

    QApplication::restoreOverrideCursor();
}


//  Drawing the selection area
//  --------------------------

void ImageCanvas::drawHAreaBorder(QPainter *p,int x1,int x2,int y,bool remove)
{
    if (!mAcquired || mImage==NULL) return;

  if(mMoving!=ImageCanvas::MoveNone) mAreaState2 = 0;
  int inc = 1;
  if(x2 < x1) inc = -1;

  if(!remove) {
    if(mAreaState2 & 4) p->setPen(Qt::black);
    else p->setPen(Qt::white);
  } else if(!mAcquired) p->setPen(QPen(QColor(150,150,150)));

  for(;;) {
      if( remove && mAcquired ) {
	int re_x1, re_y;
	mInvScaleMatrix.map( x1, y, &re_x1, &re_y );
	re_x1 = qMin( mImage->width()-1, re_x1 );
	re_y = qMin( mImage->height()-1, re_y );

	p->setPen( QPen( QColor( mImage->pixel(re_x1, re_y))));
      }
      p->drawPoint(x1,y);
    if(!remove) {
      mAreaState2++;
      mAreaState2 &= 7;
      if(!(mAreaState2&3)) {
			if(mAreaState2&4) p->setPen(Qt::black);
			else p->setPen(Qt::white);
      }
    }
    if(x1==x2) break;
    x1 += inc;
  }

}


void ImageCanvas::drawVAreaBorder(QPainter *p, int x, int y1, int y2, bool remove )
{
    if (!mAcquired || mImage==NULL) return;

  if( mMoving!=ImageCanvas::MoveNone ) mAreaState2 = 0;
  int inc = 1;
  if( y2 < y1 ) inc = -1;

  if( !remove ) {
    if( mAreaState2 & 4 ) p->setPen(Qt::black);
    else
      p->setPen(Qt::white);
  } else
    if( !mAcquired ) p->setPen( QPen( QColor(150,150,150) ) );

  for(;;) {
      if( remove && mAcquired ) {
	int re_y1, re_x;
	mInvScaleMatrix.map( x, y1, &re_x, &re_y1 );
	re_x = qMin( mImage->width()-1, re_x );
	re_y1 = qMin( mImage->height()-1, re_y1 );

	p->setPen( QPen( QColor( mImage->pixel( re_x, re_y1) )));
      }
      p->drawPoint(x,y1);

    if(!remove) {
      mAreaState2++;
      mAreaState2 &= 7;
      if(!(mAreaState2&3)) {
			if(mAreaState2&4) p->setPen(Qt::black);
			else p->setPen(Qt::white);
      }
    }
    if(y1==y2) break;
    y1 += inc;
  }
}


void ImageCanvas::drawAreaBorder(QPainter *p, bool remove)
{
   if(mSelected.isNull()) return;

   mAreaState2 = mAreaState1;
   QRect r = mSelected.normalized();

   if (r.width()>0)
   {							// top line
       drawHAreaBorder(p, r.left(), r.right(), r.top(), remove);
   }

   if (r.height()>0)
   {							// right line
       drawVAreaBorder(p, r.right(), r.top()+1, r.bottom(), remove);
       if (r.width()>0)
       {						// bottom line
           drawHAreaBorder(p, r.right()-1, r.left(), r.bottom(), remove);
							// left line
           drawVAreaBorder(p, r.left(), r.bottom()-1, r.top()+1, remove);
      }
   }
}


//  Mouse position on the selection rectangle
//  -----------------------------------------
//
//  x and y here are coordinates on the scaled pixmap.

ImageCanvas::MoveState ImageCanvas::classifyPoint(int x, int y) const
{
    if (mSelected.isEmpty()) return (ImageCanvas::MoveNone);

  QRect a = mSelected.normalized();

  int top = 0,left = 0,right = 0,bottom = 0;
  int lx = a.left()-x, rx = x-a.right();
  int ty = a.top()-y, by = y-a.bottom();

  if( a.width() > DELTA*2+2 )
    lx = abs(lx), rx = abs(rx);

  if(a.height()>DELTA*2+2)
    ty = abs(ty), by = abs(by);

  if( lx>=0 && lx<=DELTA ) left++;
  if( rx>=0 && rx<=DELTA ) right++;
  if( ty>=0 && ty<=DELTA ) top++;
  if( by>=0 && by<=DELTA ) bottom++;
  if( y>=a.top() &&y<=a.bottom() ) {
    if(left) {
      if(top) return ImageCanvas::MoveTopLeft;
      if(bottom) return ImageCanvas::MoveBottomLeft;
      return ImageCanvas::MoveLeft;
    }
    if(right) {
      if(top) return ImageCanvas::MoveTopRight;
      if(bottom) return ImageCanvas::MoveBottomRight;
      return ImageCanvas::MoveRight;
    }
  }
  if(x>=a.left()&&x<=a.right()) {
    if(top) return ImageCanvas::MoveTop;
    if(bottom) return ImageCanvas::MoveBottom;
    if(mSelected.contains(QPoint(x,y))) return ImageCanvas::MoveWhole;
  }
  return ImageCanvas::MoveNone;
}


void ImageCanvas::setCursorShape(Qt::CursorShape cs)
{
    if (mCurrentCursor!=cs)				// optimise no-change
    {
        widget()->setCursor(cs);
        mCurrentCursor = cs;
    }
}


//  Miscellaneous settings and information
//  --------------------------------------

void ImageCanvas::setReadOnly(bool ro)
{
    mReadOnly = ro;
    emit imageReadOnly(ro);
}


void ImageCanvas::setMaintainAspect(bool ma)
{
    mMaintainAspect = ma;
    repaint();
}


const QString ImageCanvas::imageInfoString(int w, int h, int d) const
{
    if (w==0 && h==0 && d==0)
    {
        if (mImage==NULL) return ("-");
        w = mImage->width();
        h = mImage->height();
        d = mImage->depth();
    }

    return (i18n("%1x%2 pixel, %3 bit", w, h, d));
}


//  Multiple views
//  --------------
//
//  Not used by Kooka.

void ImageCanvas::deleteView(const QImage *delimage)
{
    if (delimage==rootImage())
    {
        kDebug();
        newImage(NULL);
   }
}


//  Highlight areas
//  ---------------

int ImageCanvas::highlight(const QRect &rect, bool ensureVis)
{
    int idx = mPixmapLabel->addHighlightRect(rect);
    if (ensureVis) scrollTo(rect);
    return (idx);
}


void ImageCanvas::removeHighlight(int idx)
{
    mPixmapLabel->removeHighlightRect(idx);
}


void ImageCanvas::scrollTo(const QRect &rect)
{
    if (!rect.isValid()) return;
    QRect targetRect = mScaleMatrix.mapRect(rect);
    QPoint p = targetRect.center();
    ensureVisible(p.x(), p.y(), 10+targetRect.width()/2, 10+targetRect.height()/2);
}


void ImageCanvas::setHighlightStyle(ImageCanvas::HighlightStyle style,
                                    const QPen &pen, const QBrush &brush)
{
    mPixmapLabel->setHighlightStyle(style, pen, brush);
}
