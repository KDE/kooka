/* This file is part of the KDE Project         -*- mode:c++; -*-
   Copyright (C) 2008 Jonathan Marten <jjm@keelhaul.me.uk>

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

#ifndef SCANSIZESELECTOR_H
#define SCANSIZESELECTOR_H

#include "libkscanexport.h"

#include <qsize.h>
#include <qrect.h>

#include <kvbox.h>

class QComboBox;
class QRadioButton;

class PaperSize;

class KSCAN_EXPORT ScanSizeSelector : public KVBox
{
    Q_OBJECT

public:
    ScanSizeSelector(QWidget *parent, const QSize &bedSize);
    ~ScanSizeSelector();

    void selectCustomSize(const QRect &rect);
    void selectSize(const QRect &rect);

protected slots:
    void slotSizeSelected(int idx);
    void slotPortraitLandscape(int id);

signals:
    void sizeSelected(const QRect &size);

private:
    void newScanSize(int width, int height);
    void implementPortraitLandscape(const PaperSize *sp);
    void implementSizeSetting(const PaperSize *sp);

    int bedWidth, bedHeight;

    QRect m_customSize;
    int m_prevSelected;

    QComboBox *m_sizeCb;
    QRadioButton *m_portraitRb;
    QRadioButton *m_landscapeRb;
};

#endif                          // SCANSIZESELECTOR_H
