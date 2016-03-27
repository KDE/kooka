/************************************************** -*- mode:c++; -*- ***
 *                                  *
 *  This file is part of libkscan, a KDE scanning library.      *
 *                                  *
 *  Copyright (C) 2013 Jonathan Marten <jjm@keelhaul.me.uk>     *
 *  Copyright (C) 1999 Klaas Freitag <freitag@suse.de>          *
 *                                  *
 *  This library is free software; you can redistribute it and/or   *
 *  modify it under the terms of the GNU Library General Public     *
 *  License as published by the Free Software Foundation and appearing  *
 *  in the file COPYING included in the packaging of this file;     *
 *  either version 2 of the License, or (at your option) any later  *
 *  version.                                *
 *                                  *
 *  This program is distributed in the hope that it will be useful, *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of  *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   *
 *  GNU General Public License for more details.            *
 *                                  *
 *  You should have received a copy of the GNU General Public License   *
 *  along with this program;  see the file COPYING.  If not, write to   *
 *  the Free Software Foundation, Inc., 51 Franklin Street,     *
 *  Fifth Floor, Boston, MA 02110-1301, USA.                *
 *                                  *
 ************************************************************************/

#ifndef IMAGECANVAS_H
#define IMAGECANVAS_H

#include "kookascan_export.h"

#include <qgraphicsview.h>
#include <qvector.h>

class QGraphicsScene;
class QGraphicsPixmapItem;

class QMenu;

class SelectionItem;

