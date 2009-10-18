/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>  

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

#include "dispgamma.h"
#include "dispgamma.moc"

#include <qpainter.h>
#include <qpixmap.h>
#include <qevent.h>


DispGamma::DispGamma( QWidget *parent )
    : QWidget( parent )
{
    vals = NULL;
    margin = 10;
}

DispGamma::~DispGamma()
{
}

void DispGamma::resizeEvent (QResizeEvent*ev )
{
    repaint();
}

void DispGamma::paintEvent( QPaintEvent *ev )
{
    QPainter p(this);
    int w = vals->size()+1;

    // Viewport auf margin setzen.
    p.setViewport( margin, margin, width() - margin, height() - margin );
    p.setWindow( 0, 255, w, -256 );

    p.setClipRect( ev->rect());

    p.setPen(palette().highlight().color());
    p.setBrush(palette().base());
    // Background
    p.drawRect( 0,0, w, 256 );

    p.setPen(QPen(palette().midlight(), 1, Qt::DotLine));
    // horizontal Grid
    for( int l = 1; l < 5; l++ )
            p.drawLine( 1, l*51, 255, l*51 );

    // vertical Grid
    for( int l = 1; l < 5; l++ )
            p.drawLine( l*51, 2, l*51, 255 );

    // draw gamma-Line
    p.setPen(palette().highlight().color());

    int py = vals->at(1);
    for( int x = 2; x<w-1; x++)
    {
        int y = vals->at(x);
        p.drawLine(x-1,py,x,y);
        py = y;
    }
}


QSize DispGamma::sizeHint() const
{
    return QSize( 256 + 2*margin,256 + 2 * margin );
}

QSizePolicy DispGamma::sizePolicy()
{
    return QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
}
