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

#ifndef SCANICONS_H
#define SCANICONS_H

#include "kookascan_export.h"

#include <qicon.h>


/**
 * @short Access to scanner icons.
 *
 * This class provides access to icons and pixmaps representing an scan
 * or an image type.
 *
 * @author Jonathan Marten
 **/

class KOOKASCAN_EXPORT ScanIcons
{
public:
    /**
     * Access to the singleton object.
     *
     * @return the object
     **/
    static ScanIcons *self();

    /**
     * An icon type.
     **/
    enum IconType
    {
        BlackWhite,				///< Black/white bitmap
        Greyscale,				///< Grey scale
        Halftone,				///< Halftone
        Colour					///< Colour
    };

    /**
     * Get an icon for the specified scan or image type.
     *
     * @param type The required type
     * @return The requested icon
     **/
    QIcon icon(ScanIcons::IconType type);

private:
    explicit ScanIcons();

private:
    QIcon mBlackWhiteIcon;
    QIcon mGreyscaleIcon;
    QIcon mHalftoneIcon;
    QIcon mColourIcon;
};

#endif							// SCANICONS_H
