/***************************************************************************
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

#ifndef __IMG_SAVER_H__
#define __IMG_SAVER_H__
#include <qobject.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qmemarray.h>
#include <qstring.h>
#include <qimage.h>
#include <stdlib.h>
#include <qdialog.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlistbox.h>
#include <qmap.h>
#include <kdialogbase.h>
#include <kurl.h>


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
#define OP_FILE_GROUP      "Files"


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
   ISS_SAVE_CANCELED

} ImgSaveStat;

/**
 *  enum picType:
 *  Specifies the type of the image to save. This is important for
 *  getting the format.
 **/
typedef enum {
   PT_PREVIEW,
   PT_THUMBNAIL,
   PT_HICOLOR_IMAGE,
   PT_COLOR_IMAGE,
   PT_GRAY_IMAGE,
   PT_BW_IMAGE,
   PT_FINISHED
} picType;


class KookaImage;
/**
 *  Class FormatDialog:
 *  Asks the user for the image-Format and gives help for
 *  selecting it.
 **/

class FormatDialog:public KDialogBase
{
   Q_OBJECT
public:
   FormatDialog( QWidget *parent, const QString&, const char * );


   QString      getFormat( ) const;
   QCString      getSubFormat( ) const;
   QString      errorString( ImgSaveStat stat );

   bool         askForFormat( ) const
      { return( ! cbDontAsk->isChecked()); }

public slots:
    void        setSelectedFormat( QString );


protected slots:
   void 	showHelp( const QString& item );

private:

   void		check_subformat( const QString & format );
   void 	buildHelp( void );
   void 	readConfig( void );

   QMap<QString, QString> 	format_help;
   QComboBox   	*cb_subf;
   QListBox    	*lb_format;
   QLabel      	*l_help;
   QLabel	*l2;
   QCheckBox    *cbRemember;
   QCheckBox    *cbDontAsk;
};

/**
 *  Class ImgSaver:
 *  The main class of this module. It manages all saving of images
 *  in kooka
 *  It asks the user for the img-format if desired, creates thumbnails
 *  and cares for database entries (later ;)
 **/

class ImgSaver:public QObject {
   Q_OBJECT
public:
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
   QString     lastFilename() const { return( last_file ); }
   KURL        lastFileUrl() const { return( KURL(last_file )); }
   /**
    *  returns the image format of the last saved image.
    */
   QCString    lastSaveFormat( void ) const { return( last_format ); }

   QString     getFormatForType( picType ) const;
   void        storeFormatForType( picType, QString, bool );
   bool        isRememberedFormat( picType type, QString format ) const;

   /* static function that exports a file */
   static bool    copyImage( const KURL& fromUrl, const KURL& toUrl, QWidget *overWidget=0 );
   static bool    renameImage( const KURL& fromUrl, KURL& toUrl, bool askExt=false, QWidget *overWidget=0 );
   static QString tempSaveImage( KookaImage *img, const QString& format, int colors = -1 );

   /* static function that returns the extension of an url */
   static QString extension( const KURL& );

public slots:
   ImgSaveStat saveImage( QImage *image );
   ImgSaveStat saveImage( QImage *image, const KURL& filename, const QString& imgFormat );

private:
   QString 	picTypeAsString( picType type ) const;
   QString      findFormat( picType type );
   QString      findSubFormat( QString format );
   void		createDir( const QString& );

   ImgSaveStat  save( QImage *image, const QString &filename, const QString &format,
		     const QString &subformat );
   QString      createFilename( QString format );
   void	        readConfig( void );
   QString      startFormatDialog( picType );

   // QStrList    all_formats;
   QString     	directory;    // dir where the image should be saved
   QString     	last_file;
   QCString   	subformat;
   QCString    	last_format;
   bool	       	ask_for_format;

   // QDict<QString> formats;
};

#endif
