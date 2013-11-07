/************************************************************************
 *									*
 *  This file is part of libkscan, a KDE scanning library.		*
 *									*
 *  Copyright (C) 2013 Jonathan Marten <jjm@keelhaul.me.uk>		*
 *  Copyright (C) 1999 Klaas Freitag <freitag@suse.de>			*
 *									*
 *  This library is free software; you can redistribute it and/or	*
 *  modify it under the terms of the GNU Library General Public		*
 *  License as published by the Free Software Foundation and appearing	*
 *  in the file COPYING included in the packaging of this file;		*
 *  either version 2 of the License, or (at your option) any later	*
 *  version.								*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public License	*
 *  along with this program;  see the file COPYING.  If not, write to	*
 *  the Free Software Foundation, Inc., 51 Franklin Street,		*
 *  Fifth Floor, Boston, MA 02110-1301, USA.				*
 *									*
 ************************************************************************/

#include "imagecanvas.h"
#include "imagecanvas.moc"

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QTransform>

#include <qimage.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qevent.h>
#include <qapplication.h>

#include <klocale.h>
#include <kdebug.h>
#include <kmenu.h>

#include "imgscaledialog.h"


//  Parameters for display and selection
//  ------------------------------------

#undef HOLD_SELECTION					// hold selection over new image

const int MIN_AREA_WIDTH = 2;				// minimum size of selection area
const int MIN_AREA_HEIGHT = 2;
const int DELTA = 3;					// tolerance for mouse hits
const int TIMER_INTERVAL = 100;				// animation timer interval

const int DASH_DASH = 4;				// length of drawn dashes
const int DASH_SPACE = 4;				// length of drawn spaces

const int DASH_LENGTH = (DASH_DASH+DASH_SPACE);


//  HighlightItem -- A graphics item to maintain and draw a single
//  highlight rectangle.

class HighlightItem : public QGraphicsItem
{
public:
    HighlightItem(const QRect &rect,
                  ImageCanvas::HighlightStyle style,
                  const QPen &pen, const QBrush &brush,
                  QGraphicsItem *parent = NULL);
    virtual ~HighlightItem()				{}

    virtual QRectF boundingRect() const;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                       QWidget *widget = NULL);

private:
    QRectF mRectangle;

    ImageCanvas::HighlightStyle mStyle;
    QPen mPen;
    QBrush mBrush;
};


HighlightItem::HighlightItem(const QRect &rect,
                             ImageCanvas::HighlightStyle style,
                             const QPen &pen, const QBrush &brush,
                             QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    mRectangle = rect;
    mStyle = style;
    mPen = pen;
    mBrush = brush;

    mPen.setCosmetic(true);
}


QRectF HighlightItem::boundingRect() const
{
    return (mRectangle);
}


void HighlightItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setPen(mPen);
    painter->setBrush(mBrush);

    if (mStyle==ImageCanvas::HighlightBox) painter->drawRect(mRectangle);
    else painter->drawLine(mRectangle.left(), mRectangle.bottom(),
                           mRectangle.right(), mRectangle.bottom());
}


//  SelectionItem -- A graphics item to maintain and draw the
//  selection rectangle.
//
//  There is only one of these items on the canvas.  Its bounding
//  rectangle, stored here in scene coordinates (i.e. image pixels)
//  is the master reference for the selection rectangle.

class SelectionItem : public QGraphicsItem
{
public:
    SelectionItem(QGraphicsItem *parent = NULL);
    virtual ~SelectionItem()				{}

    virtual QRectF boundingRect() const;
    void setRect(const QRectF &rect);

    void stepDashPattern();
    void resetDashPattern();

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                       QWidget *widget = NULL);

private:
    QRectF mRectangle;
    int mDashOffset;
};


SelectionItem::SelectionItem(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    kDebug();

    mDashOffset = 0;
}


QRectF SelectionItem::boundingRect() const
{
    return (mRectangle);
}


void SelectionItem::setRect(const QRectF &rect)
{
    prepareGeometryChange();
    mRectangle = rect;
}


void SelectionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setBrush(QBrush());

    QPen pen1(Qt::white);				// solid white box behind
    painter->setPen(pen1);
    painter->drawRect(mRectangle);

    QPen pen2(Qt::CustomDashLine);			// dashed black box on top
    QVector<qreal> dashes(2);
    dashes[0] = DASH_DASH;
    dashes[1] = DASH_SPACE;
    pen2.setDashPattern(dashes);
    pen2.setDashOffset(mDashOffset);
    painter->setPen(pen2);
    painter->drawRect(mRectangle);
}


