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

#ifndef KOOKAIMAGE_H
#define KOOKAIMAGE_H

#include "kookacore_export.h"

#include <qimage.h>
#include <qurl.h>

class KFileItem;


/**
  * @author Klaas Freitag
  *
  * class that represents an image, very much as QImage. But this one can contain
  * multiple pages.
  */

// TODO: into class (but never used)

class KOOKACORE_EXPORT KookaImage : public QImage
{
public:
    KookaImage();
    explicit KookaImage(const QImage &img);
    ~KookaImage() = default;

    KookaImage &operator=(const KookaImage &src);

    /**
     * load an image from a KURL. This method reads the entire file and sets
     * the subimage count if applicable.  Returns a null string if succeeded,
     * or an error message string if failed.
     */
    QString loadFromUrl(const QUrl &url);

    /**
     * The number of subimages. This is 0 if there are no subimages.
     */
    int subImagesCount() const;

    /**
     * the parent image.
     */
    KookaImage *parentImage() const;

    /**
     * returns true if this is a subimage.
     */
    bool isSubImage() const;

    /**
     *  Set and get the KFileItem of the image. Note that the KFileItem pointer returned
     *  may be NULL.
     */
    const KFileItem *fileItem() const;
    void setFileItem(const KFileItem *item);

    /**
     * set the url of the kooka image. Note that loadFromUrl sets this
     * url automatically.
     */
    void setUrl(const QUrl &url);
    QUrl url() const;

    /**
     * checks if the image is file bound ie. was loaded from file. If this
     * method returns false, fileMetaInfo and FileItem are undefined.
     */
    bool isFileBound() const;


private:
    void init();
    QString loadTiffDir(const QString &file, int subno);

private:
    int         m_subImages;
    QUrl                m_url;
    const KFileItem           *m_fileItem;
    bool                m_fileBound;
};

#endif                          // KOOKAIMAGE_H