/**
 * @short Image display canvas widget.
 *
 * Displays a scalable image in a scrolling area, along with an interactive
 * selection area and any number of optional highlight areas.
 *
 * The selection area is used for selecting a part of the image for scanning
 * or cropping, while the highlight areas are used for indicating words
 * while performing OCR spell checking.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class KOOKASCAN_EXPORT ImageCanvas : public QGraphicsView
{
    Q_OBJECT

public:
    /**
     * Create the image canvas widget.
     *
     * @param parent The parent widget.
     * @param start_image The initial image to display.  If this is
     * not specified, nothing is displayed until an image is set
     * using @c newImage().
     **/
    ImageCanvas(QWidget *parent = Q_NULLPTR, const QImage *start_image = NULL);

    /**
     * Destructor.
     **/
    virtual ~ImageCanvas();

    /**
     * Scaling types for the image as displayed on the canvas.
     **/
    enum ScaleType {
        ScaleUnspecified,           /**< No scale specified */
        ScaleDynamic,               /**< Best fit */
        ScaleOriginal,              /**< Original size */
        ScaleFitWidth,              /**< Fit to width */
        ScaleFitHeight,             /**< Fit to height */
        ScaleZoom               /**< Fit to arbitrary zoom */
    };

    /**
     * Request for the canvas to perform an action.  This will usually be
     * as a result of a user action in the application.
     *
     * @see ImgScaleDialog
     **/
    enum UserAction {
        UserActionZoom,             /**< Display an @c ImgScaleDialog to set a zoom factor */
        UserActionFitWidth,         /**< Scale the image to fit the width */
        UserActionFitHeight,            /**< Scale the image to fit the height */
        UserActionOrigSize,         /**< Reset the scale to the original image size */
        UserActionClose,            /**< Emit the @c closingRequested() signal */
    };

    /**
     * The style used to draw a highlight box.
     **/
    enum HighlightStyle {
        HighlightBox,               /**< A rectangular box */
        HighlightUnderline          /**< A line along the bottom box edge */
    };

    /**
     * Get a context menu for the image canvas.
     *
     * The menu is created on demand, and the first time that this function
     * is called an empty menu will be returned.  The calling application
     * must populate the menu with actions and handle them when they are
     * triggered;  if an action on the canvas is required, it should request
     * them via @c slotUserAction().  Do not delete the returned menu,
     * it is owned by the @c ImageCanvas object.
     *
     * @return The context menu
     * @see slotUserAction
     **/
    QMenu *contextMenu();

    /**
     * Set the brightness of the displayed image.
     *
     * @param b The new brightness value
     * @note Not currently implemented, the value specified will be ignored.
     **/
    void setBrightness(int b)
    {
        mBrightness = b;
    }

    /**
     * Set the contrast of the displayed image.
     *
     * @param c The new contrast value
     * @note Not currently implemented, the value specified will be ignored.
     **/
    void setContrast(int c)
    {
        mContrast = c;
    }

    /**
     * Set the gamma of the displayed image.
     *
     * @param g The new gamma value
     * @note Not currently implemented, the value specified will be ignored.
     **/
    void setGamma(int g)
    {
        mGamma = g;
    }

    /**
     * Get the current brightness setting.
     *
     * @return The brightness value
     * @note Not currently implemented, an undefined value will be returned.
     **/
    int getBrightness() const
    {
        return (mBrightness);
    }

    /**
     * Get the current contrast setting.
     *
     * @return The contrast value
     * @note Not currently implemented, an undefined value will be returned.
     **/
    int getContrast() const
    {
        return (mContrast);
    }

    /**
     * Get the current gamma setting.
     *
     * @return The gamma value
     * @note Not currently implemented, an undefined value will be returned.
     **/
    int getGamma() const
    {
        return (mGamma);
    }

    /**
     * Set the scale factor to be used for display.  The image is immediately
     * resized in accordance with the scale factor.
     *
     * @param f The scale factor as a percentage. 100 means original size,
     * while 0 means dynamic (best fit) scaling.
     **/
    void setScaleFactor(int f);

    /**
     * Get the scale factor currently in use.
     *
     * @return The current scale factor as a percentage.
     * Original size scaling returns a value of 100, dynamic
     * scaling returns a value of 0.
     **/
    int getScaleFactor() const
    {
        return (mScaleFactor);
    }

    /**
     * Set the scale type to be used for display.  The image is immediately
     * resized in accordance with the scale type.
     *
     * @param type The new scale type to be set
     * @note Do not use this function to set a @p type of @c ScaleZoom.
     * Use @c setScaleFactor with the required scale factor instead.
     * @see ScaleType
     **/
    void setScaleType(ImageCanvas::ScaleType type);

    /**
     * Get the current scaling type in use.
     *
     * @return The scale type
     * @see ScaleType
     **/
    ImageCanvas::ScaleType scaleType() const;

    /**
     * Get a textual description for the current scaling type.
     *
     * @return An I18N'ed description
     **/
    const QString scaleTypeString() const;

    /**
     * Set the default scaling type that will be applied to a new image,
     * either on construction or after setting a new image without the
     * zoom hold in effect.
     *
     * @param type The new default scale type
     * @note If this function is not used. the default scaling type is @c Original.
     * @note It is not useful to use this function to set a @p type of @c ScaleZoom.
     **/
    void setDefaultScaleType(ImageCanvas::ScaleType type)
    {
        mDefaultScaleType = type;
    }

    /**
     * Get the default scaling type that will be applied to a new image.
     *
     * @return The default scale type
     **/
    ImageCanvas::ScaleType defaultScaleType() const
    {
        return (mDefaultScaleType);
    }

    /**
     * Access the image displayed.
     *
     * @return The image, or @c NULL if no image is currently set.
     **/
    const QImage *rootImage() const
    {
        return (mImage);
    }

    /**
     * Check whether an image is currently set and displayed
     *
     * @return @c true if an image is currently set
     **/
    bool hasImage() const;

    /**
     * Check whether the image canvas is read-only.
     *
     * @return @c true if the image is read-only, @c false if it allows interaction.
     * @see setReadOnly
     * @see imageReadOnly
     **/
    bool isReadOnly() const
    {
        return (mReadOnly);
    }

    /**
     * Get the bounds of the currently selected area, in absolute image pixels.
     *
     * @return The selected rectangle, in source image pixels.
     * If there is no selection, a null (invalid) rectangle is returned.
     * @see setSelectionRect
     * @see QRect::isValid()
     **/
    QRect selectedRect() const;

    /**
     * Get the bounds of the currently selected area, as a scaled proportion of
     * the image size.
     *
     * @return The selected rectangle, scaled to the image size:
     * for example, 0.5 means 50% of the image width or height.
     * If there is no selection, a null (invalid) rectangle is returned.
     * @see setSelectionRect
     * @see QRectF::isValid()
     **/
    QRectF selectedRectF() const;

    /**
     * Check whether there is a currently selected area.
     *
     * @return @c true if there is a selection.
     * @see selectedRect
     * @see selectedRectF
     **/
    bool hasSelectedRect() const;

    /**
     * Set a new selected area, in absolute image pixels.
     *
     * @param rect The new selected rectangle, in source image pixels.
     * @see selectedRect
     **/
    void setSelectionRect(const QRect &rect);

    /**
     * Set a new selected area, as a scaled proportion of
     * the image size.
     *
     * @param rect The new selected rectangle, in source image pixels:
     * for example, 0.5 means 50% of the image width or height.
     * @see selectedRectF
     **/
    void setSelectionRect(const QRectF &rect);

    /**
     * Get a copy of the selected image area.
     *
     * @return A copy of the selected image area, or a null image if there
     * is no image displayed or if there is no selected area.
     **/
    QImage selectedImage() const;

    /**
     * Get a textual description of the image size and depth.
     *
     * @return The I18N'ed description, in the format "WxH pixels, D bpp",
     * or "-" if no image is set.
     **/
    const QString imageInfoString() const;

    /**
     * As above, but return a description for the specified
     * width, height and depth.
     *
     * @param w Width
     * @param h Height
     * @param d Bit depth
     * @return The description
     **/
    static const QString imageInfoString(int w, int h, int d);

    /**
     * As above, but return a description for the specified image.
     *
     * @param img The image
     * @return Its description
     **/
    static const QString imageInfoString(const QImage *img);

    /**
     * Display a new image.
     *
     * The old image is forgotten (but not deleted).  The selection is
     * cleared, but the newRect() signals are not emitted.  All current
     * highlights are removed.  Unless the @p hold_zoom option is set
     * or @c setKeepZoom(true) has been called, the scaling type is
     * reset to the default.
     *
     * @param new_image The new image to display.  If this is @c NULL,
     * no new image is set.
     * @param hold_zoom If set to @true, do not change the current
     * scaling type or scaling factor;  if set to @false, reset the
     * scaling type to that set by @c setDefaultScaleType().
     * @see setDefaultScaleType
     * @see setKeepZoom
     **/
    void newImage(const QImage *new_image, bool hold_zoom = false);

    /**
     * Highlight a rectangular area on the current image,
     * using the previously specified style, brush and pen.
     *
     * @param rect The rectangle to be highlighted.  Unlike the selection
     * rectangle, this is specified in absolute image pixels.
     * @param ensureVis If set to @c true, the new highlight rectangle
     * will be made visible by scrolling to it if necessary.
     * @return The new highlight ID.
     * @see removeHighlight
     * @see setHighlightStyle
     * @see scrollTo
     */
    int addHighlight(const QRect &rect, bool ensureVis = false);

    /**
     * Remove a highlight from the image.
     *
     * @param id The ID of the highlight to be removed.
     * @see addHighlight
     */
    void removeHighlight(int id);

    /**
     * Remove all highlights from the image.
     *
     * @see addHighlight
     */
    void removeAllHighlights();

    /**
     * Set the style, pen and brush to be used for subequently
     * added highlight rectangles.
     *
     * @param style The new style
     * @param pen The new line pen
     * @param brush The new fill brush
     * @note The default @p style is HighlightBox, but there are no defaults
     * for @p pen or @p brush.
     * @note The @p pen is always forced to be cosmetic.
     * @see QPen::setCosmetic
     **/
    void setHighlightStyle(ImageCanvas::HighlightStyle style, const QPen &pen = QPen(), const QBrush &brush = QBrush());

    /**
     * Scroll if necessary so that the specified rectangle is visible.
     * The rectangle is specified in absolute image pixels.
     *
     * @param rect The rectangle area of interest
     * @see QGraphicsView::ensureVisible
     **/
    void scrollTo(const QRect &rect);

