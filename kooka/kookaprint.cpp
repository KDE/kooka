/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2003-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include "kookaprint.h"

#include <math.h>

#include <qpainter.h>
#include <qfontmetrics.h>
#include <qguiapplication.h>
#include <qdebug.h>

#include <klocalizedstring.h>

#include "imgprintdialog.h"
#include "kookaimage.h"


#define CUT_MARGIN		5			// margin in millimetres

// TODO: somewhere common (in libkookascan)
#define DPM_TO_DPI(d)		qRound((d)*2.54/100)	// dots/metre -> dots/inch
#define DPI_TO_DPM(d)		qRound((d)*100/2.54)	// dots/inch -> dots/metre



KookaPrint::KookaPrint()
    : QPrinter(QPrinter::HighResolution)
{
    qDebug();
    m_painter = NULL;
    m_image = NULL;
    m_extraMarginPercent = 10;

    // Initial default print parameters
    m_scaleOption = KookaPrint::ScaleScreen;
    m_printSize = QSize(100, 100);
    m_maintainAspect = true;
    m_lowResDraft = false;
    m_screenResolution = -1;				// set by caller
    m_scanResolution = -1;				// from image
    m_cutsOption = KookaPrint::CutMarksMultiple;
}


void KookaPrint::recalculatePrintParameters()
{
    if (m_image==NULL) return;				// no image to print

    qDebug() << "image:";
    qDebug() << "  size (pix) =" << m_image->size();
    qDebug() << "  dpi X =" << DPM_TO_DPI(m_image->dotsPerMeterX());
    qDebug() << "  dpi Y =" << DPM_TO_DPI(m_image->dotsPerMeterY());
    qDebug() << "printer:";
    qDebug() << "  name =" << printerName();
    qDebug() << "  colour mode =" << colorMode();
    qDebug() << "  full page?" << fullPage();
    qDebug() << "  output format =" << outputFormat();
    qDebug() << "  paper rect (mm) =" << paperRect(QPrinter::Millimeter);
    qDebug() << "  page rect (mm) =" << pageRect(QPrinter::Millimeter);
    qDebug() << "  resolution =" << resolution();
    qDebug() << "options:";
    qDebug() << "  scale mode =" << m_scaleOption;
    qDebug() << "  print size (mm) =" << m_printSize;
    qDebug() << "  scan resolution =" << m_scanResolution;
    qDebug() << "  screen resolution =" << m_screenResolution;
    qDebug() << "  cuts option =" << m_cutsOption;
    qDebug() << "  maintain aspect?" << m_maintainAspect;
    qDebug() << "  low res draft?" << m_lowResDraft;

    // Calculate the available page size, in real world units
    QRectF r = pageRect(QPrinter::Millimeter);
    mPageWidthMm = r.width();
    mPageHeightMm = r.height();

    // Calculate the size at which the image is to be printed,
    // depending on the scaling option.

    mImageWidthPix = m_image->width();			// image size in pixels
    mImageHeightPix = m_image->height();

//     double printWidthMm;				// print size of the image
//     double printHeightMm;

    if (m_scaleOption==KookaPrint::ScaleScan)		// Original scan size
    {
        const int imageRes = m_scanResolution!=-1 ? DPI_TO_DPM(m_scanResolution) : m_image->dotsPerMeterX();
	Q_ASSERT(imageRes>0);				// dots per metre
        mPrintWidthMm = double(mImageWidthPix)/imageRes*1000;
        mPrintHeightMm = double(mImageHeightPix)/imageRes*1000;
    }
    else if (m_scaleOption==KookaPrint::ScaleScreen)	// Scale to screen resolution
    {
        int screenRes = DPI_TO_DPM(m_screenResolution);
        Q_ASSERT(screenRes>0);				// dots per metre
        mPrintWidthMm = double(mImageWidthPix)/screenRes*1000;
        mPrintHeightMm = double(mImageHeightPix)/screenRes*1000;
    }
    else if (m_scaleOption==KookaPrint::ScaleCustom)	// Custom size
    {
        // For this option, "Maintain aspect ratio" can be enabled in the GUI.
        // There is however no need to take account of it here, because the
        // values are already scaled/adjusted there.

        mPrintWidthMm = double(m_printSize.width());
        mPrintHeightMm = double(m_printSize.height());
        Q_ASSERT(mPrintWidthMm>0 && mPrintHeightMm>0);
    }
    else if (m_scaleOption==KookaPrint::ScaleFitPage)	// Fit to one page
    {
        mPrintWidthMm = mPageWidthMm;
        mPrintHeightMm = mPageHeightMm;

        // If cut marks are being "always" shown, then reduce the printed
        // image size here to account for them.  For the other cut marks
        // options, the image for this scale will by definition fit on one page
        // and so they will not be shown.

        if (m_cutsOption==KookaPrint::CutMarksAlways)
        {
            mPrintWidthMm -= 2*CUT_MARGIN;
            mPrintHeightMm -= 2*CUT_MARGIN;
        }

        if (m_maintainAspect)				// maintain the aspect ratio
        {
            QRectF r = pageRect(QPrinter::DevicePixel);
            double wAspect = r.width()/mImageWidthPix;	// scaling ratio image -> page
            double hAspect = r.height()/mImageHeightPix;

            if (wAspect>hAspect)
            {
                // More scaling up is needed in the horizontal direction,
                // so reduce that to maintain the aspect ratio
                mPrintWidthMm *= hAspect/wAspect;
            }
            else if (hAspect>wAspect)
            {
                // More scaling up is needed in the vertical direction,
                // so reduce that to maintain the aspect ratio
                mPrintHeightMm *= wAspect/hAspect;
            }
        }
    }
    else Q_ASSERT(false);

    qDebug() << "scaled image size (mm) =" << QSizeF(mPrintWidthMm, mPrintHeightMm);

    mPrintResolution = DPI_TO_DPM(resolution())/1000;	// dots per millimetre

    // Now that we have the image size to be printed,
    // see if cut marks are required.

    mPageWidthAdjustedMm = mPageWidthMm;
    mPageHeightAdjustedMm = mPageHeightMm;

    bool withCutMarks;
    if (m_cutsOption==KookaPrint::CutMarksMultiple) withCutMarks = !(mPrintWidthMm<=mPageWidthMm && mPrintHeightMm<=mPageHeightMm);
    else if (m_cutsOption==KookaPrint::CutMarksAlways) withCutMarks = true;
    else if (m_cutsOption==KookaPrint::CutMarksNone) withCutMarks = false;
    else Q_ASSERT(false);
    qDebug() << "for cuts" << m_cutsOption << "with marks?" << withCutMarks;

    // If cut marks are required, reduce the available page size
    // to allow for them.

    mPrintLeftPix = 0;					// page origin of print
    mPrintTopPix = 0;

    if (withCutMarks)
    {
        mPageWidthAdjustedMm -= 2*CUT_MARGIN;
        mPageHeightAdjustedMm -= 2*CUT_MARGIN;
        qDebug() << "adjusted page size (mm) =" << QSizeF(mPageWidthAdjustedMm, mPageHeightAdjustedMm);

        mPrintLeftPix = mPrintTopPix = CUT_MARGIN*mPrintResolution;
    }

    bool onOnePage = (mPrintWidthMm<=mPageWidthAdjustedMm && mPrintHeightMm<=mPageHeightAdjustedMm);
    qDebug() << "on one page?" << onOnePage;		// see if fits on one page
							// must be true for this
    if (m_scaleOption==KookaPrint::ScaleFitPage) Q_ASSERT(onOnePage);

    // If the image fits on one page, then adjust the print margins so
    // that it is centered.  I'm not sure whether this is the right thing
    // to do, but it is implied by tool tips set in ImgPrintDialog.
    // TODO: maybe make it an option

    if (onOnePage)
    {
        int widthSpareMm = mPageWidthAdjustedMm-mPrintWidthMm;
        mPrintLeftPix += (widthSpareMm/2)*mPrintResolution;
        int heightSpareMm = mPageHeightAdjustedMm-mPrintHeightMm;
        mPrintTopPix += (heightSpareMm/2)*mPrintResolution;
    }

    // Calculate how many parts (including partial parts)
    // the image needs to be sliced up into.

    double ipart;
    double fpart = modf(mPrintWidthMm/mPageWidthAdjustedMm, &ipart);
    //qDebug() << "for cols ipart" << ipart << "fpart" << fpart;
    mPrintColumns = qRound(ipart)+(fpart>0 ? 1 : 0);
    fpart = modf(mPrintHeightMm/mPageHeightAdjustedMm, &ipart);
    //qDebug() << "for rows ipart" << ipart << "fpart" << fpart;
    mPrintRows = qRound(ipart)+(fpart>0 ? 1 : 0);

    int totalPages = mPrintColumns*mPrintRows;
    qDebug() << "print cols" << mPrintColumns << "rows" << mPrintRows << "pages" << totalPages;
    Q_ASSERT(totalPages>0);				// checks for sanity
    if (onOnePage) Q_ASSERT(totalPages==1);

    qDebug() << "done";
}









