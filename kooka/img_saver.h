/***************************************************************************
                          img_saver.h  -  description                              
                             -------------------                                         
    begin                : Mon Dec 27 1999                                           
    copyright            : (C) 1999 by Klaas Freitag                         
    email                :                                      
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
#include <qimage.h>
#include <stdlib.h>
#include <qpushbutton.h>
#include <qdialog.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlistbox.h>
#include <qdict.h>

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
   ISS_ERR_UNKNOWN,    
   ISS_ERR_PARAM,       /* Parameter wrong */
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
   PT_IMAGE
} picType;


/**
 *  Class FormatDialog:
 *  Asks the user for the image-Format and gives help for
 *  selecting it.
 **/
class FormatDialog:public QDialog {
   Q_OBJECT
public:
   FormatDialog( QWidget *parent, const char *name, QStrList *formats );


   QString      	getFormat( void );
   QString      	getSubFormat( void );
   QString      	errorString( ImgSaveStat stat );
 protected slots:
   void 			 	showHelp( const QString& item );
 
private:
   void			 	check_subformat( const char *format );
   void 				buildHelp( void );
   void 			 	readConfig( void );

   QDict<char> 	format_help;
   QComboBox   	*cb_subf;
   QListBox    	*lb_format;
   QLabel      	*l_help;
   QLabel			*l2;
   QButton			*b_ok, *b_cancel;
   bool				ask_for_format;

};

/**
 *  Class ImgSaver:
 *  The main class of this module. It manages all saving of images
 *  in kgallery.
 *  It asks the user for the img-format if desired, creates thumbnails
 *  and cares for database entries (later ;)
 **/

class ImgSaver:public QObject {
   Q_OBJECT
public:
	/**
	 *  constructor of the image-saver object.
	 *  name is the name of a subdirectory of the save directory,
	 *  which can be given in dir. If no dir is given, a unvisible
	 *  dir ~/.ksane is created.
	 *  @param dir  Name of the save root directory
	 *  @param name Name of a subdirectory in the saveroot.
	 **/
   ImgSaver( QWidget *parent, const char *name=0, const char *dir=0 );

   QString     errorString( ImgSaveStat );
   QString     lastFilename(void) { return( last_file ); };

public slots:
   ImgSaveStat saveImage( QImage *image );
   ImgSaveStat saveImage( QImage *image, QString filename );
   
   ImgSaveStat savePreview( QImage *image );
   
private:
   QString     findFormat( picType type );
   QString     findSubFormat( QString format );
   void		   createDir( QString );
   ImgSaveStat save( QImage *image, QString filename, const char *format,
		     const char *subformat );
   QString     createFilename( QString format );
   void	       readConfig( void );
   
   QStrList    all_formats;
   QString     previewfile;
   QString     subformat;     
   QString     directory;    // dir where the image should be saved
   QString     last_file;
   bool	       ask_for_format;
   
};

#endif