public slots:
    /**
     * Set whether the current scaling settings are retained when a
     * new image is set.
     *
     * @param k The new setting.  If @true, the scaling settings are always
     * retained when setting a new image, regardless of the @c hold_zoom
     * parameter to @c newImage().  The default is @c false.
     * @see newImage
     **/
    void setKeepZoom(bool k)
    {
        mKeepZoom = k;
    }

    /**
     * Set whether the aspect ratio of the image is maintained for
     * dynamic scaling mode.
     *
     * @param ma The new setting.  If @c true, the aspect ratio of the
     * image will be maintained;  if @c false, the width and height will
     * be scaled independently for the best fit.  The default is @c true.
     **/
    void setMaintainAspect(bool ma)
    {
        mMaintainAspect = ma;
    }

    /**
     * Set whether the image is considered to be read-only or whether
     * it allows user interaction.  Setting the read-only status using
     * this function causes the imageReadOnly() signal to be emitted.
     *
     * @param ro If set to @c true, the image is set to be read-only.
     * The default is @c false allowing user interaction.
     * @note If the image is read-only mouse actions are ignored and the
     * selection cannot be set interactively, although it can still be set
     * by calling the appropriate functions.
     * @see readOnly
     * @see imageReadOnly
     **/
    void setReadOnly(bool ro);

    /**
     * Perform a user-requested action on the image.
     *
     * @param act The action to be performed
     * @note For all scaling setting actions (i.e. all apart
     * from @c UserActionClose), the image is repainted at
     * the new setting.  The @c UserActionClose action emits
     * the @c closingRequested() signal.
     * @see UserAction
     * @see closingRequested
     **/
    void performUserAction(ImageCanvas::UserAction act);

