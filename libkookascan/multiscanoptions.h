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

#ifndef MULTISCANOPTIONS_H
#define MULTISCANOPTIONS_H

#include <qbytearray.h>
#include <qstring.h>

#include "kookascan_export.h"

/**
 * @short Options to control multiple scans and ADF handling.
 *
 * This class encapsulates all of the options so that they can be
 * retained by ScanParams and passed to and from the MultiScanDialog.
 *
 * @author Jonathan Marten
 **/

class KOOKASCAN_EXPORT MultiScanOptions
{
public:
    explicit MultiScanOptions();
    ~MultiScanOptions() = default;

    enum Flag
    {
        MultiScan = 0x0001,
        RotateOdd = 0x0002,
        RotateEven = 0x0004,
        RotateBoth = RotateOdd|RotateEven,
        AdfAvailable = 0x0008,
        ManualWait = 0x0010,
        DelayWait = 0x0020,
        BatchMultiple = 0x0040,
        AutoGenerate = 0x0080,
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum Rotation
    {
        RotateNone = 0,
        Rotate90 = 1,
        Rotate180 = 2,
        Rotate270 = 3,
    };

    void setFlags(MultiScanOptions::Flags f, bool on = true);
    MultiScanOptions::Flags flags() const;

    void setSource(const QByteArray &src);
    QByteArray source() const;

    void setRotation(MultiScanOptions::Flags flags, MultiScanOptions::Rotation rotate);
    MultiScanOptions::Rotation rotation(MultiScanOptions::Flags flags) const;

    void setDelay(int delay);
    int delay() const;

    QString toString() const;

private:
    QByteArray mSource;
    MultiScanOptions::Flags mFlags;
    MultiScanOptions::Rotation mRotateOdd;
    MultiScanOptions::Rotation mRotateEven;
    int mDelay;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MultiScanOptions::Flags)


#endif							// MULTISCANOPTIONS_H
