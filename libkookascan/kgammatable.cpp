/* This file is part of the KDE Project
   Copyright (C) 2002 Klaas Freitag <freitag@suse.de>

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

#include "kgammatable.h"

#include <math.h>

#include <qregularexpression.h>

#include "libkookascan_logging.h"


KGammaTable::KGammaTable(int gamma, int brightness, int contrast)
    : QObject()
{
    mGamma = (gamma < 1) ? 1 : gamma;
    mBrightness = brightness;
    mContrast = contrast;
    init();
}

KGammaTable::KGammaTable(const KGammaTable &other)
    : QObject()
{
    mGamma = other.mGamma;
    mBrightness = other.mBrightness;
    mContrast = other.mContrast;
    init();
}

void KGammaTable::init()
{
    mDirty = true;
}

// Adapted from LabeledGamma::calculateGT()
// in kdegraphics/libksane/libksane/widgets/labeled_gamma.cpp
//
// Input values are expected to be:
//   brightness     -50 ... +50
//   contrast       -50 ... +50
//   gamma           30 ... 300 = 0.3 ... 3.0
//
void KGammaTable::calcTable()
{
    if (mGamma < 1.0) {             // impossibly small
        qCWarning(LIBKOOKASCAN_LOG) << "invalid gamma" << mGamma;
        mDirty = false;                 // pointless, but don't repeat
        return;
    }

    if (mData.isEmpty()) {              // not allocated yet
        mData.resize(256);              // do it now
        qCDebug(LIBKOOKASCAN_LOG) << "allocated table size" << mData.size();
    }

    qCDebug(LIBKOOKASCAN_LOG) << "initial b" << mBrightness << "c" << mContrast << "g" << mGamma;

    const double maxval = (KGammaTable::valueRange - 1);
    double gam = 100.0 / mGamma;
    double con = (200.0 / (100.0 - mContrast)) - 1;
    double halfmax = maxval / 2.0;
    double bri = (mBrightness / halfmax) * maxval;
    qCDebug(LIBKOOKASCAN_LOG) << "adjusted bri" << bri << "con" << con << "gam" << gam;

    for (int i = 0; i < mData.size(); ++i) {
        double x = pow(i / maxval, gam) * maxval;   // apply gamma
        x = (con * (x - halfmax)) + halfmax;    // apply contrast
        x += bri;                   // apply brightness
        mData[i] = qRound(qBound(0.0, x, maxval));  // limit value, save in table
        //qCDebug(LIBKOOKASCAN_LOG) << "  " << i << "->" << mData[i];
    }

    mDirty = false;                 // table now calculated
}

void KGammaTable::setBrightness(int brightness)     // slot
{
    mBrightness = brightness;
    mDirty = true;
    emit tableChanged();
}

void KGammaTable::setContrast(int contrast)     // slot
{
    mContrast = contrast;
    mDirty = true;
    emit tableChanged();
}

void KGammaTable::setGamma(int gamma)           // slot
{
    mGamma = gamma;
    mDirty = true;
    emit tableChanged();
}

void KGammaTable::setAll(int gamma, int brightness, int contrast)
{
    mGamma = gamma < 1 ? 1 : gamma;
    mBrightness = brightness;
    mContrast = contrast;

    mDirty = true;
    emit tableChanged();
}

const int *KGammaTable::getTable(int size)
{
    if (size > 0 && size != mData.size()) {
        qCDebug(LIBKOOKASCAN_LOG) << "resize from" << mData.size() << "to" << size;
        mData.resize(size);
        mDirty = true;
    }

    if (mDirty) {
        calcTable();
    }
    return (mData.constData());
}

// originally done in KScanOption::set(const QByteArray &)
bool KGammaTable::setFromString(const QString &str)
{
    const QRegularExpression re("^(\\d+),(\\d+),(\\d+)$");
    const QRegularExpressionMatch match = re.match(str);
    if (!match.hasMatch()) return (false);		// unrecognised format

    int g = match.captured(1).toInt();
    int b = match.captured(2).toInt();
    int c = match.captured(3).toInt();

    setAll(g, b, c);
    return (true);                  // matched and set
}

// originally done in KScanOption::get()
QString KGammaTable::toString() const
{
    return (QString("%1,%2,%3").arg(mGamma).arg(mBrightness).arg(mContrast));
}
