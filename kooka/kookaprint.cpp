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
#include "kookaimage.h"
#include <kprinter.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qfontmetrics.h>
#include "imgprintdialog.h"
#include <kdebug.h>
#include <klocale.h>

KookaPrint::KookaPrint( KPrinter *printer )
    :QObject(),
     m_printer(printer),
     m_extraMarginPercent(10)
{

}

bool KookaPrint::printImage( KookaImage *img )
{
    bool result = true;
    if( ! m_printer || !img) return false;

    QString psMode = m_printer->option( OPT_PSGEN_DRAFT );
    kdDebug(28000) << "User setting for quality: " << psMode << endl;

#if 0
    if( psMode == "1" )
	m_printer->setResolution( 75 );
    else
	m_printer->setResolution( 600 );
#endif

    /* Create painter _after_ setting Resolution */
    QPainter painter(m_printer);
    m_painter = &painter;
    KookaImage   tmpImg;
    QPoint   pt(0, 0);               // the top-left corner (image will be centered)

    // We use a QPaintDeviceMetrics to know the actual page size in pixel,
    // this gives the real painting area
    QPaintDeviceMetrics printermetrics( m_painter->device() );

    int screenRes  = m_printer->option( OPT_SCREEN_RES ).toInt();
    // int printerRes = printermetrics.logicalDpiX();
    int printerRes = m_printer->resolution();

    QString scale = m_printer->option( OPT_SCALING );

    int reso = screenRes;

    if( scale == "scan" )
    {
	/* Scale to original size */
	reso = m_printer->option( OPT_SCAN_RES ).toInt();
    }
    else if( scale == "custom" )
    {
	// kdDebug(28000) << "Not yet implemented: Custom scale" << endl;
        double userWidthInch = (m_printer->option( OPT_WIDTH ).toDouble() / 25.4 );
	reso = int( double(img->width()) / userWidthInch );

        kdDebug(28000) << "Custom resolution: " << reso << endl;

    }
    else if( scale == "fitpage" )
    {
	kdDebug(28000) << "Printing using maximum space on page" << endl;
	printFittingToPage( img );
	reso = 0;  // to skip the printing on this page.
    }

    /* Scale the image for printing */
    kdDebug(28000) << "Printer-Resolution: " << printerRes << " and scale-Reso: " << reso << endl;
    QSize margins = m_printer->margins();
    kdDebug(28000) << "Printer-Margins left: " << margins.width() << " and top " << margins.height()
                   << endl;
    if( reso > 0)
    {
	double sizeInch = double(img->width()) / double(reso);
	int newWidth = int(sizeInch * printerRes);

	printerRes = printermetrics.logicalDpiY();
	sizeInch = double(img->height()) / double(reso);
	int newHeight = int(sizeInch * printerRes );

	kdDebug(28000) << "Scaling to printer size " << newWidth << " x " << newHeight << endl;

	tmpImg = img->smoothScale(newWidth, newHeight, QImage::ScaleFree);

        QSize   sz = tmpImg.size();        // the current image size
        QSize   maxOnPage = maxPageSize(); // the maximum space on one side

        int maxRows, maxCols;
        int subpagesCnt = tmpImg.cutToTiles( maxOnPage, maxRows, maxCols );

        kdDebug(28000) << "Subpages count: " << subpagesCnt <<
            " Columns:" << maxCols << " Rows:" << maxRows << endl;

        int cnt = 0;

        for( int row = 0; row < maxRows; row++ )
        {
            for( int col = 0; col < maxCols; col++ )
            {
                const QRect part = tmpImg.getTileRect( row, col );
                const QSize imgSize = part.size();

                kdDebug(28000) << "Printing part from " << part.x() << "/" << part.y()
                               << " width:"<< part.width() << " and height " << part.height() << endl;
                QImage tileImg = tmpImg.copy( part );

                m_painter->drawImage( printPosTopLeft(imgSize), tileImg );
                drawCornerMarker( imgSize, row, col, maxRows, maxCols );
                cnt++;
                if( cnt < subpagesCnt )
                    m_printer->newPage();
            }
        }
    }

    m_painter = 0;  // no, this is not a memory leak. 
    return result;
}

