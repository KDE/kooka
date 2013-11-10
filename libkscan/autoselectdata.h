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

#ifndef AUTOSELECTDATA_H
#define AUTOSELECTDATA_H

/**
 * This namespace collects together constants and limits for auto-selection.
 *
 * @author Jonathan Marten
 */

namespace AutoSelectData
{
    // Item indexes for the scanner background
    const int ItemIndexBlack = 0;
    const int ItemIndexWhite = 1;

    // Default values for the auto-selection settings
    const int DefaultMargin = 0;
    const int DefaultThreshold = 25;
    const int DefaultBackground = ItemIndexWhite;
    const int DefaultDustsize = 5;

    // Limits for the auto-selection settings
    const int MaximumMargin = 20;
    const int MaximumThreshold = 100;
    const int MaximumDustsize = 50;

    // Configuration strings for the scanner background
    const char ConfigValueBlack[] = "black";
    const char ConfigValueWhite[] = "white";

    // Configuration strings for the auto-selection state
    const char ConfigValueOn[] = "on";
    const char ConfigValueOff[] = "off";
};

#endif							// AUTOSELECTDATA_H
