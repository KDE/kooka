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

#ifndef IMAGEFILTER_H
#define IMAGEFILTER_H

#include <qstringlist.h>

#include "kookascan_export.h"


/**
 * @short Generation of a filter string for image selection or saving.
 *
 * This class generates a filter suitable to use with either Qt or
 * KDE Frameworks file dialogues.  Although @c KFileDialog is deprecated
 * in Frameworks, a KDE filter is still required for a @c KUrlRequester
 * or if KFileWidget is used directly.
 *
 * @author Jonathan Marten
 **/

namespace ImageFilter
{
    /**
     * Enumeration specifying the intended use of the filter.
     **/
    enum FilterMode
    {
        Reading = 0x01,					///< Image reading
        Writing = 0x02					///< Image writing
    };

    /**
     * Enumeration specifying options for the filter.
     **/
    enum FilterOption
    {
        NoOptions = 0x00,				///< No options specified
        AllImages = 0x10,				///< Include an "All images" entry
        AllFiles  = 0x20,				///< Include an "All files" entry
        Unsorted  = 0x40				///< Do not sort the returned list
    };
    Q_DECLARE_FLAGS(FilterOptions, FilterOption)

    /**
     * Generate a Qt-style filter list.
     *
     * This is a filter list suitable for passing
     * to @c QFileDialog::setNameFilters().
     * 
     * @param mode The intended file operation mode
     * @param options Options for the filter generation.
     * @return The filter list
     **/
    KOOKASCAN_EXPORT QStringList qtFilterList(ImageFilter::FilterMode mode,
                                              ImageFilter::FilterOptions options = ImageFilter::NoOptions);

    /**
     * Generate a Qt-style filter string.
     *
     * This is a filter string suitable for passing
     * to @c QFileDialog::getOpenFileName() or the similar static functions.
     * 
     * @param mode The intended file operation mode
     * @param options Options for the filter generation.
     * @return The filter string
     **/
    KOOKASCAN_EXPORT QString qtFilterString(ImageFilter::FilterMode mode,
                                            ImageFilter::FilterOptions options = ImageFilter::NoOptions);

    /**
     * Generate a KDE-style filter list.
     *
     * This is a filter string suitable for passing
     * to @c KUrlRequester::setFilter().
     * @param mode The intended file operation mode
     * @param options Options for the filter generation.
     * @return The filter string
     **/
    KOOKASCAN_EXPORT QString kdeFilter(ImageFilter::FilterMode mode,
                                       ImageFilter::FilterOptions options = ImageFilter::NoOptions);
}

Q_DECLARE_OPERATORS_FOR_FLAGS(ImageFilter::FilterOptions)


#endif							// IMAGEFILTER_H
