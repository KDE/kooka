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

#ifndef __IMG_CANVAS_H__
#define __IMG_CANVAS_H__

#include <qrect.h>
#include <qwmatrix.h>
#include <qscrollview.h>

class QPopupMenu;

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


class ImageCanvas: public QScrollView
{
    Q_OBJECT
    Q_ENUMS( PopupIDs )
    Q_PROPERTY( int brightness READ getBrightness WRITE setBrightness )
    Q_PROPERTY( int contrast READ getContrast WRITE setContrast )
    Q_PROPERTY( int gamma READ getGamma WRITE setGamma )
    Q_PROPERTY( int scale_factor READ getScaleFactor WRITE setScaleFactor )

public:
    ImageCanvas( QWidget *parent = 0,
                 const QImage *start_image = 0,
                 const char *name = 0);
    ~ImageCanvas( );
    virtual QSize sizeHint() const;

    int getBrightness() const;
    int getContrast() const;
    int getGamma() const;

    int getScaleFactor() const;

    const QImage *rootImage();

    bool hasImage( void ) 	{ return acquired; }
    QPopupMenu* contextMenu() { return m_contextMenu; }
    QRect sel( void );

    enum ScaleKinds { UNSPEC, DYNAMIC, FIT_ORIG, FIT_WIDTH, FIT_HEIGHT, ZOOM };

    enum PopupIDs { ID_POP_ZOOM, ID_POP_CLOSE, ID_FIT_WIDTH,
                    ID_FIT_HEIGHT, ID_ORIG_SIZE };

    bool selectedImage( QImage* );

    ScaleKinds scaleKind();
    const QString scaleKindString();

    ScaleKinds defaultScaleKind();

    const QString imageInfoString( int w=0, int h=0, int d=0 );

    void setSelectionRect(const QRect &rect);

public slots:
    void setBrightness(int);
    void setContrast(int );
    void setGamma(int );

    void toggleAspect( int aspect_in_mind )
        {
            maintain_aspect = aspect_in_mind;
            repaint();
        }

    void newImage(const QImage *new_image,bool hold_zoom = false);
    void newImageHoldZoom(const QImage *new_image);
    void deleteView(const QImage *delimage);
    void setScaleFactor( int i );
    void handle_popup(int item );
    void enableContextMenu( bool wantContextMenu );
    void setKeepZoom( bool k );
    void setScaleKind( ScaleKinds k );
    void setDefaultScaleKind( ScaleKinds k );

    /**
     * Highlight a rectangular area on the current image using the given brush
     * and pen.
     * The function returns a id that needs to be given to the remove method.
     */
    int highlight( const QRect&, const QPen&, const QBrush&, bool ensureVis=false );

    /**
     * reverts the highlighted region back to normal view.
     */
    void removeHighlight( int idx = -1 );

    /**
     * permit to do changes to the image that are saved back to the file
     */
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

    void timerEvent(QTimerEvent *);
    void viewportMousePressEvent(QMouseEvent *);
    void viewportMouseReleaseEvent(QMouseEvent *);
    void viewportMouseMoveEvent(QMouseEvent *);

    void resizeEvent( QResizeEvent * event );

private:
    void startMarqueeTimer();
    void stopMarqueeTimer();

    QStrList      urls;

    int           scale_factor;
    const QImage        *image;
    int           brightness, contrast, gamma;


#ifdef USE_KPIXMAPIO
    KPixmapIO	 pixIO;
#endif

    QWMatrix	 scale_matrix;
    QWMatrix	 inv_scale_matrix;
    QPixmap       *pmScaled;
    float	 used_yscaler;
    float	 used_xscaler;
    QPopupMenu    *m_contextMenu;
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

#endif
