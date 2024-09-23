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

#include <klocalizedstring.h>

#include "scanimage.h"

#include "imgprintdialog.h"
#include "kookasettings.h"
#include "kscandevice.h"
#include "kooka_logging.h"


#define CUT_MARGIN		5			// margin in millimetres

#define PRINT_ORDER_COLUMNS
#define CUTMARKS_COLOURSEGS


KookaPrint::KookaPrint()
    : QPrinter(QPrinter::HighResolution)
{
    qCDebug(KOOKA_LOG);
    m_image = nullptr;

    // Initial default print parameters
    m_scaleOption = static_cast<KookaPrint::ScaleOption>(KookaSettings::printScaleOption());
    m_printSize = KookaSettings::printPrintSize();
    m_maintainAspect = KookaSettings::printMaintainAspect();
    m_lowResDraft = KookaSettings::printLowResDraft();
    m_cutsOption = static_cast<KookaPrint::CutMarksOption>(KookaSettings::printCutsOption());

    setOutputFileName(KookaSettings::printFileName());

    m_screenResolution = -1;				// set by caller
    m_scanResolution = -1;				// taken from image
    m_copyMode = false;					// normal print mode
}


void KookaPrint::recalculatePrintParameters()
{
    if (m_image==nullptr) return;				// no image to print

    qCDebug(KOOKA_LOG) << "image:";
    qCDebug(KOOKA_LOG) << "  size (pix) =" << m_image->size();
    qCDebug(KOOKA_LOG) << "  dpi X =" << DPM_TO_DPI(m_image->dotsPerMeterX());
    qCDebug(KOOKA_LOG) << "  dpi Y =" << DPM_TO_DPI(m_image->dotsPerMeterY());
    qCDebug(KOOKA_LOG) << "printer:";
    qCDebug(KOOKA_LOG) << "  name =" << printerName();
    qCDebug(KOOKA_LOG) << "  colour mode =" << colorMode();
    qCDebug(KOOKA_LOG) << "  full page?" << fullPage();
    qCDebug(KOOKA_LOG) << "  output format =" << outputFormat();
    qCDebug(KOOKA_LOG) << "  paper rect (mm) =" << paperRect(QPrinter::Millimeter);
    qCDebug(KOOKA_LOG) << "  page rect (mm) =" << pageRect(QPrinter::Millimeter);
    qCDebug(KOOKA_LOG) << "  resolution =" << resolution();
    qCDebug(KOOKA_LOG) << "options:";
    qCDebug(KOOKA_LOG) << "  scale mode =" << m_scaleOption;
    qCDebug(KOOKA_LOG) << "  print size (mm) =" << m_printSize;
    qCDebug(KOOKA_LOG) << "  scan resolution =" << m_scanResolution;
    qCDebug(KOOKA_LOG) << "  screen resolution =" << m_screenResolution;
    qCDebug(KOOKA_LOG) << "  cuts option =" << m_cutsOption;
    qCDebug(KOOKA_LOG) << "  maintain aspect?" << m_maintainAspect;
    qCDebug(KOOKA_LOG) << "  low res draft?" << m_lowResDraft;

    // Calculate the available page size, in real world units
    QRectF r = pageRect(QPrinter::Millimeter);
    mPageWidthMm = r.width();
    mPageHeightMm = r.height();

    // Calculate the size at which the image is to be printed,
    // depending on the scaling option.

    mImageWidthPix = m_image->width();			// image size in pixels
    mImageHeightPix = m_image->height();

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
            QRectF pr = pageRect(QPrinter::DevicePixel);
            double wAspect = pr.width()/mImageWidthPix;	// scaling ratio image -> page
            double hAspect = pr.height()/mImageHeightPix;

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

    qCDebug(KOOKA_LOG) << "scaled image size (mm) =" << QSizeF(mPrintWidthMm, mPrintHeightMm);

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
    qCDebug(KOOKA_LOG) << "for cuts" << m_cutsOption << "with marks?" << withCutMarks;

    // If cut marks are required, reduce the available page size
    // to allow for them.

    mPrintLeftPix = 0;					// page origin of print
    mPrintTopPix = 0;

    if (withCutMarks)
    {
        mPageWidthAdjustedMm -= 2*CUT_MARGIN;
        mPageHeightAdjustedMm -= 2*CUT_MARGIN;
        qCDebug(KOOKA_LOG) << "adjusted page size (mm) =" << QSizeF(mPageWidthAdjustedMm, mPageHeightAdjustedMm);

        mPrintLeftPix = mPrintTopPix = CUT_MARGIN*mPrintResolution;
    }

    bool onOnePage = (mPrintWidthMm<=mPageWidthAdjustedMm && mPrintHeightMm<=mPageHeightAdjustedMm);
    qCDebug(KOOKA_LOG) << "on one page?" << onOnePage;	// see if fits on one page
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
    //qCDebug(KOOKA_LOG) << "for cols ipart" << ipart << "fpart" << fpart;
    mPrintColumns = qRound(ipart)+(fpart>0 ? 1 : 0);
    fpart = modf(mPrintHeightMm/mPageHeightAdjustedMm, &ipart);
    //qCDebug(KOOKA_LOG) << "for rows ipart" << ipart << "fpart" << fpart;
    mPrintRows = qRound(ipart)+(fpart>0 ? 1 : 0);

    int totalPages = mPrintColumns*mPrintRows;
    qCDebug(KOOKA_LOG) << "print cols" << mPrintColumns << "rows" << mPrintRows << "pages" << totalPages;
    Q_ASSERT(totalPages>0);				// checks for sanity
    if (onOnePage) Q_ASSERT(totalPages==1);

    qCDebug(KOOKA_LOG) << "done";
}


