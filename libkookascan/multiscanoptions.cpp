/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2024  Jonathan Marten <jjm@keelhaul.me.uk>		*
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

#include "multiscanoptions.h"


MultiScanOptions::MultiScanOptions()
{
    mSource.clear();
    mOptions = MultiScanOptions::Flags();
    mRotateOdd = MultiScanOptions::RotateNone;
    mRotateEven = MultiScanOptions::RotateNone;
    mDelay = -1;
}


void MultiScanOptions::setOptions(MultiScanOptions::Flags opts)
{
    mOptions = opts;
}


MultiScanOptions::Flags MultiScanOptions::options() const
{
    return (mOptions);
}


void MultiScanOptions::setSource(const QByteArray &src)
{
    mSource = src;
}


QByteArray MultiScanOptions::source() const
{
    return (mSource);
}


void MultiScanOptions::setRotation(MultiScanOptions::Flags flags, MultiScanOptions::Rotation rotate)
{
    if (flags & MultiScanOptions::RotateOdd) mRotateOdd = rotate;
    if (flags & MultiScanOptions::RotateEven) mRotateEven = rotate;
}


MultiScanOptions::Rotation MultiScanOptions::rotation(MultiScanOptions::Flags flags) const
{
    if ((flags & MultiScanOptions::RotateOdd) && (mOptions & MultiScanOptions::RotateOdd)) return (mRotateOdd);
    if ((flags & MultiScanOptions::RotateEven) && (mOptions & MultiScanOptions::RotateEven)) return (mRotateEven);
    return (MultiScanOptions::RotateNone);
}


void MultiScanOptions::setDelay(int delay)
{
    mDelay = delay;
}


int MultiScanOptions::delay() const
{
    return (mDelay);
}


QString MultiScanOptions::toString() const
{
    return (QString("[flags %1 src '%2' rot %3/%4 delay %5]").arg(mOptions, 4, 16, QLatin1Char('0'))
                                                             .arg(mSource.constData())
                                                             .arg(mRotateOdd).arg(mRotateEven)
                                                             .arg(mDelay));
}