signals:
    /**
     * Emitted when a new selection rectangle is created by the user
     * dragging an area.  The rectangle is provided as absolute image pixels.
     *
     * @param rect The new rectangle, the same area and in the same units
     * as would be returned by @c selectedRect(), or a null @c QRect if
     * there is no selection.
     * @see selectedRect
     **/
    void newRect(const QRect &rect);

    /**
     * Emitted when a new selection rectangle is created by the user
     * dragging an area.  The rectangle is provided as a proportion of
     * the source image size.
     *
     * @param rect The new rectangle, the same area and in the same units
     * as would be returned by @c selectedRectF(), or a null @c QRectF if
     * there is no selection.
     * @see selectedRectF
     **/
    void newRect(const QRectF &rect);

    /**
     * Emitted when the user requests to close the image,
     * by calling @c slotUserAction() with @c UserActionClose.
     **/
    void closingRequested();

    /**
     * Emitted when the scaling type is set by @c setScaleType().
     * Can be monitored by the calling application in order to set
     * a status bar indicator or similar.
     *
     * @param scaleType The new scale type as a string, as would
     * be returned by scaleTypeString().
     * @see scaleTypeString
     * @see setScaleFactor
     **/
    void scalingChanged(const QString &scaleType);

    /**
     * Emitted when the read-only state of the current image changes.
     *
     * @param isRO Set to @c true if the image is now read-only,
     * or @c false if it is not.
     */
    void imageReadOnly(bool isRO);

    /**
     * Emitted for a mouse double-click over the image.
     *
     * @param p The clicked point, in absolute image pixel coordinates.
     * @note This signal is emitted even if the image is read-only.
     */
    void doubleClicked(const QPoint &p);

protected:
    /**
     * Update and redraw the "marching ants" area rectangle.
     *
     * @param ev The timer event
     **/
    void timerEvent(QTimerEvent *ev);

    /**
     * Resize and redraw the currently displayed image to fit the new size.
     *
     * @param ev The resize event
     **/
    void resizeEvent(QResizeEvent *ev);

    /**
     * If a context menu has been created by calling @c contextMenu(),
     * then pop it up at the current mouse position.  If there is no
     * context menu, then do nothing.
     *
     * @param ev The menu event
     **/
    void contextMenuEvent(QContextMenuEvent *ev);

    /**
     * Possibly start to drag a new selection area, or move or resize
     * the current area.
     *
     * @param ev The mouse event
     **/
    void mousePressEvent(QMouseEvent *ev);

    /**
     * Finish dragging a selection area, and emit the @c newRect signals.
     *
     * @param ev The mouse event
     * @see newRect
     **/
    void mouseReleaseEvent(QMouseEvent *ev);

    /**
     * Continue to drag and redraw the selection area.
     *
     * @param ev The mouse event
     **/
    void mouseMoveEvent(QMouseEvent *ev);

    /**
     * Detect a double click on the image and emit a signal with
     * the image coordinates.
     *
     * @param ev The mouse event
     * @see doubleClicked
     **/
    void mouseDoubleClickEvent(QMouseEvent *ev);

private:
    enum MoveState {
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

    void startMarqueeTimer();
    void stopMarqueeTimer();

    ImageCanvas::MoveState classifyPoint(const QPoint &p) const;
    void setCursorShape(Qt::CursorShape cs);
    void recalculateViewScale();

    QMenu *mContextMenu;
    int mTimerId;

    const QImage *mImage;

    int mScaleFactor;
    int mBrightness;
    int mContrast;
    int mGamma;

    bool mMaintainAspect;

    ImageCanvas::MoveState mMoving;
    Qt::CursorShape mCurrentCursor;

    bool mKeepZoom;                 // keep zoom setting if image changes
    bool mReadOnly;

    ImageCanvas::ScaleType mScaleType;
    ImageCanvas::ScaleType mDefaultScaleType;

    QGraphicsScene *mScene;             // internal graphics scene
    QGraphicsPixmapItem *mPixmapItem;           // item for background pixmap
    SelectionItem *mSelectionItem;          // item for selection box

    QPoint mStartPoint;                 // start point of drag
    QPoint mLastPoint;                  // previous point of drag

    QVector<QGraphicsItem *> mHighlights;       // list of highlight rectangles

    ImageCanvas::HighlightStyle mHighlightStyle;    // for newly created highlights
    QPen mHighlightPen;
    QBrush mHighlightBrush;
};

#endif                          // IMAGECANVAS_H