void KookaPrint::printImage()
{
    if (m_image==nullptr) return;			// no image to print
    qCDebug(KOOKA_LOG) << "starting";
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);	// this may take some time

    if (!m_copyMode)
    {
        // Save the print parameters used
        KookaSettings::setPrintScaleOption(m_scaleOption);
        KookaSettings::setPrintPrintSize(m_printSize);
        KookaSettings::setPrintMaintainAspect(m_maintainAspect);
        KookaSettings::setPrintLowResDraft(m_lowResDraft);
        KookaSettings::setPrintCutsOption(m_cutsOption);
        KookaSettings::setPrintFileName(outputFileName());
        KookaSettings::self()->save();
    }
    else qCDebug(KOOKA_LOG) << "copy mode, not saving print parameters";

#if 1
    // TODO: does this work?
    if (m_lowResDraft) setResolution(75);
#endif

    // Create the painter after all the print parameters have been set.
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // The image is to be printed as 'mPrintColumns' columns in 'mPrintRows'
    // rows. Each whole slice is 'sliceWidthPix' pixels wide and 'sliceHeightPix'
    // pixels high;  the last slice in each row and column may be smaller.

    int sliceWidthPix = qRound(mImageWidthPix*mPageWidthAdjustedMm/mPrintWidthMm);
    int sliceHeightPix = qRound(mImageHeightPix*mPageHeightAdjustedMm/mPrintHeightMm);
    qCDebug(KOOKA_LOG) << "slice size =" << QSize(sliceWidthPix, sliceHeightPix);

    // Print columns and rows in this order, so that in the usual printer
    // and alignment case the sheets line up corresponding with the columns.

    const int totalPages = mPrintColumns*mPrintRows;
    int page = 1;
#ifdef PRINT_ORDER_COLUMNS
    for (int col = 0; col<mPrintColumns; ++col)
#else
    for (int row = 0; row<mPrintRows; ++row)
#endif
    {
#ifdef PRINT_ORDER_COLUMNS
        for (int row = 0; row<mPrintRows; ++row)
#else
        for (int col = 0; col<mPrintColumns; ++col)
#endif
        {
            qCDebug(KOOKA_LOG) << "print page" << page << "col" << col << "row" << row;

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

            qCDebug(KOOKA_LOG) << " " << sourceRect << "->" << targetRect;
            // The markers are drawn first so that the image will overwrite them.
            drawCornerMarkers(&painter, targetRect, row, col, mPrintRows, mPrintColumns);
            // Then the image rectangle.
            painter.drawImage(targetRect, *m_image, sourceRect);

            ++page;
            if (page<=totalPages) newPage();		// not for the last page
        }
    }

    QGuiApplication::restoreOverrideCursor();
    qCDebug(KOOKA_LOG) << "done";
}


