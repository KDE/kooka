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

#ifndef RECENTSAVER_H
#define RECENTSAVER_H

#include <qstring.h>
#include "libdialogutil_export.h"

class QUrl;


/**
 * @short A helper to look up and save recent locations for a file dialogue.
 *
 * Saving the recent locations is handled by @c KRecentDirs, but a bit of code
 * is needed before and after each file dialogue to look up the appropriate
 * recent location and save it afterwards.  This class automates that.
 *
 * @see KRecentDirs
 * @see QFileDialog
 * @author Jonathan Marten
 **/

class LIBDIALOGUTIL_EXPORT RecentSaver
{
public:
    /**
     * Constructor.
     *
     * @param fileClass The name of the file class.  If the application-global
     * recent locations list is to be used then this may start with a single ":",
     * but need not do so; if it does not start with a ":" then one is assumed.
     * If the system-global recent locations list is to be used then this
     * must start with "::".
     **/
    explicit RecentSaver(const QString &fileClass);

    /**
     * Destructor.
     **/
    ~RecentSaver();

    /**
     * Resolve the saved recent location (if there is one) and a suggested
     * file name (if required) into a URL to pass to a @c QFileDialog
     * which expects a URL.
     *
     * @param suggestedName The suggested file name, or a null string
     * if none is required.
     **/
    QUrl recentUrl(const QString &suggestedName = QString::null);

    /**
     * Resolve the saved recent location (if there is one) and a suggested
     * file name (if required) into a path, to pass to a @c QFileDialog
     * which expects a pathname.
     *
     * @param suggestedName The suggested file name, or a null string
     * if none is required.
     **/
    QString recentPath(const QString &suggestedName = QString::null);

    /**
     * Save the location selected by the file dialogue as a new recent location.
     *
     * @param url The URL returned from the file dialogue.
     **/
    void save(const QUrl &url);

    /**
     * Save the location selected by the file dialogue as a new recent location.
     *
     * @param url The file path returned from the file dialogue.
     **/
    void save(const QString &path);

private:
    QString mRecentClass;
    QString mRecentDir;
};

#endif							// RECENTSAVER_H