void KookaPrint::printImage()
{
    if (m_image==NULL) return;				// no image to print
    qDebug() << "starting";
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);	// this may take some time

#if 1
    // TODO: does this work?
    if (m_lowResDraft) setResolution(75);
#endif

    // Create the painter after all the print parameters have been set.
    m_painter = new QPainter(this);			// make accessible throughout class
    m_painter->setRenderHint(QPainter::SmoothPixmapTransform);

    // The image is to be printed as 'mPrintColumns' columns in 'mPrintRows'
    // rows. Each whole slice is 'sliceWidthPix' pixels wide and 'sliceHeightPix'
    // pixels high;  the last slice in each row and column may be smaller.

    int sliceWidthPix = qRound(mImageWidthPix*mPageWidthAdjustedMm/mPrintWidthMm);
    int sliceHeightPix = qRound(mImageHeightPix*mPageHeightAdjustedMm/mPrintHeightMm);
    qDebug() << "slice size =" << QSize(sliceWidthPix, sliceHeightPix);

    // Print columns and rows in this order, so that in the usual printer
    // and alignment case the sheets line up corresponding with the columns.

    const int totalPages = mPrintColumns*mPrintRows;
    int page = 1;
    for (int col = 0; col<mPrintColumns; ++col)
    {
        for (int row = 0; row<mPrintRows; ++row)
        {
            qDebug() << "print page" << page << "col" << col << "row" << row;

            // The slice starts at 'sliceLeftPix' and 'sliceTopPix' in the image.
            int sliceLeftPix = col*sliceWidthPix;
            int sliceTopPix = row*sliceHeightPix;

            // Calculate the source rectangle within the image
            int thisWidthPix = qMin(sliceWidthPix, (mImageWidthPix-sliceLeftPix));
            int thisHeightPix = qMin(sliceHeightPix, (mImageHeightPix-sliceTopPix));
            const QRect sourceRect(sliceLeftPix, sliceTopPix, thisWidthPix, thisHeightPix);

            // Calculate the target rectangle on the painter
            int targetWidthPix = qRound((mPageWidthAdjustedMm*mPrintResolution)*(double(thisWidthPix)/sliceWidthPix));
            int targetHeightPix = qRound((mPageHeightAdjustedMm*mPrintResolution)*(double(thisHeightPix)/sliceHeightPix));
            const QRect targetRect(mPrintLeftPix, mPrintTopPix, targetWidthPix, targetHeightPix);

            qDebug() << " " << sourceRect << "->" << targetRect;
            m_painter->drawImage(targetRect, *m_image, sourceRect);

//                 // TODO: should be drawn first so as not to go over image!
//                 drawCornerMarker(imgSize, row, col, maxRows, maxCols);

            ++page;
            if (page<=totalPages) newPage();		// not for the last page
        }
    }

    delete m_painter;					// finished with painting
    QGuiApplication::restoreOverrideCursor();
    qDebug() << "done";
}


