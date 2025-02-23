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

#include <qimage.h>
#include <qurl.h>
#include <qsharedpointer.h>

#include "multiscanoptions.h"

#include "kookascan_export.h"


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
    /**
     * A pointer to a @c ScanImage.  Always use this to pass around an image,
     * do not use a plain pointer or assume that it is a QImage unless it can
     * be assured that it will only be used for immediate reference and will
     * not be retained, copied or deleted.  The shared pointer can be copied,
     * the @c QSharedPointer will manage reference counting and deletion.
     **/
    typedef QSharedPointer<ScanImage> Ptr;

    /**
     * An image type.  This is not the detailed format of the image
     * data (which is available via @c QImage::Format), but a general
     * category which an application or its user will be interested in.
     **/
    enum ImageType
    {
        None        = 0x00,				///< None, unknown or not resolved yet
        BlackWhite  = 0x01,				///< Black/white bitmap
        Greyscale   = 0x02,				///< Grey scale
        LowColour   = 0x04,				///< Low colour (indexed with palette)
        HighColour  = 0x08,				///< High colour (RGB)
        Preview     = 0x10				///< Preview image
    };
    Q_DECLARE_FLAGS(ImageTypes, ImageType)

    /**
     * Constructor.  Creates an image from a @c QImage.
     * A shallow copy is made of the image data, which as with @c QImage
     * is automatically detached if it is modified.
     *
     * @see QImage::QImage(const QImage &)
     **/
    explicit ScanImage(const QImage &img);

    /**
     * Constructor.  Creates an image and loads it from a file.
     *
     * If the URL has a numeric fragment, then this requests the specified
     * subimage from the containing file.  Otherwise,  the main image is read
     * and the count of subimages, if it has any, is set.
     *
     * @param url URL of the file to be loaded
     * @note If there was an error loading the file, then @c errorString() can
     * be used to obtain the error message.
     * @see QImage::QImage(const QString &, const char *)
     **/
    explicit ScanImage(const QUrl &url);

    /**
     * Constructor.  Creates an image with the specified size and @p format.
     **/
    ScanImage(int width, int height, QImage::Format format);

    /**
     * Destructor.
     **/
    ~ScanImage() = default;

    /**
     * Get an error message from the image load operation.
     *
     * @return the error string, or a null string if there was no error.
     **/
    QString errorString() const				{ return (m_errorString); }

    /**
     * The number of subimages.
     *
     * @return the subimage count, or 0 if there are no subimages
     **/
    int subImagesCount() const				{ return (m_subImages); }

    /**
     * Check whether this image is a subimage.
     *
     * @return @c true if this image is a subimage
     **/
    bool isSubImage() const;

    /**
     * The URL of the image.
     *
     * @return the image URL, or an invalid URL if it was not loaded from a file.
     **/
    QUrl url() const					{ return (m_url); }

    /**
     * Checks whether the image is file bound;
     * that is, if it was originally loaded from a file.
     *
     * @return @c true if the image was loaded from a file
     **/
    bool isFileBound() const;

    /**
     * Set the name of the scanner that was used to generate the image.
     *
     * @param scanner the new scanner name
     **/
    void setScannerName(const QByteArray &scanner);

    /**
     * Get the scanner name that was set for the image.
     *
     * @return the scanner name
     * @see setScannerName
     **/
    QByteArray getScannerName() const;

    /**
     * Set the X resolution of the image in dots per inch.
     *
     * @param res the new X resolution
     * @see getXResolution
     **/
    void setXResolution(int res);

    /**
     * Set the Y resolution of the image in dots per inch.
     *
     * @param res the new Y resolution
     * @see getXResolution
     **/
    void setYResolution(int res);

    /**
     * Get the X resolution of the image in dots per inch.
     *
     * @return the X resolution, or the QImage default if it has not been set.
     * @see setXResolution
     **/
    int getXResolution() const;

    /**
     * Get the Y resolution of the image in dots per inch.
     *
     * @return the Y resolution, or the QImage default if it has not been set.
     * @see setYResolution
     **/
    int getYResolution() const;

    /**
     * Set the image type.

     * This is used when an image is being created to receive scan results,
     * if it can be deduced from the SANE parameters.
     *
     * @param type The image type
     **/
    void setImageType(ScanImage::ImageType type)	{ mImageType = type; }

    /**
     * Get the image type.
     *
     * If the image type was initially set from the SANE parameters
     * via @c setImageType(), then that is returned.  If no image type has
     * been set then the type is deduced from the image data.
     *
     * @return the image type.
     **/
    ScanImage::ImageType imageType() const;

    /**
     * Apply a transformation to the image.
     *
     * @p rotateBy The rotation required
     **/
    void transform(MultiScanOptions::Rotation rotateBy);

private:
    void init();
    QString loadTiffDir(const QString &file, int subno);

private:
    Q_DISABLE_COPY(ScanImage)

    int m_subImages;
    QUrl m_url;
    QString m_errorString;
    ScanImage::ImageType mImageType;
};

// Not doing Q_DECLARE_OPERATORS_FOR_FLAGS(ScanImage::ImageTypes) here.
// The only place that uses an OR-ed combination of types is FormatDialog,
// so the operators are declared there.

// Allow the shared pointer to be stored in a QVariant.
Q_DECLARE_METATYPE(ScanImage::Ptr)

/**
 * Conversion between resolutions in dots-per-inch (used by SANE and the scanner GUI)
 * and dots-per-metre (used by QImage).
 **/
#define DPM_TO_DPI(d)		qRound((d)*2.54/100)	// dots/metre -> dots/inch
#define DPI_TO_DPM(d)		qRound((d)*100/2.54)	// dots/inch -> dots/metre


#endif							// SCANIMAGE_H
