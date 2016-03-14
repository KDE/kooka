/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
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

#include "gammawidget.h"

#include <qpainter.h>
#include <qpixmap.h>
#include <qevent.h>
#include <qdebug.h>

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
    int size = qMin(width(), height()) - 2 * MARGIN;

    // Limit the painting area to a square of that size
    p.setViewport(MARGIN, MARGIN, size, size);
    p.setWindow(0, 0, size, size);

    // background and outer frame
    p.setBrush(palette().base());
    p.setPen(palette().windowText().color());
    p.drawRect(0, 0, size, size);

    // grid lines
    p.setPen(QPen(palette().mid().color(), 0, Qt::DotLine));
    int step = size / (NGRID + 1);
    for (int l = 1; l <= NGRID; ++l) {
        p.drawLine(1, l * step, size - 1, l * step);	// horizontal line
        p.drawLine(l * step, 1, l * step, size - 1);	// vertical line
    }

    if (mTable == NULL) return;				// no values to draw

    // the gamma curve
    p.setPen(palette().highlight().color());

    const int *vals = mTable->getTable();
    const int nvals = mTable->tableSize();
    const int maxval = KGammaTable::valueRange;
    p.setWindow(0, 0, nvals, maxval);

    int py = maxval-vals[0];
    for (int x = 1; x<nvals; ++x)
    {
        int y = maxval-vals[x];
        p.drawLine(x-1, py, x, y);
        py = y;
    }
}

QSize GammaWidget::sizeHint() const
{
    return (QSize(256 + 2 * MARGIN, 256 + 2 * MARGIN));
}
