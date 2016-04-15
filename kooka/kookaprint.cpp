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
    m_extraMarginPercent = 10;
}


void KookaPrint::setOptions(const QMap<QString, QString> *opts)
{
    m_options = opts;
}


bool KookaPrint::printImage(const KookaImage *img, int intextraMarginPercent)
{
    if (img==NULL) return false;
    bool result = true;

    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    qDebug() << "image:";
    qDebug() << "  size =" << img->size();
    qDebug() << "  dpi X =" << qRound(img->dotsPerMeterX()*(2.54/100));
    qDebug() << "  dpi Y =" << qRound(img->dotsPerMeterY()*(2.54/100));
    qDebug() << "printer:";
    qDebug() << "  name =" << printerName();
    qDebug() << "  colour mode =" << colorMode();
    qDebug() << "  full page?" << fullPage();
    qDebug() << "  output format =" << outputFormat();
    qDebug() << "  paper rect (mm) =" << paperRect(QPrinter::Millimeter);
    qDebug() << "  page rect (mm) =" << pageRect(QPrinter::Millimeter);
    qDebug() << "  resolution =" << resolution();
    qDebug() << "options:";
    for (QMap<QString, QString>::const_iterator it = m_options->constBegin();
         it!=m_options->constEnd(); ++it)
    {
        qDebug() << " " << qPrintable(it.key()) << "=" << it.value();
    }

    m_extraMarginPercent = intextraMarginPercent;

#if 1
    QString psMode = m_options->value(OPT_PSGEN_DRAFT);
// TODO: does this work?
    if (psMode=="1") setResolution(75);
#endif

    QPainter painter(this);				// create after setting resolution
    m_painter = &painter;				// make accessible throughout class
    m_painter->setRenderHint(QPainter::SmoothPixmapTransform);

    // Calculate the available page size, in real world units
    QRectF r = pageRect(QPrinter::Millimeter);
    double pageWidthMm = r.width();
    double pageHeightMm = r.height();

    // See whether cut marks are requested.
    QString cuts = m_options->value(OPT_CUTMARKS);
//    if (cuts.isEmpty()) cuts = "multiple";
    // TODO: debugging
    if (cuts.isEmpty()) cuts = "always";

    // Calculate the size at which the image is to be printed,
    // depending on the scaling option.

    const int imgWidthPix = img->width();		// image size in pixels
    const int imgHeightPix = img->height();

    double printWidthMm;				// print size of the image
    double printHeightMm;

    const QString scale = m_options->value(OPT_SCALING);
    if (scale=="scan")					// Original scan size
    {
        const QString opt = m_options->value(OPT_SCAN_RES);
        const int imageRes = !opt.isEmpty() ? DPI_TO_DPM(opt.toInt()) : img->dotsPerMeterX();
							// dots per metre
        printWidthMm = double(imgWidthPix)/imageRes*1000;
        printHeightMm = double(imgHeightPix)/imageRes*1000;
    }
    else if (scale=="screen")				// Scale to screen resolution
    {
        int screenRes  = m_options->value(OPT_SCREEN_RES).toInt();
        Q_ASSERT(screenRes>0);
        printWidthMm = double(imgWidthPix)/DPI_TO_DPM(screenRes)*1000;
        printHeightMm = double(imgHeightPix)/DPI_TO_DPM(screenRes)*1000;
    }
    else if (scale=="custom")				// Custom size
    {
        // For this option, "Maintain aspect ratio" can be enabled in the GUI.
        // There is however no need to take account of it here, because the
        // values are already scaled/adjusted there.

        printWidthMm = m_options->value(OPT_WIDTH).toDouble();
        printHeightMm = m_options->value(OPT_HEIGHT).toDouble();
        Q_ASSERT(printWidthMm>0 && printHeightMm>0);
    }
    else if (scale=="fitpage")				// Fit to one page
    {
        printWidthMm = pageWidthMm;
        printHeightMm = pageHeightMm;

        // If cut marks are being "always" shown, then reduce the printed
        // image size here to account for them.  For the other cut marks
        // options, the image for this scale will by definition fit on one page
        // and so they will not be shown.

        if (cuts=="always")
        {
            printWidthMm -= 2*CUT_MARGIN;
            printHeightMm -= 2*CUT_MARGIN;
        }

        if (m_options->value(OPT_RATIO)=="1")		// maintain the aspect ratio
        {
            QRectF r = pageRect(QPrinter::DevicePixel);
            double wAspect = r.width()/imgWidthPix;	// scaling ratio image -> page
            double hAspect = r.height()/imgHeightPix;

            if (wAspect>hAspect)
            {
                // More scaling up is needed in the horizontal direction,
                // so reduce that to maintain the aspect ratio
                printWidthMm *= hAspect/wAspect;
            }
            else if (hAspect>wAspect)
            {
                // More scaling up is needed in the vertical direction,
                // so reduce that to maintain the aspect ratio
                printHeightMm *= wAspect/hAspect;
            }
        }
    }
    else Q_ASSERT(false);

    qDebug() << "scaled image size (mm) =" << QSizeF(printWidthMm, printHeightMm);

    const int printRes = DPI_TO_DPM(resolution())/1000;	// dots per millimetre

    // Now that we have the image size to be printed,
    // see if cut marks are required.

    double pageWidthAdjustedMm = pageWidthMm;
    double pageHeightAdjustedMm = pageHeightMm;

    bool withCutMarks;
    if (cuts=="multiple") withCutMarks = !(printWidthMm<=pageWidthMm && printHeightMm<=pageHeightMm);
    else if (cuts=="always") withCutMarks = true;
    else if (cuts=="never") withCutMarks = false;
    else Q_ASSERT(false);
    qDebug() << "for cuts" << cuts << "with marks?" << withCutMarks;

    // If cut marks are required, reduce the available page size
    // to allow for them.

    int printLeftPix = 0;				// page origin of print
    int printTopPix = 0;

    if (withCutMarks)
    {
        pageWidthAdjustedMm -= 2*CUT_MARGIN;
        pageHeightAdjustedMm -= 2*CUT_MARGIN;
        qDebug() << "adjusted page size (mm) =" << QSizeF(pageWidthAdjustedMm, pageHeightAdjustedMm);

        printLeftPix = printTopPix = CUT_MARGIN*printRes;
    }

    bool onOnePage = (printWidthMm<=pageWidthAdjustedMm && printHeightMm<=pageHeightAdjustedMm);
    qDebug() << "on one page?" << onOnePage;		// see if fits on one page
    if (scale=="fitpage") Q_ASSERT(onOnePage);		// must be true for this

    // If the image fits on one page, then adjust the print margins so
    // that it is centered.  I'm not sure whether this is the right thing
    // to do, but it is implied by tool tips set in ImgPrintDialog.

    // TODO: maybe make it an option

    if (onOnePage)
    {
        int widthSpareMm = pageWidthAdjustedMm-printWidthMm;
        printLeftPix += (widthSpareMm/2)*printRes;
        int heightSpareMm = pageHeightAdjustedMm-printHeightMm;
        printTopPix += (heightSpareMm/2)*printRes;
    }

    // Calculate how many parts (including partial parts)
    // the image needs to be sliced up into.

    double ipart;
    double fpart = modf(printWidthMm/pageWidthAdjustedMm, &ipart);
    //qDebug() << "for cols ipart" << ipart << "fpart" << fpart;
    int cols = qRound(ipart)+(fpart>0 ? 1 : 0);
    fpart = modf(printHeightMm/pageHeightAdjustedMm, &ipart);
    //qDebug() << "for rows ipart" << ipart << "fpart" << fpart;
    int rows = qRound(ipart)+(fpart>0 ? 1 : 0);

    int totalPages = cols*rows;
    qDebug() << "print cols" << cols << "rows" << rows << "pages" << totalPages;
    Q_ASSERT(totalPages>0);				// checks for sanity
    if (onOnePage) Q_ASSERT(totalPages==1);

    // The image is to be printed as 'cols' columns in 'rows' rows.
    // Each whole slice is 'sliceWidthPix' pixels wide and 'sliceHeightPix'
    // pixels high;  the last slice in each row and column may be smaller.

    int sliceWidthPix = qRound(imgWidthPix*pageWidthAdjustedMm/printWidthMm);
    int sliceHeightPix = qRound(imgHeightPix*pageHeightAdjustedMm/printHeightMm);
    qDebug() << "slice size =" << QSize(sliceWidthPix, sliceHeightPix);

    // Print columns and rows in this order, so that in the usual printer
    // and alignment case the sheets line up corresponding with the columns.

    int page = 1;
    for (int col = 0; col<cols; ++col)
    {
        for (int row = 0; row<rows; ++row)
        {
            qDebug() << "print page" << page << "col" << col << "row" << row;

            // The slice starts at 'sliceLeftPix' and 'sliceTopPix' in the image.
            int sliceLeftPix = col*sliceWidthPix;
            int sliceTopPix = row*sliceHeightPix;

            // Calculate the source rectangle within the image
            int thisWidthPix = qMin(sliceWidthPix, (imgWidthPix-sliceLeftPix));
            int thisHeightPix = qMin(sliceHeightPix, (imgHeightPix-sliceTopPix));
            const QRect sourceRect(sliceLeftPix, sliceTopPix, thisWidthPix, thisHeightPix);

            // Calculate the target rectangle on the painter
            int targetWidthPix = qRound((pageWidthAdjustedMm*printRes)*(double(thisWidthPix)/sliceWidthPix));
            int targetHeightPix = qRound((pageHeightAdjustedMm*printRes)*(double(thisHeightPix)/sliceHeightPix));
            const QRect targetRect(printLeftPix, printTopPix, targetWidthPix, targetHeightPix);

            qDebug() << " " << sourceRect << "->" << targetRect;
            m_painter->drawImage(targetRect, *img, sourceRect);

//                 // TODO: should be drawn first so as not to go over image!
//                 drawCornerMarker(imgSize, row, col, maxRows, maxCols);

            ++page;
            if (page<=totalPages) newPage();		// not for the last page
        }
    }

    m_painter = NULL;					// no, this is not a memory leak
    QGuiApplication::restoreOverrideCursor();
    qDebug() << "done";
    return (result);
}


