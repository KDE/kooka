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

#include "sizeindicator.h"
#include "sizeindicator.moc"

#include <qpainter.h>
#include <qevent.h>
#include <QLinearGradient>

#include <klocale.h>
#include <kdebug.h>
#include <kglobal.h>




SizeIndicator::SizeIndicator(QWidget *parent, long thres, long crit)
    : QLabel(parent)
{
    setFrameStyle( QFrame::Box | QFrame::Sunken );
    setMinimumWidth(fontMetrics().width( QString::fromLatin1("MMMM.MM MiB")));

   threshold = thres;
   sizeInByte = -1;
   setCritical( crit );
}


void SizeIndicator::setCritical( long crit )
{
   critical = crit;
   devider = 255.0 / double( critical );
}


void SizeIndicator::setThreshold( long thres )
{
   threshold = thres;
}


SizeIndicator::~SizeIndicator()
{
}


void SizeIndicator::setSizeInByte(long newSize)
{
    kDebug() << "New size" << newSize << "bytes";

    sizeInByte = newSize;

    if (newSize<0) setText("");
    else setText(KGlobal::locale()->formatByteSize(newSize));
    // TODO: do we want JEDEC units here?
}


// TODO: maybe use some more colours (green below, orange between
// thresh and crit, red above crit, adjust gradient to indicate how
// close to those thresholds)
//
// TODO: hardwired colours - get from colour scheme
void SizeIndicator::paintEvent(QPaintEvent *ev)
{
    QFrame::paintEvent(ev);				// draw the frame
    QPainter p(this);
    const QRect cr = contentsRect();
    p.setClipRect(cr);					// paint the contents

    int w = cr.width();
    int h = cr.height();

    if (sizeInByte>=threshold)
    {
        QColor warnColor;
        int c = int(double(sizeInByte) * devider);
        if (c>255) c = 255;
        warnColor.setHsv( 0, c, c );

        QLinearGradient g(0, h/2, w, h/2);
        g.setColorAt(0, palette().background().color());
        g.setColorAt(1, warnColor);
        p.fillRect(0, 0, w, h, QBrush(g));
    }
							// display the text
    p.drawText(0, 0, w, h, Qt::AlignHCenter|Qt::AlignVCenter, text());
}