void KookaPrint::drawMarkerAroundPoint(const QPoint &p)
{
    if (! m_painter) {
        return;
    }
    const int len = 10;

    m_painter->drawLine(p - QPoint(len, 0), p + QPoint(len, 0));
    m_painter->drawLine(p - QPoint(0, len), p + QPoint(0, len));

}

void KookaPrint::drawCutSign(const QPoint &p, int num, MarkerDirection dir)
{
    QBrush saveB = m_painter->brush();
    int start = 0;
    const int radius = 20;

    QColor brushColor(Qt::red);
    int toffX = 0;
    int toffY = 0;
    QString numStr = QString::number(num);

    QFontMetrics fm = m_painter->fontMetrics();
    int textWidth = fm.width(numStr) / 2;
    int textHeight = fm.width(numStr) / 2;
    int textYOff = 0;
    int textXOff = 0;
    switch (dir) {
    case SW:
        start = -90;
        brushColor = Qt::green;
        toffX = -1;
        toffY = 1;
        textXOff = -1 * textWidth;
        textYOff = textHeight;
        break;
    case NW:
        start = -180;
        brushColor = Qt::blue;
        toffX = -1;
        toffY = -1;
        textXOff = -1 * textWidth;
        textYOff = textHeight;
        break;
    case NO:
        start = -270;
        brushColor = Qt::yellow;
        toffX = 1;
        toffY = -1;
        textXOff = -1 * textWidth;
        textYOff = textHeight;

        break;
    case SO:
        start = 0;
        brushColor = Qt::magenta;
        toffX = 1;
        toffY = 1;
        textXOff = -1 * textWidth;
        textYOff = textHeight;
        break;
    default:
        start = 0;
    }

    /* to draw around the point p, subtraction of the half radius is needed */
    int x = p.x() - radius / 2;
    int y = p.y() - radius / 2;

    // m_painter->drawRect( x, y, radius, radius );  /* debug !!! */
    const int tAway = radius * 3 / 4;

    QRect bRect = fm.boundingRect(QString::number(num));
    int textX = p.x() + tAway * toffX + textXOff;
    int textY = p.y() + tAway * toffY + textYOff;

    // m_painter->drawRect( textX, textY, bRect.width(), bRect.height() );
    //qDebug() << "Drawing to position [" << textX << "," << textY << "]";
    m_painter->drawText(textX,
                        textY,
                        QString::number(num));
    QBrush b(brushColor, Qt::NoBrush /* remove this to get debug color*/);

    m_painter->setBrush(b);
    m_painter->drawPie(x, y, radius, radius, 16 * start, -16 * 90);

    m_painter->setBrush(saveB);
}

