/***************************************************************************
                          img_saver.h  -  description                      
                             -------------------           
    begin                : Mon Dec 27 1999                               
    copyright            : (C) 1999 by Klaas Freitag                     
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#ifndef __IMG_SAVER_H__
#define __IMG_SAVER_H__
#include <qobject.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qarray.h>
#include <qstring.h>
#include <qimage.h>
#include <stdlib.h>
#include <qpushbutton.h>
#include <qdialog.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlistbox.h>
#include <qmap.h>
#include <kdialogbase.h>
#include <kurl.h>


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


/**
 *  Class FormatDialog:
 *  Asks the user for the image-Format and gives help for
 *  selecting it.
 **/

class FormatDialog:public KDialogBase
{
   Q_OBJECT
public:
   FormatDialog( QWidget *parent, const char * );


   QString      getFormat( void ) const;
   QCString      getSubFormat( void ) const;
   QString      errorString( ImgSaveStat stat );

   bool         rememberFormat( void ) const;

   
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
   bool		ask_for_format;
   QCheckBox    *cbRemember;

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
   QString     lastFilename(void) const { return( last_file ); }
   /**
    *  returns the image format of the last saved image.
    */
   QCString    lastSaveFormat( void ) const { return( last_format ); }
   
   QString     getFormatForType( picType ) const;
   void        storeFormatForType( picType, QString );


   /**
    * Static function that returns the kooka base dir.
    */
   static QString kookaImgRoot( void );

#if 0
   /**
    * Static function that returns the relative path to the kooka imgage root
    */
   static QString relativeToImgRoot( QString );
#endif
   
public slots:
   ImgSaveStat saveImage( QImage *image );
   ImgSaveStat saveImage( QImage *image, QString filename, QString imgFormat );
   
   ImgSaveStat savePreview( QImage *image );
   
private:
   QString      findFormat( picType type );
   QString      findSubFormat( QString format );
   void		createDir( const QString& );
   ImgSaveStat  save( QImage *image, const QString &filename, const QString &format,
		     const QString &subformat );
   QString      createFilename( QString format );
   void	        readConfig( void );
   QString      startFormatDialog( picType );
   
   // QStrList    all_formats;
   QString      previewfile;
   QString     	directory;    // dir where the image should be saved
   QString     	last_file;
   QCString   	subformat;     
   QCString    	last_format;
   bool	       	ask_for_format;

   // QDict<QString> formats;
};

#endif
