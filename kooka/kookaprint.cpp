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

KookaPrint::KookaPrint( KPrinter *printer )
    :QObject(),
     m_printer(printer)
{
    
}

bool KookaPrint::printImage( KookaImage *img )
{
    bool result = true;
    if( ! m_printer || !img) return false;

    QPainter        painter(m_printer);
    QPainter *p = &painter;
    QImage tmpImg = *img;
    QPoint  pt(0, 0);               // the top-left corner (image will be centered)
        
    // We use a QPaintDeviceMetrics to know the actual page size in pixel,
    // this gives the real painting area
    QPaintDeviceMetrics     metrics(p->device());

    tmpImg = img->smoothScale(metrics.width(), metrics.height(), QImage::ScaleMin);
#if 0
        // scale the image if needed
    switch (autofit)
    {
	...
	case 1:
	    // always fit to page
	    img = m_image.smoothScale(metrics.width(), metrics.height(), QImage::ScaleMin);
	    break;
	    ...
		}
#endif
    // center the image on the paint device
    QSize   sz = tmpImg.size();        // the current image size
    pt.setX((metrics.width()-sz.width())/2);
    pt.setY((metrics.height()-sz.height())/2);

    // draw the image
    p->drawImage(pt, tmpImg);

    return result;
}
