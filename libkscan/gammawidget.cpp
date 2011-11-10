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

#include "gammawidget.h"
#include "gammawidget.moc"

#include <qpainter.h>
#include <qpixmap.h>
#include <qevent.h>

#include <kdebug.h>

#include "kgammatable.h"


static const int MARGIN = 5;				// margin aroung display
static const int NGRID = 4;				// number of grid lines


GammaWidget::GammaWidget(KGammaTable *table, QWidget *parent)
    : QWidget(parent)
{
    mTable = table;
    connect(mTable, SIGNAL(tableChanged()), SLOT(repaint()));

    QSizePolicy pol(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setSizePolicy(pol);
}


void GammaWidget::paintEvent(QPaintEvent *ev)
{
    QPainter p(this);

    // Calculate the minimum display size
    int size = qMin(width(), height())-2*MARGIN;

    // Limit the painting area to a square of that size
    p.setViewport(MARGIN, MARGIN, size, size);
    p.setWindow(0, 0, size, size);

    // background and outer frame
    p.setBrush(palette().base());
    p.setPen(palette().windowText().color());
    p.drawRect(0, 0, size, size);

    // grid lines
    p.setPen(QPen(palette().mid().color(), 0, Qt::DotLine));
    int step = size/(NGRID+1);
    for (int l = 1; l<=NGRID; ++l)
    {
        p.drawLine(1, l*step, size-1, l*step);		// horizontal line
        p.drawLine(l*step, 1, l*step, size-1);		// vertical line
    }

    if (mTable==NULL) return;				// no values to draw

    // the gamma curve
    p.setPen(palette().highlight().color());
    const int *vals = mTable->getTable();
    int nvals = mTable->tableSize();
    int maxval = KGammaTable::valueRange;
    int py = (maxval-1)-((size-1)*vals[0])/maxval;
    for (int v = 1; v<nvals; ++v)
    {
        int x = (size*v)/nvals;
        int y = (maxval-1)-((size-1)*vals[v])/maxval;

        p.drawLine(x-1, py, x, y);
        py = y;
    }
}


QSize GammaWidget::sizeHint() const
{
    return (QSize(256+2*MARGIN, 256+2*MARGIN));
}