// Counting backwards so that the "ants" march clockwise
// (at least with the current Qt painting implementation)
void SelectionItem::stepDashPattern()
{
    --mDashOffset;
    if (mDashOffset<0) mDashOffset = (DASH_LENGTH-1);
    update();
}


void SelectionItem::resetDashPattern()
{
    mDashOffset = 0;
}


//  ImageCanvas -- Scrolling area containing the pixmap/highlight widget.
//  Selected areas are drawn over the top of that.

ImageCanvas::ImageCanvas(QWidget *parent, const QImage *start_image)
    : QGraphicsView(parent)
{
    setObjectName("ImageCanvas");

    kDebug();

    mContextMenu = NULL;
    mTimerId = 0;

    mKeepZoom = false;
    mReadOnly = false;
    mScaleType = ImageCanvas::ScaleUnspecified;
    mDefaultScaleType = ImageCanvas::ScaleOriginal;
    mScaleFactor = 100;					// means original size
    mMaintainAspect = true;

    setAlignment(Qt::AlignLeft|Qt::AlignTop);

    mScene = new QGraphicsScene(this);
    setScene(mScene);

    mPixmapItem = new QGraphicsPixmapItem;
    mPixmapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    mScene->addItem(mPixmapItem);

    mSelectionItem = new SelectionItem;
    mSelectionItem->setVisible(false);			// not displayed yet
    mScene->addItem(mSelectionItem);

    mMoving = ImageCanvas::MoveNone;
    mCurrentCursor = Qt::ArrowCursor;

    newImage(start_image);

    setCursorShape(Qt::CrossCursor);
    setMouseTracking(true);

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

void ImageCanvas::newImage(const QImage *new_image, bool hold_zoom)
{
    mImage = new_image;					// don't free old image, not ours

#ifdef HOLD_SELECTION
    QRect oldSelected = mSelected;
    kDebug() << "original selection" << oldSelected << "null=" << oldSelected.isEmpty();
    kDebug() << "   w=" << mSelected.width() << "h=" << mSelected.height();
#endif

    stopMarqueeTimer();					// also clears selection

    if (mImage!=NULL)					// handle the new image
    {
        kDebug() << "new image size is" << mImage->size();
        mPixmapItem->setPixmap(QPixmap::fromImage(*mImage));
        setSceneRect(mPixmapItem->boundingRect());	// image always defines size

        if (!mKeepZoom && !hold_zoom) setScaleType(defaultScaleType());
    
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
        mPixmapItem->setPixmap(QPixmap());
    }

    recalculateViewScale();
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


void ImageCanvas::performUserAction(ImageCanvas::UserAction act)
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
        return;
    }

    recalculateViewScale();
}


//  Selected rectangle
//  ------------------

bool ImageCanvas::hasSelectedRect() const
{
    if (!hasImage()) return (false);
    return (mSelectionItem->isVisible() && mSelectionItem->boundingRect().isValid());
}


// Get the selected area in absolute image pixels.
QRect ImageCanvas::selectedRect() const
{
    if (!hasSelectedRect()) return (QRect());
    return (mSelectionItem->boundingRect().toRect());
}


// Get the selected area as a proportion of the image size.
QRectF ImageCanvas::selectedRectF() const
{
    if (!hasSelectedRect()) return (QRectF());
    const QRectF r = mSelectionItem->boundingRect();

    QRectF retval;
    retval.setLeft(r.left()/mImage->width());
    retval.setRight(r.right()/mImage->width());
    retval.setTop(r.top()/mImage->height());
    retval.setBottom(r.bottom()/mImage->height());
    return (retval);
}


// Set the selected area in absolute image pixels.
void ImageCanvas::setSelectionRect(const QRect &rect)
{
   kDebug() << "rect=" << rect;
   if (!hasImage()) return;

   if (!rect.isValid()) stopMarqueeTimer();		// clear the selection
   else							// set the selection
   {
       mSelectionItem->setRect(rect);
       startMarqueeTimer();
   }
}


