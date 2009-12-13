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
#include <q3scrollview.h>
#include <qpixmap.h>

class KMenu;

// TODO: these two enums into class
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

// TODO: these to cpp file
const int MIN_AREA_WIDTH = 3;
const int MIN_AREA_HEIGHT = 3;
const int delta = 3;


class KSCAN_EXPORT ImageCanvas : public Q3ScrollView
{
    Q_OBJECT
    Q_ENUMS( PopupIDs )
    Q_PROPERTY( int brightness READ getBrightness WRITE setBrightness )
    Q_PROPERTY( int contrast READ getContrast WRITE setContrast )
    Q_PROPERTY( int gamma READ getGamma WRITE setGamma )
    Q_PROPERTY( int scale_factor READ getScaleFactor WRITE setScaleFactor )

public:
    ImageCanvas( QWidget *parent = NULL,
                 const QImage *start_image = NULL);
    virtual ~ImageCanvas();

    KMenu *contextMenu();

    virtual QSize sizeHint() const;

    int getBrightness() const { return (brightness); };
    int getContrast() const { return (contrast); };
    int getGamma() const { return (gamma); };
    int getScaleFactor() const { return (scale_factor); };

    const QImage *rootImage() const { return (image); };

    bool hasImage() const { return (acquired); }
    QRect sel() const;

    enum ScaleKinds { UNSPEC, DYNAMIC, FIT_ORIG, FIT_WIDTH, FIT_HEIGHT, ZOOM };

    enum UserAction
    {
        UserActionZoom,
        UserActionFitWidth,
        UserActionFitHeight,
        UserActionOrigSize,
        UserActionClose,
    };

// TODO: parameter names
    bool selectedImage( QImage* );

    ScaleKinds scaleKind();
    const QString scaleKindString();

    ScaleKinds defaultScaleKind();

    const QString imageInfoString( int w=0, int h=0, int d=0 );

    void setSelectionRect(const QRect &rect);

public slots:
// TODO: parameter names, do these need to be slots?
    void setBrightness(int);
    void setContrast(int );
    void setGamma(int );

    void toggleAspect(int aspect_in_mind);

    void newImage(const QImage *new_image, bool hold_zoom = false);
    void newImageHoldZoom(const QImage *new_image);
    void deleteView(const QImage *delimage);
    void setScaleFactor( int i );
    void setKeepZoom( bool k );
    void setScaleKind( ScaleKinds k );
    void setDefaultScaleKind( ScaleKinds k );

    void slotUserAction(int act);			// ImageCanvas::UserAction

    /**
     * Highlight a rectangular area on the current image using the given brush
     * and pen.
     * The function returns a id that needs to be given to the remove method.
     */
// TODO: parameter names
    int highlight( const QRect&, const QPen&, const QBrush&, bool ensureVis=false );

    /**
     * reverts the highlighted region back to normal view.
     */
    void removeHighlight( int idx = -1 );

    /**
     * permit to do changes to the image that are saved back to the file
     */
// TODO: parameter names
    void setReadOnly( bool );

    bool readOnly();
	
signals:
    void newRect( QRect );
    void scalingRequested();
    void closingRequested();
    void scalingChanged( const QString& );
    /**
     * signal emitted if the permission of the currently displayed image changed,
     * ie. if it goes from writeable to readable.
     * @param shows if the image is now read only (true) or not.
     */
    void imageReadOnly( bool isRO );
    
protected:
    void drawContents( QPainter * p, int clipx, int clipy, int clipw, int cliph );

    void timerEvent(QTimerEvent *ev);
    void resizeEvent(QResizeEvent *ev);
    void viewportMousePressEvent(QMouseEvent *ev);
    void viewportMouseReleaseEvent(QMouseEvent *ev);
    void viewportMouseMoveEvent(QMouseEvent *ev);
    void contentsContextMenuEvent(QContextMenuEvent *ev);

private:
    void startMarqueeTimer();
    void stopMarqueeTimer();

    int           scale_factor;
    const QImage        *image;
    int           brightness, contrast, gamma;


#ifdef USE_KPIXMAPIO
    KPixmapIO	 pixIO;
#endif

    QMatrix	 scale_matrix;
    QMatrix	 inv_scale_matrix;
    QPixmap       pmScaled;
    float	 used_yscaler;
    float	 used_xscaler;
    KMenu    *mContextMenu;
    bool		 maintain_aspect;

    int	         timer_id;
    QRect selected;
    preview_state moving;
    int 		 cr1,cr2;
    int 		 lx,ly;
    bool 	 acquired;

    /* private functions for the running ant */
    void drawHAreaBorder(QPainter &p,int x1,int x2,int y,bool remove);
    void drawVAreaBorder(QPainter &p,int x,int y1,int y2,bool remove);
    void drawAreaBorder(QPainter *p,bool remove = false);
    void update_scaled_pixmap( void );

    preview_state classifyPoint(int x,int y);

    class ImageCanvasPrivate;
    ImageCanvasPrivate *d;
};

#endif							// IMAGECANVAS_H