void KookaPrint::printFittingToPage(KookaImage *img)
{
    if( ! img || ! m_painter ) return;

    KookaImage   tmpImg;

    QString psMode = m_printer->option( OPT_RATIO );
    bool maintainAspect = (psMode == "1");

    QSize s = maxPageSize();

    double wAspect = double(s.width())  / double(img->width());
    double hAspect = double(s.height()) / double(img->height());

    // take the smaller one.
    double aspect = wAspect;
    if( hAspect < wAspect ) aspect = hAspect;

    // default: maintain aspect ratio.
    int newWidth  = int( double( img->width() ) * aspect );
    int newHeight = int( double( img->height()) * aspect );

    if( ! maintainAspect )
    {
	newWidth  = int( double( img->width() )  * wAspect );
	newHeight = int( double( img->height() ) * hAspect );
    }
    
    tmpImg = img->smoothScale(newWidth, newHeight, QImage::ScaleFree);

    m_painter->drawImage( 0,0, tmpImg );
    
}


void KookaPrint::drawMarkerAroundPoint( const QPoint& p )
{
    if( ! m_painter ) return;
    const int len = 10;
    
    m_painter->drawLine( p-QPoint(len,0), p+QPoint(len,0));
    m_painter->drawLine( p-QPoint(0,len), p+QPoint(0,len));

}


void KookaPrint::drawCutSign( const QPoint& p, int num, MarkerDirection dir )
{
    QBrush saveB = m_painter->brush();
    int start = 0;
    const int radius=20;

    QColor brushColor( Qt::red );
    int toffX=0;
    int toffY=0;
    QString numStr = QString::number(num);

    QFontMetrics fm = m_painter->fontMetrics();
    int textWidth = fm.width( numStr )/2;
    int textHeight = fm.width( numStr )/2;
    int textYOff = 0;
    int textXOff = 0;
    switch( dir )
    {
	case SW:
	    start = -90;
	    brushColor = Qt::green;
	    toffX =-1;
	    toffY = 1;
	    textXOff = -1*textWidth;
	    textYOff = textHeight;
	    break;
	case NW:
	    start = -180;
	    brushColor = Qt::blue;
	    toffX =-1;
	    toffY =-1;
	    textXOff = -1*textWidth;
	    textYOff = textHeight;
	    break;
	case NO:
	    start = -270;
	    brushColor = Qt::yellow;
	    toffX = 1;
	    toffY = -1;
	    textXOff = -1*textWidth;
	    textYOff = textHeight;

	    break;
	case SO:
	    start = 0;
	    brushColor = Qt::magenta;
	    toffX = 1;
	    toffY = 1;
	    textXOff = -1*textWidth;
	    textYOff = textHeight;
	    break;
	default:
	    start = 0;
    }

    /* to draw around the point p, subtraction of the half radius is needed */
    int x = p.x()-radius/2;
    int y = p.y()-radius/2;

    // m_painter->drawRect( x, y, radius, radius );  /* debug !!! */
    const int tAway = radius*3/4;   
    
    QRect bRect = fm.boundingRect( QString::number(num));
    int textX = p.x()+ tAway * toffX + textXOff;
    int textY = p.y()+ tAway * toffY + textYOff;
    
    // m_painter->drawRect( textX, textY, bRect.width(), bRect.height() );
    kdDebug(28000) << "Drawing to position " << textX << "/" << textY << endl;
    m_painter->drawText( textX,
			 textY,
			 QString::number(num));
    QBrush b( brushColor, NoBrush /* remove this to get debug color*/ );
    
    
    m_painter->setBrush( b );
    m_painter->drawPie( x, y, radius, radius, 16*start, -16*90 );

    m_painter->setBrush( saveB );
}


/*
 * draws the circle and the numbers that indicate the pages to glue to the side
 */