/*
 * draws the circle and the numbers that indicate the pages to glue to the side
 */
void KookaPrint::drawCornerMarker(const QSize &imgSize, int row, int col, int maxRows, int maxCols)
{
    QPoint p;

    //qDebug() << "Marker: Row" << row << "col" << col << "from max" << maxRows << "x" << maxCols;

    // Top left.
    p = printPosTopLeft(imgSize);
    drawMarkerAroundPoint(p);
    int indx = maxCols * row + col + 1;
    if (maxRows > 1 || maxCols > 1) {
        if (col > 0) {
            drawCutSign(p, indx - 1, SW);
        }
        if (row > 0) {
            drawCutSign(p, indx - maxCols, NO);
        }

        if (row > 0 && col > 0) {
            drawCutSign(p, indx - maxCols - 1, NW);
        }
    }

    // Top Right
    p = printPosTopRight(imgSize);
    drawMarkerAroundPoint(p);
    if (maxRows > 1 || maxCols > 1) {
        if (col < maxCols - 1) {
            drawCutSign(p, indx + 1, SO);
        }
        if (row > 0) {
            drawCutSign(p, indx - maxCols, NW);
        }
        if (row > 0 && col < maxCols - 1) {
            drawCutSign(p, indx - maxCols + 1, NO);
        }
    }

    // Bottom Right
    p = printPosBottomRight(imgSize);
    if (maxRows > 1 || maxCols > 1) {
        if (col < maxCols - 1) {
            drawCutSign(p, indx + 1, NO);
        }
        if (row < maxRows - 1) {
            drawCutSign(p, indx + maxCols, SW);
        }
        if (row < maxRows - 1 && col < maxCols - 1) {
            drawCutSign(p, indx + maxCols, SO);
        }
    }

    // p += QPoint( 1, 1 );
    drawMarkerAroundPoint(p);   /* at bottom right */

    /* Bottom left */
    p = printPosBottomLeft(imgSize);
    // p += QPoint( -1, 1 );
    if (maxRows > 1 || maxCols > 1) {
        if (col > 0) {
            drawCutSign(p, indx - 1, NW);
        }
        if (row < maxRows - 1) {
            drawCutSign(p, indx + maxCols, SO);
        }
        if (row < maxRows - 1 && col > 0) {
            drawCutSign(p, indx + maxCols - 1, SW);
        }
    }
    drawMarkerAroundPoint(p);   /* at bottom left */
}


int KookaPrint::extraMarginPix() const
{
    /* take the half extra margin */
    return int(double(m_maxPageSize.width()) * double(m_extraMarginPercent) / 100.0 / 2.0);
}

QPoint KookaPrint::printPosTopLeft(const QSize &imgSize) const
{
    /* take the half extra margin */
    int eMargin = extraMarginPix();

    return QPoint(eMargin + (m_maxPageSize.width()  - imgSize.width()) / 2,
                  eMargin + (m_maxPageSize.height() - imgSize.height()) / 2);
}

QPoint KookaPrint::printPosTopRight(const QSize &imgSize) const
{
    /* take the half extra margin */
    int eMargin = extraMarginPix();

    return QPoint(eMargin + (m_maxPageSize.width()  - imgSize.width()) / 2 + imgSize.width(),
                  eMargin + (m_maxPageSize.height() - imgSize.height()) / 2);
}

QPoint KookaPrint::printPosBottomLeft(const QSize &imgSize) const
{
    int eMargin = extraMarginPix();
    /* take the half extra margin */
    return QPoint(eMargin + (m_maxPageSize.width()  - imgSize.width()) / 2,
                  eMargin + (m_maxPageSize.height() - imgSize.height()) / 2 + imgSize.height());
}

QPoint KookaPrint::printPosBottomRight(const QSize &imgSize) const
{
    /* take the half extra margin */
    int eMargin = extraMarginPix();

    return QPoint(eMargin + (m_maxPageSize.width()  - imgSize.width()) / 2 + imgSize.width(),
                  eMargin + (m_maxPageSize.height() - imgSize.height()) / 2 + imgSize.height());
}
