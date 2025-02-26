/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2025 Jonathan Marten <jjm@keelhaul.me.uk>		*
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

#include "papersizes.h"

#ifdef HAVE_LIBPAPER
extern "C" {
#include <paper.h>
}
#endif

#include "libkookascan_logging.h"

static const PaperSize defaultSizes[] =
{
    { "ISO A3",     297, 420 },
    { "ISO A4",     210, 297 },
    { "ISO A5",     148, 210 },
    { "ISO A6",     105, 148 },
    { "US Letter",  215, 279 },
    { "US Legal",   216, 356 },
    { "9x13",        90, 130 },
    { "10x15",      100, 150 },
    { nullptr,        0,   0 }
};

#define POINT_TO_MM(p)		qRound((p)/72.0*25.4)


PaperSizes *PaperSizes::self()
{
    static PaperSizes *instance = new PaperSizes();
    return (instance);
}


PaperSizes::PaperSizes()
{
#ifdef HAVE_LIBPAPER
    paperinit();					// start to access library

    PaperSize ps;
    for (const struct paper *p = paperfirst(); p!=nullptr; p = papernext(p))
    {							// iterate over defined papers
        //qCDebug(LIBKOOKASCAN_LOG) << "  paper at" << ((void*) p)
        //                          << "name" << papername(p)
        //                          << "width" << paperpswidth(p)
        //                          << "height" << paperpsheight(p);

        // The next line is safe, because a unique pointer (presumably
        // into libpaper's internally allocated data) is returned for
        // each paper size.
        ps.name = papername(p);
        // These sizes are provided by the library in PostScript points (1/72 inch).
        ps.width = POINT_TO_MM(paperpswidth(p));
        ps.height = POINT_TO_MM(paperpsheight(p));
        mSizeList.append(ps);
    }
    qCDebug(LIBKOOKASCAN_LOG) << "using libpaper, got" << mSizeList.size() << "sizes";

    ps.name = nullptr;					// finish with null terminator
    mSizeList.append(ps);

    paperdone();					// finished accessing library
    mSizes = mSizeList.constData();			// set pointer to data
#else
    qCDebug(LIBKOOKASCAN_LOG) << "using default paper size list";
    mSizes = defaultSizes;				// set pointer to defaults
#endif
}
