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


#define OP_SAVER_GROUP         "Files"

#define OP_SAVER_ASK_FORMAT    "AskForSaveFormat"
#define OP_SAVER_ASK_FILENAME  "AskForFilename"
#define OP_SAVER_REC_FMT       "OnlyRecommended"

#define OP_FORMAT_HICOLOR      "HiColorSaveFormat"
#define OP_FORMAT_COLOR        "ColorSaveFormat"
#define OP_FORMAT_GRAY         "GraySaveFormat"
#define OP_FORMAT_BW           "BWSaveFormat"
#define OP_FORMAT_THUMBNAIL    "ThumbnailFormat"
#define OP_FORMAT_UNKNOWN      "UnknownFormat"


class KookaImage;


/**
 *  Class ImgSaver:
 *  The main class of this module. It manages all saving of images
 *  in kooka
 *  It asks the user for the img-format if desired, creates thumbnails
 *  and cares for database entries (later ;)
 **/

class ImgSaver
{

public:

    /**
     *  enum ImageSaveStatus:
     *  Errorflags for the save. These enums are returned by the
     *  all image save operations and the calling object may display
     *  a human readable Error-Message on this information.
     *  See errorString() for what the flags mean.
     **/
    enum ImageSaveStatus
    {
        SaveStatusOk,
        SaveStatusPermission,
        SaveStatusBadFilename,
        SaveStatusNoSpace,
        SaveStatusFormatNoWrite,
        SaveStatusUnknown,
        SaveStatusParam,
        SaveStatusProtocol,
        SaveStatusMkdir,
        SaveStatusCanceled
    };

    ImgSaver(const KUrl &dir = KUrl());

    KUrl lastURL() const { return (mLastUrl); }
    QString errorString(ImgSaver::ImageSaveStatus status);

    /* static functions used by the gallery operations */
    static bool copyImage(const KUrl &fromUrl, const KUrl &toUrl, QWidget *overWidget = NULL);
    static bool renameImage(const KUrl &fromUrl, const KUrl &toUrl, bool askExt = false, QWidget *overWidget = NULL);
    static QString tempSaveImage(const KookaImage *img, const ImageFormat &format, int colors = -1);

    static bool isRememberedFormat(ImageMetaInfo::ImageType type, const ImageFormat &format);
    static QString picTypeAsString(ImageMetaInfo::ImageType type);

    ImgSaver::ImageSaveStatus setImageInfo(const ImageMetaInfo *info);
    ImgSaver::ImageSaveStatus saveImage(const QImage *image);
    ImgSaver::ImageSaveStatus saveImage(const QImage *image, const KUrl &url, const ImageFormat &format);

private:
    static ImageFormat findFormat(ImageMetaInfo::ImageType type);
    static QString findSubFormat(const ImageFormat &format);

    static ImageFormat getFormatForType(ImageMetaInfo::ImageType type);
    static void storeFormatForType(ImageMetaInfo::ImageType type, const ImageFormat &format);

    ImgSaver::ImageSaveStatus save(const QImage *image, const KUrl &url,
                                   const ImageFormat &format, const QString &subformat = QString::null);
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
