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

#include "recentsaver.h"

#include <qurl.h>
#include <qdebug.h>
#include <qfileinfo.h>

#include <krecentdirs.h>

#include "libdialogutil_logging.h"


RecentSaver::RecentSaver(const QString &fileClass)
{
    Q_ASSERT(!fileClass.isEmpty());
    mRecentClass = fileClass;
    if (!mRecentClass.startsWith(':')) mRecentClass.prepend(':');
}


QUrl RecentSaver::recentUrl(const QString &suggestedName)
{
    // QUrl::fromLocalFile("") -> QUrl(), so no need for null test here
    return (QUrl::fromLocalFile(recentPath(suggestedName)));
}


QString RecentSaver::recentPath(const QString &suggestedName)
{
    mRecentDir = KRecentDirs::dir(mRecentClass);
    if (!mRecentDir.isEmpty() && !mRecentDir.endsWith('/')) mRecentDir += '/';

    QString recentDir = mRecentDir;
    if (!suggestedName.isEmpty()) recentDir += suggestedName;
    qCDebug(LIBDIALOGUTIL_LOG) << "for" << mRecentClass << "dir" << mRecentDir << "->" << recentDir;
    return (recentDir);
}


void RecentSaver::save(const QUrl &url)
{
    if (!url.isValid()) return;				// didn't get a valid entry
    if (!url.isLocalFile()) return;			// can only save local dirs
    save(url.path());					// save the local path
}


void RecentSaver::save(const QString &path)
{
    if (path.isEmpty()) return;				// didn't get a valid entry

    QString rd = QFileInfo(path).path();		// just take directory path
    if (!rd.endsWith('/')) rd += '/';			// ensure saved as directory
    if (rd==mRecentDir) return;				// nothing new, no need to save

    qCDebug(LIBDIALOGUTIL_LOG) << "for" << mRecentClass << "saving" << rd;
    KRecentDirs::add(mRecentClass, rd);
}
