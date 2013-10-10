/***************************************************** -*- mode:c++; -*- ***
                          img_saver.h  -  description
                             -------------------
    begin                : Mon Dec 27 1999
    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

#ifndef IMGSAVER_H
#define IMGSAVER_H


#include <kurl.h>

#include "imageformat.h"
#include "libkscan/imagemetainfo.h"


#define OP_SAVER_GROUP		"Files"

#define OP_SAVER_ASK_FORMAT	"AskForSaveFormat"
#define OP_SAVER_ASK_FILENAME	"AskForFilename"
#define OP_SAVER_ASK_BEFORE	"AskBeforeScan"
#define OP_SAVER_REC_FMT	"OnlyRecommended"

#define OP_FORMAT_HICOLOR	"HiColorSaveFormat"
#define OP_FORMAT_COLOR		"ColorSaveFormat"
#define OP_FORMAT_GRAY		"GraySaveFormat"
#define OP_FORMAT_BW		"BWSaveFormat"
#define OP_FORMAT_THUMBNAIL	"ThumbnailFormat"
#define OP_FORMAT_UNKNOWN	"UnknownFormat"


class KookaImage;


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

class ImgSaver
{
public:

    /**
     * Status return information.
     *
     * @see errorString
     **/
    enum ImageSaveStatus
    {
        SaveStatusOk,				///< Success
        SaveStatusPermission,			///< Permission denied
        SaveStatusBadFilename,			///< Bad file name
        SaveStatusNoSpace,			///< No space left on device
        SaveStatusFormatNoWrite,		///< Cannot write this image format
        SaveStatusFailed,			///< Image save failed
        SaveStatusParam,			///< Bad parameter
        SaveStatusProtocol,			///< Cannot write to this protocol
        SaveStatusMkdir,			///< Cannot create directory
        SaveStatusCanceled			///< User cancelled
    };

    /**
     * Constructor.
     *
     * @param dir The directory into which the image is to eventually
     * be saved.  If it does not exist, it will be created at this point.
     * The default is the current gallery root.
     **/
    ImgSaver(const KUrl &dir = KUrl());

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
    ImgSaver::ImageSaveStatus saveImage(const QImage *image,
                                        const KUrl &url,
                                        const ImageFormat &format,
                                        const QString &subformat = QString::null);

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
    ImgSaver::ImageSaveStatus saveImage(const QImage *image);

    /**
     * Set image information prior to saving.
     *
     * If there is enough information provided, the file name and save
     * format will be resolved and remembered for a subsequent @c saveImage
     * operation.  The user may be prompted for a file name and/or a file
     * format at this point.
     *
     * @param info Image information
     * @return Status of the operation
     *
     * @see saveImage
     **/
    ImgSaver::ImageSaveStatus setImageInfo(const ImageMetaInfo *info);

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
    static bool copyImage(const KUrl &fromUrl, const KUrl &toUrl, QWidget *overWidget = NULL);

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
    static bool renameImage(const KUrl &fromUrl, const KUrl &toUrl, bool askExt = false, QWidget *overWidget = NULL);

    /**
     * Save an image to a temporary file.
     *
     * @param img The image to save
     * @param format The image format to save in.
     * @param colors The colour depth (bits per pixel) required.  If specified,
     * this must be either 1, 8, 24 or 32.  The default is for no colour
     * conversion.
     *
     * @return The file name as saved, or @c QString::null if there was
     *         an error.
     **/
    static QString tempSaveImage(const KookaImage *img, const ImageFormat &format, int colors = -1);

    /**
     * Check the remembered save format for an image type.
     *
     * @param type Type of image
     * @param format Save format
     *
     * @return @c true if @p format is the remembered format for the image @p type.
     **/
    static bool isRememberedFormat(ImageMetaInfo::ImageType type, const ImageFormat &format);

    /**
     * Get a readable description for an image type.
     *
     * @param type Type of image
     * @return The readable description
     **/
    static QString picTypeAsString(ImageMetaInfo::ImageType type);

    /**
     * Get the location where the image will be saved to.
     *
     * @return Save location
     **/
    KUrl saveURL() const			{ return (mSaveUrl); }

    /**
     * Get the location where the previous image was saved to.
     *
     * @return Save location
     **/
    KUrl lastURL() const			{ return (mLastUrl); }

private:
    static ImageFormat findFormat(ImageMetaInfo::ImageType type);
    static QString findSubFormat(const ImageFormat &format);

    static ImageFormat getFormatForType(ImageMetaInfo::ImageType type);
    static void storeFormatForType(ImageMetaInfo::ImageType type, const ImageFormat &format);

    QString createFilename();

    ImgSaver::ImageSaveStatus getFilenameAndFormat(ImageMetaInfo::ImageType type);

    QString m_saveDirectory;				// dir where the image should be saved
    QByteArray mLastFormat;
    KUrl mLastUrl;

    bool m_saveAskFormat;
    bool m_saveAskFilename;

    KUrl mSaveUrl;
    ImageFormat mSaveFormat;
    QString mSaveSubformat;
};


#endif							// IMGSAVER_H