// Set the selected area as a proportion of the image size.
void ImageCanvas::setSelectionRect(const QRectF &rect)
{
   kDebug() << "rect=" << rect;
   if (!hasImage()) return;

   if (!rect.isValid()) stopMarqueeTimer();		// clear the selection
   else							// set the selection
   {
       QRectF setval;
       setval.setLeft(rect.left()*mImage->width());
       setval.setRight(rect.right()*mImage->width());
       setval.setTop(rect.top()*mImage->height());
       setval.setBottom(rect.bottom()*mImage->height());

       mSelectionItem->setRect(setval);
       startMarqueeTimer();
   }
}


//  Selected image
//  --------------

QImage ImageCanvas::selectedImage() const
{
    if (!hasImage()) return (QImage());
							// no image available
    const QRect r = selectedRect();
    if (!r.isValid()) return (QImage());		// no selection
    return (mImage->copy(r));				// extract and return selection
}


//  Animation timer
//  ---------------

void ImageCanvas::timerEvent(QTimerEvent *)
{
   if (!hasImage()) return;				// no image acquired
   if (!isVisible()) return;				// we're not visible
   if (mMoving!=ImageCanvas::MoveNone) return;		// mouse operation in progress

   mSelectionItem->stepDashPattern();
}


void ImageCanvas::startMarqueeTimer()
{
    if (mTimerId==0) mTimerId = startTimer(TIMER_INTERVAL);
    mSelectionItem->setVisible(true);
}


void ImageCanvas::stopMarqueeTimer()
{
    if (mTimerId!=0)
    {
        killTimer(mTimerId);
        mTimerId = 0;
    }

    mSelectionItem->setVisible(false);			// clear the selection
    mSelectionItem->resetDashPattern();
}


//  Mouse events
//  ------------

void ImageCanvas::mousePressEvent(QMouseEvent *ev)
{
    if (mReadOnly) return;				// only if permitted
    if (ev->button()!=Qt::LeftButton) return;		// only action this button
    if (mMoving!=ImageCanvas::MoveNone) return;		// something already in progress
    if (!hasImage()) return;				// no image displayed

    const QList<QGraphicsItem *> its = items(ev->pos());
    if (its.isEmpty()) return;				// not over any item
    if (its.last()!=mPixmapItem) return;		// not within the image

    mMoving = classifyPoint(ev->pos());			// see where hit happened
    if (mMoving==ImageCanvas::MoveNone)			// starting new area
    {
        QPoint p = mapToScene(ev->pos()).toPoint();	// position in scene coords
        mStartPoint = p;
        mLastPoint = p;

        mSelectionItem->setRect(QRectF(p.x(), p.y(), 0, 0));
        mSelectionItem->setVisible(true);
        mMoving = ImageCanvas::MoveNew;
    }
}


void ImageCanvas::mouseReleaseEvent(QMouseEvent *ev)
{
    if (mReadOnly) return;				// only if permitted
    if (ev->button()!=Qt::LeftButton) return;		// only action this button
    if (mMoving==ImageCanvas::MoveNone) return;		// nothing in progress
    if (!hasImage()) return;				// no image displayed

    mMoving = ImageCanvas::MoveNone;

    QRect selected = selectedRect();
    kDebug() << "selected rect" << selected;
    if (selected.width()<MIN_AREA_WIDTH || selected.height()<MIN_AREA_HEIGHT)
    {
        kDebug() << "no selection";
        stopMarqueeTimer();				// also hides selection
        emit newRect(QRect());
        emit newRect(QRectF());
    }
    else
    {
        kDebug() << "have selection";
        startMarqueeTimer();
        emit newRect(selectedRect());
        emit newRect(selectedRectF());
    }

    mouseMoveEvent(ev);					// update cursor shape
}


