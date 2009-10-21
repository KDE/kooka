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



// TODO: enum into class
/**
 *  enum ImgSaveStat:
 *  Errorflags for the save. These enums are returned by the
 *  all image save operations and the calling object my display
 *  a human readable Error-Message on this information
 **/
typedef enum {
   ISS_OK,         /* Image save OK      */
   ISS_ERR_PERM,   /* permission Error   */
   ISS_ERR_FILENAME,   /* bad filename       */
   ISS_ERR_NO_SPACE,   /* no space on device */
   ISS_ERR_FORMAT_NO_WRITE, /* Image format can not be written */
   ISS_ERR_UNKNOWN,
   ISS_ERR_PARAM,       /* Parameter wrong */
   ISS_ERR_PROTOCOL,
   ISS_ERR_MKDIR,
   ISS_SAVE_CANCELED

} ImgSaveStat;

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
     *  enum ImageType:
     *  Specifies the type of the image to save. This is important for
     *  getting the format.
     **/
    enum ImageType
    {
        ImgNone       = 0x00,
        ImgPreview    = 0x01,
        ImgThumbnail  = 0x02,
        ImgHicolor    = 0x04,
        ImgColor      = 0x08,
        ImgGray       = 0x10,
        ImgBW         = 0x20
    };

	/**
	 *  constructor of the image-saver object.
	 *  name is the name of a subdirectory of the save directory,
	 *  which can be given in dir. If no dir is given, an
	 *  dir ~/.ksane is created.
	 *  @param dir  Name of the save root directory
	 *  @param name Name of a subdirectory in the saveroot.
	 **/
    ImgSaver(const KUrl &dir = KUrl());

    QString     errorString( ImgSaveStat );

    /**
     *  returns the name of the last file that was saved by ImgSaver.
     */
    KUrl        lastURL() const { return (last_url); }

    /**
     *  returns the image format of the last saved image.
     */
    QByteArray    lastSaveFormat( void ) const { return (last_format); }

    /* static functions used by the gallery operations */
    static bool copyImage( const KUrl& fromUrl, const KUrl& toUrl, QWidget *overWidget=0 );
    static bool renameImage( const KUrl& fromUrl, const KUrl& toUrl, bool askExt=false, QWidget *overWidget=0 );
    static QString tempSaveImage(const KookaImage *img, const QString &format, int colors = -1);

    static bool isRememberedFormat(ImgSaver::ImageType type,const QString &format);
    static QString picTypeAsString(ImgSaver::ImageType type);

//public slots:
    ImgSaveStat saveImage( const QImage *image );
    ImgSaveStat saveImage( const QImage *image, const KUrl &url, const QString& imgFormat );

private:
    static QString findFormat(ImgSaver::ImageType type);
    static QString findSubFormat(const QString &format);

    static QString getFormatForType(ImgSaver::ImageType);
    static void storeFormatForType(ImgSaver::ImageType,const QString &format);

    ImgSaveStat save(const QImage *image,const KUrl &url,
                     const QString &format,const QString &subformat = QString::null);

    static void createDir(const QString &dir);
    QString createFilename();

    /* static function that returns the extension of an url */
    static QString extension(const KUrl &url);

    QString m_saveDirectory;				// dir where the image should be saved
    QByteArray last_format;
    KUrl last_url;

    bool m_saveAskFormat;
    bool m_saveAskFilename;
};


#endif							// IMGSAVER_H
