/***************************************************************************
                          ksaneocr.cpp  -  description                              
                             -------------------                                         
    begin                : Fri Jun 30 2000                                           
    copyright            : (C) 2000 by Klaas Freitag                         
    email                : Klaas.Freitag@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/
#include <qcolor.h>
#include "config.h"

#ifdef HAVE_LIBPGM2ASC
#include "gocr/gocr.h"
#include "gocr/pnm.h"

#include "gocr/pgm2asc.h"
#endif /* libPgm2asc */

#include "ksaneocr.h"


KSANEOCR::KSANEOCR( const QImage *img)
{
    if( !img ) return;

#ifdef HAVE_LIBPGM2ASC
    int cs=0,spc=0,mo=0,dust_size=10;
    int verbose=0;

    QImage bw_img;

    if( img->depth()==32 )
        bw_img = img->convertDepth( 8, MonoOnly );
    else if (img->depth() == 1)
        bw_img = img->convertDepth( 8, MonoOnly );
    else
        bw_img = *img;

    /* Palette stretch - no idea if this is ok */
    QArray<uchar> imgBuf(bw_img.height()*bw_img.width());
    int x, y, count;
    count = 0;

    for( y = 0; y < bw_img.height(); y++ )
    {
        for( x = 0; x < bw_img.width(); x++)
        {
            if( bw_img.numColors() == 2 )
            {

                if( bw_img.pixelIndex(x,y) == 0 )
                    imgBuf[count] = 255;
                else
                    imgBuf[count] = 0;
            }
            else
            {
                imgBuf[count] = bw_img.pixelIndex(x,y);
            }
            count++;
        }
    }

    if( ! bw_img.isNull() )
    {
        debug( "Conversion worked");


        pix pixmap;

        pixmap.x = bw_img.width();
        pixmap.y = bw_img.height();
        pixmap.bpp = 1;
        pixmap.p = imgBuf.data();

        pgm2asc(&pixmap, mo, cs, spc, dust_size, "_", verbose);

        int linecounter = 0;
        const char *line = getTextLine( linecounter++ );
        while( line )
        {
            debug( "%s", line );
            line = getTextLine( linecounter++ );
        }
        free_textlines();
    }
#endif
}

KSANEOCR::~KSANEOCR()
{
}
