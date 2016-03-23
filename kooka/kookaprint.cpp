/***************************************************************************
                kookaprint.cpp  -  Printing from the gallery
                             -------------------
    begin                : Tue May 13 2003
    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

#include "kookaprint.h"

#include <qpainter.h>
#include <qfontmetrics.h>
#include <qguiapplication.h>
#include <qdebug.h>

#include <klocalizedstring.h>

#include "imgprintdialog.h"
#include "kookaimage.h"


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
    qDebug() << "  size" << img->size();
    qDebug() << "  dpi X" << qRound(img->dotsPerMeterX()*(2.54/100));
    qDebug() << "  dpi Y" << qRound(img->dotsPerMeterY()*(2.54/100));
    qDebug() << "printer:";
    qDebug() << "  name" << printerName();
    qDebug() << "  colour mode" << colorMode();
    qDebug() << "  full page?" << fullPage();
    qDebug() << "  output format" << outputFormat();
    qDebug() << "  paper rect (mm)" << paperRect(QPrinter::Millimeter);
    qDebug() << "  page rect (mm)" << pageRect(QPrinter::Millimeter);
    qDebug() << "  resolution" << resolution();
    qDebug() << "options:";
    for (QMap<QString, QString>::const_iterator it = m_options->constBegin();
         it!=m_options->constEnd(); ++it)
    {
        qDebug() << " " << qPrintable(it.key()) << it.value();
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

    KookaImage tmpImg;
    QPoint pt(0, 0);               // the top-left corner (image will be centered)

    int screenRes  = m_options->value(OPT_SCREEN_RES).toInt();
    int printerRes = resolution();

    QString scale = m_options->value(OPT_SCALING);

    m_maxPageSize = maxPageSize();			// calculate total page size

    int reso = screenRes;
    if (scale == "scan") {
        /* Scale to original size */
        reso = m_options->value(OPT_SCAN_RES).toInt();
    } else if (scale == "custom") {
        // //qDebug() << "Not yet implemented: Custom scale";
        double userWidthInch = (m_options->value(OPT_WIDTH).toDouble() / 25.4);
        reso = int(double(img->width()) / userWidthInch);

        //qDebug() << "Custom resolution:" << reso;

        // TODO: for this option "Maintain aspect ratio" is enabled in GUI, but
        // not taken account of here.  No need, the scaling is taken account of
        // there.

    } else if (scale == "fitpage") {
        //qDebug() << "Printing using maximum space on page";
        printFittingToPage(img);
        reso = 0;  // to skip the printing on this page.
        // TODO: combine printFittingToPage() with below
    }

    /* Scale the image for printing */
    //qDebug() << "Printer-Resolution:" << printerRes << "scale-reso:" << reso;
//    QSize margins = margins();
    //qDebug() << "Printer-Margins left:" << margins.width() << "top" << margins.height();

    if (reso > 0) {
        double sizeInch = double(img->width()) / double(reso);
        int newWidth = int(sizeInch * printerRes);

        int printerResY = m_painter->device()->logicalDpiY();

        sizeInch = double(img->height()) / double(reso);
        int newHeight = int(sizeInch * printerResY);

        qDebug() << "Scaling to size [" << newWidth << "x" << newHeight << "]";

        // TODO: this is a memory intensive operation, can it be eliminated?
        tmpImg = img->scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QSize   sz = tmpImg.size();        // the current image size

        int maxRows, maxCols;
        int totalPages = tmpImg.cutToTiles(m_maxPageSize, maxRows, maxCols);

        qDebug() << "Subpages count:" << totalPages
                << "Columns:" << maxCols << "Rows:" << maxRows;

        int page = 1;
        for (int row = 0; row < maxRows; ++row)
        {
            for (int col = 0; col < maxCols; ++col)
            {
                const QRect part = tmpImg.getTileRect(row, col);
                const QSize imgSize = part.size();

                qDebug() << "Printing page" << page << "from source" << part;

                const QRect targetRect(printPosTopLeft(imgSize), imgSize);
                m_painter->drawImage(targetRect, tmpImg, part);

                drawCornerMarker(imgSize, row, col, maxRows, maxCols);

                ++page;
                if (page<=totalPages) newPage();	// not for the last page
            }
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
