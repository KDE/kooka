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
 * @short An image representing scanned results or loaded from the gallery.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 *
 * A class that represents an image, based on @c QImage with extensions for
 * image meta-information and supporting image formats that can contain
 * multiple pages.
 */

class KOOKASCAN_EXPORT ScanImage : public QImage
{
public:
    /* A pointer to a @c ScanImage.  Always use this to pass around an image,
     * do not use a plain pointer or copy the image that it points to.
     * The pointer can be copied, the @c QSharedPointer will manage reference
     * counting and deletion.
     */
    typedef QSharedPointer<ScanImage> Ptr;

    /*
     * Constructor.  Creates a null image.
     */
    ScanImage();

    /*
     * Constructor.  Creates an image from a @c QImage.
     * A shallow copy is made of the image data, which as with @c QImage
     * is automatically detached if it is modified.
     *
     * @see QImage::QImage(const QImage &)
     */
    explicit ScanImage(const QImage &img);

    /*
     * Destructor.
     */
    ~ScanImage() = default;

    // TODO: delete, use Q_DISABLE_COPY in private section
    ScanImage &operator=(const ScanImage &src);

    /**
     * Load an image from a URL.
     *
     * If the URL has a numeric fragment, then this requests the specified
     * subimage from the containing file.  Otherwise, read in the main image
     * and set the count of subimages if it has any.
     *
     * @param url URL of the file to be loaded
     * @return a null string if the load succeeded, or an error message string
     */
    QString loadFromUrl(const QUrl &url);

    /**
     * The number of subimages.
     *
     * @return The subimage count, or 0 if there are no subimages
     */
    int subImagesCount() const;

    /**
     * Check whether this image is a subimage.
     *
     * @return @c true if this image is a subimage
     */
    bool isSubImage() const;

    /**
     * The URL of the image.
     *
     * @return the image URL, or an invalid URL if it was not loaded from a file.
     * @note The URL is set when loaded via @c loadFromUrl().
     */
    QUrl url() const;

    /**
     * Checks whether the image is file bound;
     * that is, if it was loaded from a file via @c loadFromurl().
     *
     * @return @c true if the image was loaded from a file
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