void KookaPrint::printFittingToPage(const KookaImage *img)
{
    if (img == NULL || m_painter == NULL) return;

    const bool maintainAspect = (m_options->value(OPT_RATIO)=="1");

    double wAspect = double(m_maxPageSize.width())  / double(img->width());
    double hAspect = double(m_maxPageSize.height()) / double(img->height());

    // take the smaller one.
    double aspect = wAspect;
    if (hAspect < wAspect) {
        aspect = hAspect;
    }

    // default: maintain aspect ratio.
    int newWidth  = int(double(img->width()) * aspect);
    int newHeight = int(double(img->height()) * aspect);

    if (! maintainAspect) {
        newWidth  = int(double(img->width())  * wAspect);
        newHeight = int(double(img->height()) * hAspect);
    }

    const QRect targetRect(0, 0, newWidth, newHeight);
    qDebug() << "source" << img->rect() << "into" << targetRect;
    m_painter->drawImage(targetRect, *img, img->rect());
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


QSize KookaPrint::maxPageSize(int extraShrinkPercent) const
{
    if (m_painter==NULL) return (QSize());

    QSize ret(m_painter->device()->width(), m_painter->device()->height());

    if (extraShrinkPercent>0)				// make smaller by percentage
    {
        const double extraShrink = double(100-extraShrinkPercent)/100.0;
        ret.setWidth(qRound(ret.width() * extraShrink));
        ret.setHeight(qRound(ret.height() * extraShrink));
    }

    return (ret);					// result in pixels
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
