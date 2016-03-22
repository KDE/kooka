/* This file is part of the KDE Project
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef IMAGEMETAINFO_H
#define IMAGEMETAINFO_H

#include "kookascan_export.h"

#include <qstring.h>

class QImage;

/**
 * @short Image meta-information.
 *
 * Information about an image which may be useful to an application, but
 * is not stored with or is not readily available from a @c QImage.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class KOOKASCAN_EXPORT ImageMetaInfo
{
public:

    /**
     * An image type.  This is not the detailed format of the image
     * data (which is available via @c QImage::Format), but a general
     * category which an application or its user will be interested in.
     **/
    enum ImageType
    {
        Unknown     = 0x00,				///< Unknown or not resolved yet
        BlackWhite  = 0x01,				///< Black/white bitmap
        Greyscale   = 0x02,				///< Grey scale (indexed with palette)
        LowColour   = 0x04,				///< Low colour (indexed with palette)
        HighColour  = 0x08,				///< High colour (RGB)
        Preview     = 0x10,				///< A preview image (application defined)
        Thumbnail   = 0x20				///< A thumbnail image (application defined)
    };
    Q_DECLARE_FLAGS(ImageTypes, ImageType)

    /**
     * Constructor.
     *
     * The image type is initialised to @c Unknown, and the
     * X and Y resolutions to -1.
     **/
    explicit ImageMetaInfo();

    /**
     * Set the X resolution of the image.
     *
     * @param res The new X resolution
     * @see getXResolution
     **/
    void setXResolution(int res)
    {
        m_xRes = res;
    }

    /**
     * Set the Y resolution of the image.
     *
     * @param res The new Y resolution
     * @see getYResolution
     **/
    void setYResolution(int res)
    {
        m_yRes = res;
    }

    /**
     * Set the mode of the image.
     *
     * This is intended to be a user readable description.
     *
     * @param mode The new mode string
     * @see getMode
     **/
    void setMode(const QString &mode)
    {
        m_mode = mode;
    }

    /**
     * Set the name of the scanner that was used to generate the image.
     *
     * @param scanner The new scanner name
     * @see getScannerName
     **/
    void setScannerName(const QString &scanner)
    {
        m_scanner = scanner;
    }

    /**
     * Set the image type.
     *
     * @param type The new image type
     * @see getImageType
     **/
    void setImageType(ImageMetaInfo::ImageType type)
    {
        m_type = type;
    }

    /**
     * Get the X resolution of the image.
     *
     * @return The X resolution, or -1 if it has not been set.
     * @see setXResolution
     **/
    int getXResolution() const
    {
        return (m_xRes);
    }

    /**
     * Get the Y resolution of the image.
     *
     * @return The Y resolution, or -1 if it has not been set.
     * @see setYResolution
     **/
    int getYResolution() const
    {
        return (m_yRes);
    }

    /**
     * Get the mode description of the image.
     *
     * @return The mode string
     * @see setMode
     **/
    QString getMode() const
    {
        return (m_mode);
    }

    /**
     * Get the scanner name for the image.
     *
     * @return The scanner name
     * @see setScannerName
     **/
    QString getScannerName() const
    {
        return (m_scanner);
    }

    /**
     * Get the image type.
     *
     * @return The set image type.
     * @see setImageType
     **/
    ImageMetaInfo::ImageType getImageType() const
    {
        return (m_type);
    }

    /**
     * Deduce the image type for an image.
     *
     * @param image The image to examine
     * @return The image type
     *
     * @note This will never return the type as @c Preview or @c Thumbnail.
     **/
    static ImageType findImageType(const QImage *image);

private:
    int m_xRes;
    int m_yRes;
    QString m_mode;
    QString m_scanner;
    ImageMetaInfo::ImageType m_type;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ImageMetaInfo::ImageTypes)

#endif							// IMAGEMETAINFO_H
