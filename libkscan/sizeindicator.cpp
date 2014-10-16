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

#include <qpainter.h>
#include <qevent.h>
#include <QLinearGradient>

#include <klocale.h>
#include <QDebug>
#include <kglobal.h>
#include <kcolorscheme.h>
#include <KFormat>

SizeIndicator::SizeIndicator(QWidget *parent, long thresh, long crit)
    : QLabel(parent)
{
    setFrameStyle(QFrame::Box | QFrame::Sunken);
    setMinimumWidth(fontMetrics().width("MMMM.MM MiB"));

    mThreshold = thresh;
    mCritical = crit;
    mSizeInByte = -1;
}

SizeIndicator::~SizeIndicator()
{
}

void SizeIndicator::setCritical(long crit)
{
    mCritical = crit;
    if (mSizeInByte >= 0 && isVisible()) {
        update();
    }
}

void SizeIndicator::setThreshold(long thresh)
{
    mThreshold = thresh;
    if (mSizeInByte >= 0 && isVisible()) {
        update();
    }
}

void SizeIndicator::setSizeInByte(long newSize)
{
    //qDebug() << "New size" << newSize << "bytes";

    mSizeInByte = newSize;

    if (newSize < 0) {
        setText("");
    } else {
        setText(KFormat().formatByteSize(newSize));
    }
    // TODO: do we want JEDEC units here?
}

void SizeIndicator::paintEvent(QPaintEvent *ev)
{
    QFrame::paintEvent(ev);             // draw the frame

    QPainter p(this);

    const QRect cr = contentsRect();
    p.setClipRect(cr);                  // paint the contents
    int w = cr.width();
    int h = cr.height();

    KColorScheme sch(QPalette::Normal, KColorScheme::Button);

    QLinearGradient g;

    if (mSizeInByte < DEFAULT_SMALL) {
        p.fillRect(0, 0, w, h, sch.background(KColorScheme::PositiveBackground).color());
    } else if (mSizeInByte < mThreshold) {
        int t = w * (1.0 - (double(mSizeInByte - DEFAULT_SMALL) / (mThreshold - DEFAULT_SMALL)));
        g.setStart(t - w / 5, h / 2);
        g.setFinalStop(t + w / 5, h / 2);
        g.setColorAt(0, sch.background(KColorScheme::PositiveBackground).color());
        g.setColorAt(1, sch.background(KColorScheme::NeutralBackground).color());
        p.fillRect(0, 0, w, h, QBrush(g));
    } else if (mSizeInByte < mCritical) {
        int t = w * (1.0 - (double(mSizeInByte - mThreshold) / (mCritical - mThreshold)));
        g.setStart(t - w / 5, h / 2);
        g.setFinalStop(t + w / 5, h / 2);
        g.setColorAt(0, sch.background(KColorScheme::NeutralBackground).color());
        g.setColorAt(1, sch.background(KColorScheme::NegativeBackground).color());
        p.fillRect(0, 0, w, h, QBrush(g));
    } else {
        p.fillRect(0, 0, w, h, sch.background(KColorScheme::NegativeBackground).color());
    }
    // display the text
    p.drawText(0, 0, w, h, Qt::AlignHCenter | Qt::AlignVCenter, text());
}
