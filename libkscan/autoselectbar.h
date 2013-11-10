/************************************************** -*- mode:c++; -*- ***
 *									*
 *  This file is part of libkscan, a KDE scanning library.		*
 *									*
 *  Copyright (C) 2013 Jonathan Marten <jjm@keelhaul.me.uk>		*
 *									*
 *  This library is free software; you can redistribute it and/or	*
 *  modify it under the terms of the GNU Library General Public		*
 *  License as published by the Free Software Foundation and appearing	*
 *  in the file COPYING included in the packaging of this file;		*
 *  either version 2 of the License, or (at your option) any later	*
 *  version.								*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public License	*
 *  along with this program;  see the file COPYING.  If not, write to	*
 *  the Free Software Foundation, Inc., 51 Franklin Street,		*
 *  Fifth Floor, Boston, MA 02110-1301, USA.				*
 *									*
 ************************************************************************/

#ifndef AUTOSELECTBAR_H
#define AUTOSELECTBAR_H

#include "libkscanexport.h"

#include <qwidget.h>


class KColorPatch;
class KScanSlider;
class AutoSelectDialog;


class KSCAN_EXPORT AutoSelectBar : public QWidget
{
    Q_OBJECT

public:
    AutoSelectBar(int initialValue, QWidget *parent = 0);
    virtual ~AutoSelectBar();

public slots:
    void setThreshold(int thresh);
    void setAdvancedSettings(int margin, bool bgIsWhite, int dustsize);

protected slots:
    void slotThresholdChanged(int value);
    void slotShowSettings();

signals:
    void thresholdChanged(int value);
    void advancedSettingsChanged(int margin, bool bgIsWhite, int dustsize);
    void performSelection();

private:
    KScanSlider *mThresholdSlider;
    KColorPatch *mColourPatch;

    int mMargin;
    int mDustsize;
    bool mBgIsWhite;
};

#endif							// AUTOSELECTBAR_H
