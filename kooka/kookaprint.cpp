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

#include "imgprintdialog.h"
#include <kdebug.h>

KookaPrint::KookaPrint( KPrinter *printer )
    :QObject(),
     m_printer(printer)
{

}

bool KookaPrint::printImage( KookaImage *img )
{
    bool result = true;
    if( ! m_printer || !img) return false;

    QString psMode = m_printer->option( OPT_PSGEN_DRAFT );
    kdDebug(28000) << "User setting for resolution: " << psMode << endl;

    if( psMode == "1" )
	m_printer->setResolution( 75 );
    else
	m_printer->setResolution( 600 );

    /* Create painter _after_ setting Resolution */
    QPainter painter(m_printer);
    QPainter *p = &painter;
    QImage   tmpImg = *img;
    QPoint   pt(0, 0);               // the top-left corner (image will be centered)

    // We use a QPaintDeviceMetrics to know the actual page size in pixel,
    // this gives the real painting area
    QPaintDeviceMetrics printermetrics( p->device() );

    int screenRes  = m_printer->option( OPT_SCREEN_RES ).toInt();
    int printerRes = printermetrics.logicalDpiX();

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

    /* Scale the image for printing */
    kdDebug(28000) << "Printer-Resolution: " << printerRes << " and scale-Reso: " << reso << endl;
    if( reso > 0)
    {
	double sizeInch = double(img->width()) / double(reso);
	int newWidth = int(sizeInch * printerRes);

	printerRes = printermetrics.logicalDpiY();
	sizeInch = double(img->height()) / double(reso);
	int newHeight = int(sizeInch * printerRes );

	kdDebug(28000) << "Scaling to printer size " << newWidth << " x " << newHeight << endl;

	tmpImg = img->smoothScale(newWidth, newHeight, QImage::ScaleFree);

	// center the image on the paint device
	QSize   sz = tmpImg.size();        // the current image size
	// TODO: check if too large
	pt.setX((printermetrics.width()-sz.width())/2);
	pt.setY((printermetrics.height()-sz.height())/2);

	// draw the image
	p->drawImage(pt, tmpImg);
    }
    return result;
}

#include "kookaprint.moc"