void ImageCanvas::mouseMoveEvent(QMouseEvent *ev)
{
    if (mReadOnly) return;				// only if permitted
    if (!hasImage()) return;				// no image displayed

    int x = ev->pos().x();				// mouse position
    int y = ev->pos().y();				// in view coordinates

    int lx = mImage->width();				// limits for moved rectangle
    int ly = mImage->height();				// in scene coordinates

    QPoint pixExtent = mapFromScene(lx, ly);		// those same limits
    int ix = pixExtent.x();				// in view coordinates
    int iy = pixExtent.y();

    // Limit drag rectangle to the scaled pixmap
    if (x<0) x = 0;
    if (x>=ix) x = ix-1;
    if (y<0) y = 0;
    if (y>=iy) y = iy-1;

    QPoint scenePos = mapToScene(x, y).toPoint();	// that limited position
    int sx = scenePos.x();				// in scene coordinates
    int sy = scenePos.y();

    //kDebug() << x << y << "moving" << mMoving;

    // Set the cursor shape appropriately
    switch (mMoving!=ImageCanvas::MoveNone ? mMoving : classifyPoint(QPoint(x, y)))
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
        const QRectF originalRect = mSelectionItem->boundingRect();
        QRectF updatedRect = originalRect;
        switch (mMoving)
        {
case ImageCanvas::MoveNone:
            break;

case ImageCanvas::MoveTopLeft:
            if (sx<originalRect.right()) updatedRect.setLeft(sx);
            if (sy<originalRect.bottom()) updatedRect.setTop(sy);
            break;

case ImageCanvas::MoveTop:
            if (sy<originalRect.bottom()) updatedRect.setTop(sy);
            break;

case ImageCanvas::MoveTopRight:
            if (sx>originalRect.left()) updatedRect.setRight(sx);
            if (sy<originalRect.bottom()) updatedRect.setTop(sy);
            break;

case ImageCanvas::MoveRight:
            if (sx>originalRect.left()) updatedRect.setRight(sx);
            break;

case ImageCanvas::MoveBottomRight:
            if (sx>originalRect.left()) updatedRect.setRight(sx);
            if (sy>originalRect.top()) updatedRect.setBottom(sy);
            break;

case ImageCanvas::MoveBottom:
            if (sy>originalRect.top()) updatedRect.setBottom(sy);
            break;

case ImageCanvas::MoveBottomLeft:
            if (sx<originalRect.right()) updatedRect.setLeft(sx);
            if (sy>originalRect.top()) updatedRect.setBottom(sy);
            break;

case ImageCanvas::MoveLeft:
            if (sx<originalRect.right()) updatedRect.setLeft(sx);
            break;

case ImageCanvas::MoveNew:
            if (sx>mStartPoint.x())			// drag to the right
            {
                updatedRect.setLeft(mStartPoint.x());
                updatedRect.setRight(sx);
            }
            else					// drag to the left
            {
                updatedRect.setRight(mStartPoint.x());
                updatedRect.setLeft(sx);
            }

            if (sy>mStartPoint.y())			// drag down
            {
                updatedRect.setTop(mStartPoint.y());
                updatedRect.setBottom(sy);
            }
            else					// drag up
            {
                updatedRect.setBottom(mStartPoint.y());
                updatedRect.setTop(sy);
            }
            break;

case ImageCanvas::MoveWhole:
            int dx = sx-mLastPoint.x();
            int dy = sy-mLastPoint.y();

            QRectF r = updatedRect.translated(dx, dy);	// prospective new rectangle

            if (r.left()<0) dx -= r.left();		// limit to edges
            if (r.right()>=lx) dx -= (r.right()-lx+1);
            if (r.top()<0) dy -= r.top();
            if (r.bottom()>=ly) dy -= (r.bottom()-ly+1);

            updatedRect.translate(dx, dy);
            break;
        }

        if (updatedRect!=originalRect) mSelectionItem->setRect(updatedRect);
    }

    mLastPoint = scenePos;
}


void ImageCanvas::mouseDoubleClickEvent(QMouseEvent *ev)
{
    if (!hasImage()) return;				// no image displayed
							// convert to image pixels
    emit doubleClicked(mapToScene(ev->pos()).toPoint());
}


void ImageCanvas::resizeEvent(QResizeEvent *ev)
{
    QGraphicsView::resizeEvent(ev);
    recalculateViewScale();
}


//  Scaling settings
//  ----------------

void ImageCanvas::setScaleFactor(int i)
{
   kDebug() << "to" << i;
   mScaleFactor = i;
   if (i==0) setScaleType(ImageCanvas::ScaleDynamic);

   recalculateViewScale();
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
case ImageCanvas::ScaleZoom:	        return (i18n("Zoom %1%", mScaleFactor));
default:				return (i18n("Unknown"));
    }
}


//  Calculate an appropriate scale (i.e. view transform) for displaying the image
//  -----------------------------------------------------------------------------

void ImageCanvas::recalculateViewScale()
{
    if (!hasImage()) return;

    const int iw = mImage->width();			// original image size
    const int ih = mImage->height();
    const int aw = width()-2*frameWidth();		// available display size
    const int ah = height()-2*frameWidth();
							// width/height of scroll bar
    const int sbSize = QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);

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

    QTransform trans;
    trans.scale(xscale, yscale);
    kDebug() << "setting transform to" << trans;
    setTransform(trans);
}