void KookaPrint::drawCornerMarker( const QSize& imgSize, int row, int col, int maxRows, int maxCols )
{
    QPoint p;

    kdDebug(28000) << "Marker: Row: " << row << " and col " << col <<" from max "
		   << maxRows << "x" << maxCols << endl;
    
    // Top left.
    p = printPosTopLeft( imgSize );
    drawMarkerAroundPoint( p );
    int indx = maxCols*row+col+1;
    if( maxRows > 1 || maxCols > 1 )
    {
	if( col > 0 )
	    drawCutSign( p, indx-1, SW );
	if( row > 0 )
	    drawCutSign( p, indx-maxCols, NO );

	if( row > 0 && col > 0 )
	    drawCutSign( p, indx-maxCols-1, NW );
    }
    
    // Top Right
    p = printPosTopRight( imgSize );
    drawMarkerAroundPoint( p );
    if( maxRows > 1 || maxCols > 1 )
    {
	if( col < maxCols-1 )
	    drawCutSign( p, indx+1, SO );
	if( row > 0 )
	    drawCutSign( p, indx-maxCols, NW );
	if( row > 0 && col < maxCols-1 )
	    drawCutSign( p, indx-maxCols+1, NO );
    }

    // Bottom Right
    p = printPosBottomRight( imgSize );
    if( maxRows > 1 || maxCols > 1 )
    {
	if( col < maxCols-1 )
	    drawCutSign( p, indx+1, NO );
	if( row < maxRows-1 )
	    drawCutSign( p, indx+maxCols, SW );
	if( row < maxRows -1 && col < maxCols-1 )
	    drawCutSign( p, indx+maxCols, SO );
    }

    // p += QPoint( 1, 1 );
    drawMarkerAroundPoint( p ); /* at bottom right */

    /* Bottom left */
    p = printPosBottomLeft( imgSize );
    // p += QPoint( -1, 1 );
    if( maxRows > 1 || maxCols > 1 )
    {
	if( col > 0 )
	    drawCutSign( p, indx-1, NW );
	if( row < maxRows-1 )
	    drawCutSign( p, indx+maxCols, SO );
	if( row < maxRows -1 && col > 0 )
	    drawCutSign( p, indx+maxCols-1, SW );
    }
    drawMarkerAroundPoint( p ); /* at bottom left */
}

QSize KookaPrint::maxPageSize( int extraShrinkPercent ) const
{
    if( ! m_painter ) return QSize();
    QPaintDeviceMetrics printermetrics( m_painter->device() );

    double extraShrink = double(100-extraShrinkPercent)/100.0;
    
    QSize retSize( printermetrics.width(), printermetrics.height() );
    
    if( extraShrinkPercent > 0 )
	retSize = QSize( int(double(printermetrics.width())* extraShrink) ,
			 int(double(printermetrics.height())* extraShrink ));
    return retSize;
}

int KookaPrint::extraMarginPix() const
{
    QSize max = maxPageSize();
    /* take the half extra margin */
    return int(double(max.width())*double(m_extraMarginPercent) / 100.0 / 2.0);
}

QPoint KookaPrint::printPosTopLeft( const QSize& imgSize ) const
{
    QSize max = maxPageSize();
    /* take the half extra margin */
    int eMargin = extraMarginPix();

    return QPoint( eMargin + (max.width()  - imgSize.width())/2,
                   eMargin + (max.height() - imgSize.height())/2 );
}

QPoint KookaPrint::printPosTopRight(const QSize& imgSize) const
{
    QSize max = maxPageSize();
    /* take the half extra margin */
    int eMargin = extraMarginPix();

    return QPoint( eMargin + (max.width()  - imgSize.width())/2+imgSize.width(),
                   eMargin + (max.height() - imgSize.height())/2 );
}

QPoint KookaPrint::printPosBottomLeft(const QSize& imgSize) const
{
    QSize max = maxPageSize();
    int eMargin = extraMarginPix();
    /* take the half extra margin */
    return QPoint( eMargin+(max.width()  - imgSize.width())/2,
                   eMargin+(max.height() - imgSize.height())/2 + imgSize.height() );
}

QPoint KookaPrint::printPosBottomRight(const QSize& imgSize) const
{
    QSize max = maxPageSize();
    /* take the half extra margin */
    int eMargin = extraMarginPix();

    return QPoint( eMargin+(max.width()  - imgSize.width())/2 + imgSize.width(),
                   eMargin+(max.height() - imgSize.height())/2 + imgSize.height() );
}



#include "kookaprint.moc"
