/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2008-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#ifndef SCANSIZESELECTOR_H
#define SCANSIZESELECTOR_H

#include "kookascan_export.h"

#include <qframe.h>
#include <qsize.h>
#include <qrect.h>


class QComboBox;
class QRadioButton;

struct PaperSize;

class KOOKASCAN_EXPORT ScanSizeSelector : public QFrame
{
    Q_OBJECT

public:
    explicit ScanSizeSelector(QWidget *parent, const QSize &bedSize);
    virtual ~ScanSizeSelector() = default;

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

    int m_bedWidth, m_bedHeight;

    QRect m_customSize;
    int m_prevSelected;

    QComboBox *m_sizeCb;
    QRadioButton *m_portraitRb;
    QRadioButton *m_landscapeRb;
};

#endif                          // SCANSIZESELECTOR_H