//  Miscellaneous settings and information
//  --------------------------------------

void ImageCanvas::setCursorShape(Qt::CursorShape cs)
{
    if (mCurrentCursor!=cs)				// optimise no-change
    {
        setCursor(cs);
        mCurrentCursor = cs;
    }
}


void ImageCanvas::setReadOnly(bool ro)
{
    mReadOnly = ro;
    if (mReadOnly)
    {
        stopMarqueeTimer();				// clear selection
        setCursorShape(Qt::CrossCursor);		// ensure cursor is reset
    }
    emit imageReadOnly(ro);
}


const QString ImageCanvas::imageInfoString(int w, int h, int d)		// static
{
    return (i18n("%1x%2 pixels, %3 bpp", w, h, d));
}


const QString ImageCanvas::imageInfoString(const QImage *img)		// static
{
    if (img==NULL) return ("-");
    else return (imageInfoString(img->width(), img->height(), img->depth()));
}


const QString ImageCanvas::imageInfoString() const			// member
{
    return (imageInfoString(mImage));
}


bool ImageCanvas::hasImage() const
{
    return (mImage!=NULL && !mImage->isNull());
}


//  Highlight areas
//  ---------------

int ImageCanvas::addHighlight(const QRect &rect, bool ensureVis)
{
    HighlightItem *item = new HighlightItem(rect, mHighlightStyle,
                                            mHighlightPen, mHighlightBrush);

    int idx = mHighlights.indexOf(NULL);		// any empty slots?
    if (idx!=-1) mHighlights[idx] = item;		// yes, reuse that
    else						// no, append new item
    {
        idx = mHighlights.size();
        mHighlights.append(item);
    }

    mScene->addItem(item);
    if (ensureVis) scrollTo(rect);
    return (idx);
}


void ImageCanvas::removeAllHighlights()
{
    for (int idx = 0; idx<mHighlights.size(); ++idx) removeHighlight(idx);
}


void ImageCanvas::removeHighlight(int idx)
{
    if (idx<0 || idx>mHighlights.size()) return;

    QGraphicsItem *item = mHighlights[idx];
    if (item==NULL) return;

    mScene->removeItem(item);
    delete item;
    mHighlights[idx] = NULL;
}


void ImageCanvas::scrollTo(const QRect &rect)
{
    if (rect.isValid()) ensureVisible(rect);
}


void ImageCanvas::setHighlightStyle(ImageCanvas::HighlightStyle style,
                                    const QPen &pen, const QBrush &brush)
{
    mHighlightStyle = style;
    mHighlightPen = pen;
    mHighlightBrush = brush;
}


//  Mouse position on the selection rectangle
//  -----------------------------------------

// This works in view coordinates, in order that the DELTA tolerance
// is consistent regardless of the view scale.

ImageCanvas::MoveState ImageCanvas::classifyPoint(const QPoint &p) const
{
    if (!mSelectionItem->isVisible()) return (ImageCanvas::MoveNone);

    const QRect r = mapFromScene(mSelectionItem->boundingRect()).boundingRect().normalized();
    //kDebug() << p << "for rect" << r;
    if (r.isEmpty()) return (ImageCanvas::MoveNone);

    const bool onLeft = (abs(p.x()-r.left())<DELTA);
    const bool onRight = (abs(p.x()-r.right())<DELTA);

    const bool onTop = (abs(p.y()-r.top())<DELTA);
    const bool onBottom = (abs(p.y()-r.bottom())<DELTA);

    const bool withinRect = r.contains(p);
    const bool overRect= r.adjusted(-DELTA, -DELTA, DELTA, DELTA).contains(p);

    if (onLeft && onTop) return (ImageCanvas::MoveTopLeft);
    if (onLeft && onBottom) return (ImageCanvas::MoveBottomLeft);

    if (onRight && onTop) return (ImageCanvas::MoveTopRight);
    if (onRight && onBottom) return (ImageCanvas::MoveBottomRight);

    if (onLeft && overRect) return (ImageCanvas::MoveLeft);
    if (onRight && overRect) return (ImageCanvas::MoveRight);
    if (onTop && overRect) return (ImageCanvas::MoveTop);
    if (onBottom && overRect) return (ImageCanvas::MoveBottom);

    if (withinRect) return (ImageCanvas::MoveWhole);

    return (ImageCanvas::MoveNone);
}
