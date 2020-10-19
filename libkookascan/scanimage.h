/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#ifndef SCANIMAGE_H
#define SCANIMAGE_H

#include "kookascan_export.h"

#include <qimage.h>
#include <qurl.h>


/**
  * @author Klaas Freitag
  *
  * class that represents an image, very much as QImage. But this one can contain
  * multiple pages.
  */

// TODO: into class (but never used)

class KOOKASCAN_EXPORT ScanImage : public QImage
{
public:
    ScanImage();
    explicit ScanImage(const QImage &img);
    ~ScanImage() = default;

    // TODO: delete
    ScanImage &operator=(const ScanImage &src);

    /**
     * Load an image from a URL. This method reads the image file and sets
     * the subimage count if applicable.  Returns a null string if succeeded,
     * or an error message string if failed.
     */
    QString loadFromUrl(const QUrl &url);

    /**
     * The number of subimages. This is 0 if there are no subimages.
     */
    int subImagesCount() const;

    /**
     * returns true if this is a subimage.
     */
    bool isSubImage() const;

    /**
     * Get the URL of the image, if it was loaded from a file.  Note that
     * loadFromUrl() records the URL automatically.
     */
    void setUrl(const QUrl &url);
    QUrl url() const;

    /**
     * Checks if the image is file bound, i.e. it was loaded from a file.
     */
    bool isFileBound() const;

private:
    void init();
    QString loadTiffDir(const QString &file, int subno);

private:
    int m_subImages;
    QUrl m_url;
};

#endif							// SCANIMAGE_H