// Draw a cross marker centered on that corner of the printed image.
void KookaPrint::drawMarkerAroundPoint(QPainter *painter, const QPoint &p)
{
    const int len = (CUT_MARGIN-1)*mPrintResolution;	// length in device pixels

    painter->save();
    painter->setPen(QPen(QBrush(Qt::black), 0));	// cosmetic pen width
    painter->drawLine(p-QPoint(len, 0), p+QPoint(len, 0));
    painter->drawLine(p-QPoint(0, len), p+QPoint(0, len));
    painter->restore();
}


void KookaPrint::drawCutSign(QPainter *painter, const QPoint &p, int num, Qt::Corner dir)
{
    painter->save();
    int start = 0;
    const int radius = (CUT_MARGIN-2)*mPrintResolution;	// offset in device pixels

    QColor brushColor(Qt::red);
    int toffX = 0;
    int toffY = 0;
    QString numStr = QString::number(num);

    QFontMetrics fm = painter->fontMetrics();
    int textWidth = fm.horizontalAdvance(numStr)/2;
    int textHeight = fm.height()/2;
    int textYOff = 0;
    int textXOff = 0;
    switch (dir) {
    case Qt::BottomLeftCorner:
        start = -90;
        brushColor = Qt::green;
        toffX = -1;
        toffY = 1;
        textXOff = -1 * textWidth;
        textYOff = textHeight;
        break;
    case Qt::TopLeftCorner:
        start = -180;
        brushColor = Qt::blue;
        toffX = -1;
        toffY = -1;
        textXOff = -1 * textWidth;
        textYOff = textHeight;
        break;
    case Qt::TopRightCorner:
        start = -270;
        brushColor = Qt::yellow;
        toffX = 1;
        toffY = -1;
        textXOff = -1 * textWidth;
        textYOff = textHeight;
        break;
    case Qt::BottomRightCorner:
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

    // painter->drawRect( x, y, radius, radius );  /* debug !!! */
    const int tAway = radius * 3 / 4;

    int textX = p.x() + tAway * toffX + textXOff;
    int textY = p.y() + tAway * toffY + textYOff;

    //painter->drawRect( textX, textY, bRect.width(), bRect.height() );
    painter->drawText(textX, textY, numStr);

#ifdef CUTMARKS_COLOURSEGS
    // TODO: this is wrong, the colour should be derived from the adjacent
    // page number so that the print is assembled by placing corresponding
    // numbers and colours together.
    QBrush b(brushColor);				// draw a colour segment
    painter->setBrush(b);				// to show correct orientation
    painter->drawPie(x, y, radius, radius, start*16, -90*16);
#endif

    painter->restore();
}


// Draw the circle and the numbers that indicate the adjacent pages
void KookaPrint::drawCornerMarkers(QPainter *painter, const QRect &targetRect,
                                   int row, int col, int maxRows, int maxCols)
{
    const bool multiPages = (maxRows>1 || maxCols>1);

    const bool firstColumn = (col==0);
    const bool lastColumn = (col==(maxCols-1));
    const bool firstRow = (row==0);
    const bool lastRow = (row==(maxRows-1));

#ifdef PRINT_ORDER_COLUMNS
    const int indx = maxRows*col + row + 1;
#else
    const int indx = maxCols*row + col + 1;
#endif
    //qCDebug(KOOKA_LOG) << "for col" << col << "/" << maxCols << "row" << row << "/" << maxRows << "-> index" << indx;

    // All the measurements here are derived from 'targetRect',
    // which is in device pixels.

    // Page index, if required
    if (multiPages)
    {
        const QString numStr = QString("= %1 =").arg(indx);
        const QFontMetrics &fm = painter->fontMetrics();
        const int xoff = targetRect.left()+((targetRect.width()-fm.horizontalAdvance(numStr))/2);

        painter->setPen(Qt::black);
        painter->drawText(xoff, fm.height()-1, numStr);
    }

    // Top left
    QPoint p = targetRect.topLeft();
    drawMarkerAroundPoint(painter, p);
    if (multiPages)
    {
#ifdef PRINT_ORDER_COLUMNS
        if (!firstColumn) drawCutSign(painter, p, indx - maxRows, Qt::BottomLeftCorner);
        if (!firstRow) drawCutSign(painter, p, indx - 1, Qt::TopRightCorner);
        if (!firstColumn && !firstRow) drawCutSign(painter, p, indx - maxRows - 1, Qt::TopLeftCorner);
#else
        if (!firstColumn) drawCutSign(painter, p, indx - 1, Qt::BottomLeftCorner);
        if (!firstRow) drawCutSign(painter, p, indx - maxCols, Qt::TopRightCorner);
        if (!firstColumn && !firstRow) drawCutSign(painter, p, indx - maxCols - 1, Qt::TopLeftCorner);
#endif
    }

    // Top right
    p = targetRect.topRight();
    drawMarkerAroundPoint(painter, p);
    if (multiPages)
    {
#ifdef PRINT_ORDER_COLUMNS
        if (!lastColumn) drawCutSign(painter, p, indx + maxRows, Qt::BottomRightCorner);
        if (!firstRow) drawCutSign(painter, p, indx - 1, Qt::TopLeftCorner);
        if (!lastColumn && !firstRow) drawCutSign(painter, p, indx + maxRows - 1, Qt::TopRightCorner);
#else
        if (!lastColumn) drawCutSign(painter, p, indx + 1, Qt::BottomRightCorner);
        if (!firstRow) drawCutSign(painter, p, indx - maxCols, Qt::TopLeftCorner);
        if (!lastColumn && !firstRow) drawCutSign(painter, p, indx - maxCols + 1, Qt::TopRightCorner);
#endif
    }

    // Bottom right
    p = targetRect.bottomRight();
    drawMarkerAroundPoint(painter, p);
    if (multiPages)
    {
#ifdef PRINT_ORDER_COLUMNS
        if (!lastColumn) drawCutSign(painter, p, indx + maxRows, Qt::TopRightCorner);
        if (!lastRow) drawCutSign(painter, p, indx + 1, Qt::BottomLeftCorner);
        if (!lastColumn && !lastRow) drawCutSign(painter, p, indx + maxRows + 1, Qt::BottomRightCorner);
#else
        if (!lastColumn) drawCutSign(painter, p, indx + 1, Qt::TopRightCorner);
        if (!lastRow) drawCutSign(painter, p, indx + maxCols, Qt::BottomLeftCorner);
        if (!lastColumn && !lastRow) drawCutSign(painter, p, indx + maxCols + 1, Qt::BottomRightCorner);
#endif
    }

    // Bottom left
    p = targetRect.bottomLeft();
    drawMarkerAroundPoint(painter, p);
    if (multiPages)
    {
#ifdef PRINT_ORDER_COLUMNS
        if (!firstColumn) drawCutSign(painter, p, indx - maxRows, Qt::TopLeftCorner);
        if (!lastRow) drawCutSign(painter, p, indx + 1, Qt::BottomRightCorner);
        if (!firstColumn && !lastRow) drawCutSign(painter, p, indx - maxRows + 1, Qt::BottomLeftCorner);
#else
        if (!firstColumn) drawCutSign(painter, p, indx - 1, Qt::TopLeftCorner);
        if (!lastRow) drawCutSign(painter, p, indx + maxCols, Qt::BottomRightCorner);
        if (!firstColumn && !lastRow) drawCutSign(painter, p, indx + maxCols - 1, Qt::BottomLeftCorner);
#endif
    }
}


void KookaPrint::setCopyMode(bool on)
{
    qCDebug(KOOKA_LOG) << on;
    m_copyMode = on;
}
