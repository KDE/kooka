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

#ifndef IMGSAVER_H
#define IMGSAVER_H

#include <qurl.h>

#include "kookacore_export.h"

#include "scanimage.h"
#include "imageformat.h"

class QWidget;


/**
 * @short Manages the saving of images.
 *
 * All saving of images within the application is handled here.  Depending
 * on the user's preference settings and saved information, it may either
 * decide where to save the image automatically, or ask the user for
 * a file name and file format.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class KOOKACORE_EXPORT ImgSaver
{
public:

    /**
     * Status return information.
     *
     * @see errorString
     **/
    enum ImageSaveStatus
    {
        SaveStatusOk,					///< Success
        SaveStatusPermission,				///< Permission denied
        SaveStatusBadFilename,				///< Bad file name
        SaveStatusNoSpace,				///< No space left on device
        SaveStatusFormatNoWrite,			///< Cannot write this image format
        SaveStatusFailed,				///< Image save failed
        SaveStatusParam,				///< Bad parameter
        SaveStatusProtocol,				///< Cannot write to this protocol
        SaveStatusMkdir,				///< Cannot create directory
        SaveStatusCanceled				///< User cancelled
    };

    /**
     * Constructor.
     *
     * @param dir The directory into which the image is to eventually
     * be saved.  If it does not exist, it will be created at this point.
     * The default is the current gallery root.
     **/
    explicit ImgSaver(const QUrl &dir = QUrl());

    /**
     * Save an image.
     *
     * @param image The image to save.
     * @param url Where to save the image to. This must be a complete URL
     *            including file name.
     * @param format The format to save the image in.
     * @param subformat Additional information for the format.
     *
     * @return Status of the save
     *
     * @note The @c dir parameter to the constructor is ignored.
     **/
    ImgSaver::ImageSaveStatus saveImage(const ScanImage::Ptr image,
                                        const QUrl &url,
                                        const ImageFormat &format,
                                        const QString &subformat = QString());

    /**
     * Save an image.
     *
     * The image is saved to the @c dir location as specified in the
     * constructor.  Depending on the settings of the application preferences,
     * whether @c setImageInfo has previously been called with enough
     * information, or the previously remembered settings, this may prompt
     * the user for a file name, a file format, or both.
     *
     * @param image The image to save
     *
     * @return Status of the save
     *
     * @see setImageInfo
     **/
    ImgSaver::ImageSaveStatus saveImage(const ScanImage::Ptr image);

    /**
     * Set image information prior to saving.
     *
     * If there is enough information provided, the file name and save
     * format will be resolved and remembered for a subsequent @c saveImage
     * operation.  The user may be prompted for a file name and/or a file
     * format at this point.
     *
     * @param type the image type
     * @return Status of the operation
     * @see saveImage
     **/
    ImgSaver::ImageSaveStatus setImageInfo(ScanImage::ImageType type);

    /**
     * Get a readable description for an status return code.
     *
     * @return The I18N'ed message.
     **/
    QString errorString(ImgSaver::ImageSaveStatus status) const;

    /**
     * Copy a image file.
     *
     * The file is copied, without any interpretation of the contents,
     * using KIO.
     *
     * @param fromUrl The existing file to be copied.
     * @param toUrl The destination file, which must not already exist.
     * @param overWidget Top level widget, for KIO access purposes.
     *
     * @return @c true if the copy operation succeeded.
     *
     * @note If the @p toUrl does not have a file name extension
     * corresponding to any a known MIME type but the @p fromUrl does,
     * then the user is asked whether to add the same extension as
     * the @p fromUrl.
     *
     * @note If the @p toUrl and the @p fromurl both do have a file name
     * extensions, then they must resolve to the same MIME type.
     **/
    static bool copyImage(const QUrl &fromUrl, const QUrl &toUrl, QWidget *overWidget = nullptr);

    /**
     * Rename a image file.
     *
     * The file is renamed or moved, without any interpretation of the
     * contents, using KIO.
     *
     * @param fromUrl The existing file to be renamed.
     * @param toUrl The destination file, which must not already exist.
     * @param overWidget Top level widget, for KIO access purposes.
     *
     * @return @c true if the rename operation succeeded.
     *
     * @note If the @p toUrl does not have a file name extension
     * corresponding to any a known MIME type but the @p fromUrl does,
     * then the user is asked whether to add the same extension as
     * the @p fromUrl.
     *
     * @note If the @p toUrl and the @p fromurl both do have a file name
     * extensions, then they must resolve to the same MIME type.
     **/
    static bool renameImage(const QUrl &fromUrl, const QUrl &toUrl, bool askExt = false, QWidget *overWidget = nullptr);

    /**
     * Check the remembered save format for an image type.
     *
     * @param type Type of image
     * @param format Save format
     *
     * @return @c true if @p format is the remembered format for the image @p type.
     **/
    static bool isRememberedFormat(ScanImage::ImageType type, const ImageFormat &format);

    /**
     * Get a readable description for an image type.
     *
     * @param type Type of image
     * @return The readable description
     **/
    static QString picTypeAsString(ScanImage::ImageType type);

    /**
     * Get the location where the image will be saved to.
     *
     * @return Save location
     **/
    QUrl saveURL() const
    {
        return (mSaveUrl);
    }

    /**
     * Get the location where the previous image was saved to.
     *
     * @return Save location
     **/
    QUrl lastURL() const
    {
        return (mLastUrl);
    }

private:
    QString createFilename();
    ImgSaver::ImageSaveStatus getFilenameAndFormat(ScanImage::ImageType type);

    QUrl m_saveDirectory;				// dir where the image should be saved
    QByteArray mLastFormat;
    QUrl mLastUrl;

    bool m_saveAskFormat;
    bool m_saveAskFilename;

    QUrl mSaveUrl;
    ImageFormat mSaveFormat;
    QString mSaveSubformat;
};

#endif							// IMGSAVER_H
