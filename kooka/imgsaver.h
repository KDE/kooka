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

#ifndef __IMGSAVER_H__
#define __IMGSAVER_H__

#include <qobject.h>

#include <kurl.h>


#define OP_FILE_GROUP      "Files"
#define OP_FILE_ASK_FORMAT "AskForSaveFormat"
#define OP_ASK_FILENAME    "AskForFilename"
#define OP_FORMAT_HICOLOR  "HiColorSaveFormat"
#define OP_FORMAT_COLOR    "ColorSaveFormat"
#define OP_FORMAT_GRAY     "GraySaveFormat"
#define OP_FORMAT_BW       "BWSaveFormat"
#define OP_FORMAT_THUMBNAIL "ThumbnailFormat"
#define OP_PREVIEW_GROUP   "ScanPreview"
#define OP_PREVIEW_FILE    "PreviewFile"
#define OP_PREVIEW_FORMAT  "PreviewFormat"
#define OP_ONLY_REC_FMT    "OnlyRecommended"

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

class ImgSaver : public QObject
{
    Q_OBJECT

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
   ImgSaver( QWidget *parent, const KURL );
   ImgSaver( QWidget *parent );

   QString     errorString( ImgSaveStat );
   /**
    *  returns the name of the last file that was saved by ImgSaver.
    */
   KURL        lastURL() const { return( last_url ); }
   /**
    *  returns the image format of the last saved image.
    */
   QCString    lastSaveFormat( void ) const { return( last_format ); }

   QString     getFormatForType( ImgSaver::ImageType ) const;
   void        storeFormatForType( ImgSaver::ImageType, QString, bool );
   bool        isRememberedFormat( ImgSaver::ImageType type, QString format ) const;

   /* static function that exports a file */
   static bool    copyImage( const KURL& fromUrl, const KURL& toUrl, QWidget *overWidget=0 );
   static bool    renameImage( const KURL& fromUrl, const KURL& toUrl, bool askExt=false, QWidget *overWidget=0 );
   static QString tempSaveImage( KookaImage *img, const QString& format, int colors = -1 );

   /* static function that returns the extension of an url */
   static QString extension( const KURL& );

   static QString picTypeAsString(ImgSaver::ImageType type);

public slots:
   ImgSaveStat saveImage( const QImage *image );
   ImgSaveStat saveImage( const QImage *image, const KURL &url, const QString& imgFormat );

private:
   QString      findFormat( ImgSaver::ImageType type );
   QString      findSubFormat( QString format );
   void		createDir( const QString& );

   ImgSaveStat  save( const QImage *image, const KURL &url, const QString &format,
		     const QString &subformat );
   QString      createFilename( QString format );
   void	        readConfig( void );
   QString      startFormatDialog( ImgSaver::ImageType );

   QString     	directory;    // dir where the image should be saved

   QCString   	subformat;
   QCString    	last_format;
    KURL last_url;
   bool	       	ask_for_format;
};

#endif