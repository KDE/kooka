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

#include "imagefilter.h"

#include <qimagereader.h>
#include <qimagewriter.h>
#include <qmimedatabase.h>
#include <qmimetype.h>

#include <klocalizedstring.h>


static bool commentLessThan(const QString &s1, const QString &s2)
{
    const int idx1 = s1.indexOf('|');
    const int idx2 = s2.indexOf('|');
    return (s1.mid(idx1+1).toLower()<s2.mid(idx2+1).toLower());
}


static QStringList filterList(ImageFilter::FilterMode mode, ImageFilter::FilterOptions options, bool kdeFormat)
{
    QStringList list;
    QStringList allPatterns;

    QList<QByteArray> mimeTypes;
    if (mode==ImageFilter::Writing) mimeTypes = QImageWriter::supportedMimeTypes();
    else mimeTypes = QImageReader::supportedMimeTypes();

    QMimeDatabase db;

    foreach (const QByteArray &mimeType, mimeTypes)
    {
        QMimeType mime = db.mimeTypeForName(mimeType);
        if (!mime.isValid()) continue;

        QStringList pats = mime.globPatterns();
        list.append(pats.join(' ')+'|'+mime.comment());
        if (options & ImageFilter::AllImages) allPatterns.append(pats);
    }

    if (!(options & ImageFilter::Unsorted))		// unless list wanted unsorted,
    {							// sort by the mime type comment
        std::sort(list.begin(), list.end(), commentLessThan);
    }

    if (!kdeFormat)
    {
        // We generated a KDE format filter above (for ease of sorting),
        // so if we want a Qt format filter then it needs to be rearranged.
        for (QStringList::iterator it = list.begin(); it!=list.end(); ++it)
        {
            QString &filter = (*it);			// modifying list in place
            int idx = filter.indexOf('|');
            if (idx==-1) continue;
            filter = filter.mid(idx+1)+" ("+filter.left(idx)+')';
            (*it) = filter;
        }
    }

    if (!allPatterns.isEmpty())				// want an "All Images" entry
    {
        if (kdeFormat) list.prepend(i18n("%1|All Image Files", allPatterns.join(' ')));
        else list.prepend(i18n("All Image Files (%1)", allPatterns.join(' ')));
    }

    if (options & ImageFilter::AllFiles)		// want an "All Files" entry
    {
        if (kdeFormat) list.append(i18n("*|All Files"));
        else list.append(i18n("All Files (*)"));
    }

    return (list);
}


QString ImageFilter::qtFilterString(ImageFilter::FilterMode mode, ImageFilter::FilterOptions options)
{
    return (qtFilterList(mode, options).join(";;"));
}


QStringList ImageFilter::qtFilterList(ImageFilter::FilterMode mode, ImageFilter::FilterOptions options)
{
    return (filterList(mode, options, false));
}


QString ImageFilter::kdeFilter(ImageFilter::FilterMode mode, ImageFilter::FilterOptions options)
{
    return (filterList(mode, options, true).join('\n'));
}
