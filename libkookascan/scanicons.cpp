/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2016 Jonathan Marten <jjm@keelhaul.me.uk>		*
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

#include "scanicons.h"

#include <qicon.h>

#include <kiconloader.h>

#include "libkookascan_logging.h"


// This is safe - the ScanIcons instance will be not be constructed
// until long after application startup.
static QMap<QByteArray, ScanIcons::IconType> sModeMap;


ScanIcons::ScanIcons()
{
    KIconLoader::global()->addAppDir("libkookascan");	// access to our icons

    // The option display strings are translated via the 'sane-backends' message
    // catalogue, see KScanCombo::setList().  But finding a scan mode icon works on
    // the untranslated strings, which are really SANE values.  So the strings
    // here need to cover the full set of possible SANE values for all backends
    // (extra ones which a particular SANE backend doesn't support don't matter).
    //
    // These map key strings therefore need to be untranslated SANE option values.
    // So don't translate these strings or wrap them in any sort of i18n().

    sModeMap.insert("Lineart", ScanIcons::BlackWhite);
    sModeMap.insert("Binary", ScanIcons::BlackWhite);
    sModeMap.insert("Gray", ScanIcons::Greyscale);
    sModeMap.insert("Grayscale", ScanIcons::Greyscale);
    sModeMap.insert("Color", ScanIcons::Colour);
    sModeMap.insert("Halftone", ScanIcons::Halftone);
}


ScanIcons *ScanIcons::self()
{
    static ScanIcons *instance = nullptr;
    if (instance==nullptr) instance = new ScanIcons;
    return (instance);
}


static QIcon findIcon(ScanIcons::IconType type, QIcon *var, const QString &name1, const QString &name2)
{
    if (var->isNull())					// not found yet
    {
        // First try to find an icon as installed by libksane, with canReturnNull
        // set so that a null path will be returned if the icon was not found.
        // They look better than our rather ancient ones, so use them
        // if possible.
        QString ip = KIconLoader::global()->iconPath(name1, KIconLoader::Small, true);
        // Then try our own icons, returning the 'unknown' icon if not found.
        if (ip.isEmpty()) ip = KIconLoader::global()->iconPath(name2, KIconLoader::Small);
        qCDebug(LIBKOOKASCAN_LOG) << "for" << type << "using" << ip;
        *var = QIcon(ip);				// save for next time
        Q_ASSERT(!var->isNull());
    }

    return (*var);
}


QIcon ScanIcons::icon(ScanIcons::IconType type)
{
    switch (type)
    {
case ScanIcons::BlackWhite:	return (findIcon(type, &mBlackWhiteIcon, "black-white", "palette-lineart"));
case ScanIcons::Greyscale:	return (findIcon(type, &mGreyscaleIcon, "gray-scale", "palette-gray"));
case ScanIcons::Halftone:	return (findIcon(type, &mHalftoneIcon, "halftone", "palette-halftone"));
case ScanIcons::Colour:		return (findIcon(type, &mColourIcon, "color", "palette-color"));
default:			return (QIcon());
    }
}


QIcon ScanIcons::icon(const QByteArray &scanMode)
{
    if (scanMode.isEmpty()) return (QIcon());
    if (!sModeMap.contains(scanMode))
    {
        qCWarning(LIBKOOKASCAN_LOG) << "Don't know what type of scan" << scanMode << "is. Please add it to ScanIcons::ScanIcons().";
        return (QIcon());
    }

    return (icon(sModeMap.value(scanMode)));
}


QList<QByteArray> ScanIcons::allModes() const
{
    return (sModeMap.keys());
}
