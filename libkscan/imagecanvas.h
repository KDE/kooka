/* This file is part of the KDE Project			-*- mode:c++; -*-
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

#ifndef IMAGECANVAS_H
#define IMAGECANVAS_H

#include "libkscanexport.h"

#include <qrect.h>
#include <qmatrix.h>
#include <qscrollarea.h>
#include <qpen.h>


class QPixmap;
class QLabel;

class KMenu;

class ImageCanvasWidget;


class KSCAN_EXPORT ImageCanvas : public QScrollArea
{
    Q_OBJECT
    Q_PROPERTY(int mBrightness READ getBrightness WRITE setBrightness)
    Q_PROPERTY(int mContrast READ getContrast WRITE setContrast)
    Q_PROPERTY(int mGamma READ getGamma WRITE setGamma)
    Q_PROPERTY(int mScaleFactor READ getScaleFactor WRITE setScaleFactor)
    Q_PROPERTY(ImageCanvas::ScaleType mDefaultScaleType READ defaultScaleType WRITE setDefaultScaleType)
    Q_PROPERTY(ImageCanvas::ScaleType mScaleType READ scaleType WRITE setScaleType)

public:
    ImageCanvas( QWidget *parent = NULL, const QImage *start_image = NULL);
    virtual ~ImageCanvas();

    enum ScaleType
    {
        ScaleUnspecified,
        ScaleDynamic,
        ScaleOriginal,
        ScaleFitWidth,
        ScaleFitHeight,
        ScaleZoom
    };

    enum UserAction
    {
        UserActionZoom,
        UserActionFitWidth,
        UserActionFitHeight,
        UserActionOrigSize,
        UserActionClose,
    };

    enum HighlightStyle
    {
        Box,
        Underline
    };

    KMenu *contextMenu();

    int getBrightness() const		{ return (mBrightness); }
    void setBrightness(int b)		{ mBrightness = b; }
    int getContrast() const		{ return (mContrast); }
    void setContrast(int c)		{ mContrast = c; }
    int getGamma() const		{ return (mGamma); }
    void setGamma(int g)		{ mGamma = g; }

    int getScaleFactor() const		{ return (mScaleFactor); }
    void setScaleFactor(int f);

    ImageCanvas::ScaleType defaultScaleType() const	{ return (mDefaultScaleType); }
    void setDefaultScaleType(ImageCanvas::ScaleType k)	{ mDefaultScaleType = k; }

    const QImage *rootImage() const	{ return (mImage); }
    bool hasImage() const		{ return (mAcquired); }
    bool readOnly() const		{ return (mReadOnly); }

    QRect selectedRect() const;
    void setSelectionRect(const QRect &rect);
    QImage selectedImage() const;

    ImageCanvas::ScaleType scaleType() const;
    void setScaleType(ImageCanvas::ScaleType type);
    const QString scaleTypeString() const;
    const QString imageInfoString(int w = 0, int h = 0, int d = 0) const;

    void newImage(const QImage *new_image, bool hold_zoom = false);
    void newImageHoldZoom(const QImage *new_image);
    void deleteView(const QImage *delimage);

    /**
     * Highlight a rectangular area on the current image using the given brush
     * and pen.
     * The function returns a id that needs to be given to the remove method.
     */
    int highlight(const QRect &rect, bool ensureVis = false);

    /**
     * reverts the highlighted region back to normal view.
     */
    void removeHighlight(int idx = -1);

    void setHighlightStyle(ImageCanvas::HighlightStyle style, const QPen &pen = QPen(), const QBrush &brush = QBrush());
    void scrollTo(const QRect &rect);

public slots:
    void setKeepZoom(bool k)		{ mKeepZoom = k; }
    void setMaintainAspect(bool ma);
    void setReadOnly(bool ro);

    void slotUserAction(int act);			// ImageCanvas::UserAction

signals:
    void newRect(const QRect &rect);
    void scalingRequested();
    void closingRequested();
    void scalingChanged(const QString &scaleType);

    /**
     * signal emitted if the permission of the currently displayed image changed,
     * ie. if it goes from writeable to readable.
     * @param shows if the image is now read only (true) or not.
     */
    void imageReadOnly(bool isRO);
    
protected:
    void timerEvent(QTimerEvent *ev);
    void resizeEvent(QResizeEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);
    void mousePressEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);

private:
    enum MoveState
    {
        MoveNone,
        MoveTopLeft,
        MoveTopRight,
        MoveBottomLeft,
        MoveBottomRight,
        MoveLeft,
        MoveRight,
        MoveTop,
        MoveBottom,
        MoveWhole,
        MoveNew
    };

    int contentsX() const;				// Q3ScrollView compatibility
    int contentsY() const;

    void startMarqueeTimer();
    void stopMarqueeTimer();

    void setCursorShape(Qt::CursorShape cs);
    ImageCanvas::MoveState classifyPoint(int x, int y) const;

    /* private functions for the running ant */
    void drawHAreaBorder(QPainter *p, int x1, int x2, int y, bool remove);
    void drawVAreaBorder(QPainter *p, int x, int y1, int y2, bool remove);
    void drawAreaBorder(QPainter *p, bool remove = false);

    void updateScaledPixmap();

    KMenu *mContextMenu;
    int mTimerId;

    const QImage *mImage;

    int mScaleFactor;
    int mBrightness;
    int mContrast;
    int mGamma;

    QMatrix mScaleMatrix;
    QMatrix mInvScaleMatrix;
    bool mMaintainAspect;
    QPixmap mScaledPixmap;
    ImageCanvasWidget *mPixmapLabel;

    QRect mSelected;
    ImageCanvas::MoveState mMoving;
    Qt::CursorShape mCurrentCursor;
    int mAreaState1;
    int mAreaState2;
    int mLastX;
    int mLastY;

    bool mAcquired;
    bool mKeepZoom;					// keep zoom setting if image changes
    bool mReadOnly;

    ImageCanvas::ScaleType mScaleType;
    ImageCanvas::ScaleType mDefaultScaleType;

    class ImageCanvasPrivate;
    ImageCanvasPrivate *d;
};


#endif							// IMAGECANVAS_H
