/***************************************************************************
                          img_saver.cpp  -  description
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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>


#include <kglobal.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kimageio.h>

#include <qdir.h>
#include <qlayout.h>
#include <qfileinfo.h>
#include <qmessagebox.h>
#include <qvbox.h>
#include <qbuttongroup.h>

#include "resource.h"
#include "img_saver.h"


#define OP_FILE_ASK_FORMAT "AskForSaveFormat"
#define OP_FORMAT_HICOLOR  "HiColorSaveFormat"
#define OP_FORMAT_COLOR    "ColorSaveFormat"
#define OP_FORMAT_GRAY     "GraySaveFormat"
#define OP_FORMAT_BW       "BWSaveFormat"
#define OP_PREVIEW_GROUP   "ScanPreview"
#define OP_PREVIEW_FILE     "PreviewFile"
#define OP_FILE_GROUP      "Files"

FormatDialog::FormatDialog( QWidget *parent, const char *name,  
			    QStrList *formats = 0)
   :KDialogBase( parent, "FormDialog", true,
                 /* Tabbed,*/ I18N( "Kooka save Assistant" ),
		 Ok|Cancel, Ok, parent,  name )

{
   if( ! formats )
   {
      // BIG ERROR - something wrong with ImageIO
      return;
   }
   buildHelp();
   // readConfig();
   // QFrame *page = addPage( QString( "Save the image") );
   QFrame *page = new QFrame( this );
   page->setFrameStyle( QFrame::Box | QFrame::Sunken );
   CHECK_PTR( page );
   setMainWidget( page );
   
   QVBoxLayout *bigdad = new QVBoxLayout( page, marginHint(), spacingHint());
   CHECK_PTR(bigdad);

   // some nice words
   QLabel *l0 = new QLabel( page );
   CHECK_PTR(l0);
   l0->setText( I18N( "<B>Save Assistant</B><P>Select an image format to save the scanned image." ));
   bigdad->addWidget( l0 );

   QFrame *f1 = new QFrame( page );
   f1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
   bigdad->addWidget( f1 );
   
   // Layout-Boxes
   // QHBoxLayout *hl1= new QHBoxLayout( );  // Caption
   QHBoxLayout *lhBigMiddle = new QHBoxLayout( spacingHint() );  // Big middle
   CHECK_PTR(lhBigMiddle);
   bigdad->addLayout( lhBigMiddle );
   QVBoxLayout *lvFormatSel = new QVBoxLayout( spacingHint() );  // Selection List
   CHECK_PTR(lvFormatSel);
   lhBigMiddle->addLayout( lvFormatSel );
   
   // Insert Scrolled List for formats
   QLabel *l1 = new QLabel( page );
   CHECK_PTR(l1);
   l1->setText( I18N( "Available image formats:" ));
   
   lb_format = new QListBox( page, "ListBoxFormats" );
   CHECK_PTR(lb_format);
   lb_format->insertStrList( formats );
   connect( lb_format, SIGNAL( highlighted(const QString&)),
	    SLOT( showHelp(const QString&)));
   
   // Insert label for helptext
   l_help = new QLabel( page );
   CHECK_PTR(l_help);
   l_help->setFrameStyle( QFrame::Panel|QFrame::Sunken );
   l_help->setText( I18N("-No format selected-" ));
   l_help->setAlignment( AlignVCenter | AlignHCenter );

   // Insert Selbox for subformat
   l2 = new QLabel( page );
   CHECK_PTR(l2);
   l2->setText( I18N( "Select the image-sub-format" ));
   cb_subf = new QComboBox( page, "ComboSubFormat" );
   CHECK_PTR( cb_subf );

   // Checkbox to store setting
   
#if 0 
   QFrame *vf = new QFrame(page);
   vf->setFrameStyle( QFrame::Panel|QFrame::Sunken );
#endif
   cbRemember = new QCheckBox(I18N("Remember this format for this image type"),
			      page );

   QCheckBox *cbDontAsk  = new QCheckBox(I18N("Dont ask again for that format"),
			      page );

   // cbRemember->setFrameStyle( QFrame::Panel|QFrame::Sunken );
   CHECK_PTR( cbRemember );
   
   // bigdad->addWidget( l_caption, 1 );
   lvFormatSel->addWidget( l1, 1 );
   lvFormatSel->addWidget( lb_format, 6 );
   lvFormatSel->addWidget( l2, 1 );
   lvFormatSel->addWidget( cb_subf, 1 );

   lhBigMiddle->addWidget( l_help, 2 );
   //bigdad->addStretch(1);
   bigdad->addWidget( cbRemember , 2 );
   bigdad->addWidget( cbDontAsk , 2 );

   // bigdad->addWidget( cbRemember,1 );
   bigdad->activate();
   
}

void FormatDialog::showHelp( const QString& item )
{
   const char *helptxt = format_help[ item ];

   if( helptxt ) {
      // Set the hint 
      l_help->setText( helptxt );

      // and check subformats
      check_subformat( helptxt );
   } else {
      l_help->setText( I18N("-no hint available-" ));
   }      
}

void FormatDialog::check_subformat( const char *format )
{
   // not yet implemented
   debug( "This is format in check_subformat: %s", format );
   cb_subf->setEnabled( false );
   // l2 = Label "select subformat" ->bad name :-|
   l2->setEnabled( false );
}

void FormatDialog::setSelectedFormat( QString fo )
{
   QListBoxItem *item = lb_format->findItem( fo );

   if( item )
   {
      // Select it.
      lb_format->setSelected( lb_format->index(item), true );
   }
}


QString FormatDialog::getFormat( void ) const
{
   int item = lb_format->currentItem();

   if( item > -1 )
   {
      const QString f = lb_format->text( item );
      return( f );
   }
   return( "BMP" );
}


QString FormatDialog::getSubFormat( void ) const
{
   // Not yet...
   return( "" );
}

#include "formathelp.h"
void FormatDialog::buildHelp( void )
{
   format_help.insert( "BMP", HELP_BMP );
   format_help.insert( "PNM", HELP_PNM );
   format_help.insert( "JPEG", HELP_JPG );
   format_help.insert( "JPG", HELP_JPG );

   format_help.insert( "EPS", HELP_EPS );
}


bool FormatDialog::rememberFormat( void ) const
{
   bool result = false;
   
   if( cbRemember )
   {
      return( cbRemember->isChecked());
   }

   return( result );
}


/* ********************************************************************** */

ImgSaver::ImgSaver(  QWidget *parent, const char *name, const char *dir )
   : QObject( parent, name )
{

   QDir home = QDir::home();

   QDir path( name );

   all_formats = QImage::outputFormats();
   if( ! dir )
   {
      if( path.isRelative() )
	 directory = home.absPath()+ "/.kscan/";
      else
	 directory = "";  /* absolute Path given in name  ! */
   }
   else
   {
      directory = dir;
      if( directory.right( 1 ) != "/" )
	 directory += "/";
      createDir( directory );
   }

   /* Check directories */
   if( name )
   {
      directory += QString( name );
      createDir( directory );

      if( directory.right( 1 ) != "/" )
	 directory += "/";
   }		
	
   readConfig();
   
}

/* Needs a full qualified directory name */
void ImgSaver::createDir( QString dir )
{
   QFileInfo fi( dir );
 	
   if( ! fi.exists() )
   {
      debug( "Wrn: Directory does not exist -> try to create  !" );
      if( mkdir( dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
      {
	 QMessageBox::warning( 0,"Warning","The directory \n" + dir +
			       "\nto save images does not exist\nand could not be created !\n"
			       "Please check the permissions.", QMessageBox::Warning,
			       QMessageBox::Ok | QMessageBox::Default );
				
      }
   }
   if( ! fi.isWritable() )
   {
      QMessageBox::warning( 0,"Warning","The directory \n" + dir +
			    "\nto save images is not writeable.\n"
			    "Please check the permissions.", QMessageBox::Warning,
			    QMessageBox::Ok | QMessageBox::Default );
				
   }
}

/**
 *   This function asks the user for a filename or creates
 *   one by itself, depending on the settings
 **/
ImgSaveStat ImgSaver::saveImage( QImage *image )
{
   ImgSaveStat stat;
   picType imgType;
   
   if( !image ) return( ISS_ERR_PARAM );

   /* Find out what kind of image it is  */
   if( image->depth() > 8 )
   {
      imgType = PT_HICOLOR_IMAGE;
   }
   else
   {
      if( image->depth() == 1 )
      {
	 imgType = PT_BW_IMAGE;
      }
      else
      {
	 imgType  = PT_COLOR_IMAGE;
	 if( image->allGray() )
	 {
	    imgType = PT_GRAY_IMAGE;
	 }
      }
   }

   
   QString format = findFormat( imgType );
   QString subformat = findSubFormat( format );
   // Call save-Function with this params

   if( format.isEmpty() )
   {
    	debug( "Save canceled by user -> no save !" );
    	return( ISS_SAVE_CANCELED );
   }
   stat = save( image, directory + createFilename( format ),
		format, subformat );

   return( stat );

}

/**
 *  This member creates a filename for the image to save.
 *  This is done by numbering all existing files and adding
 *  one
 **/
QString ImgSaver::createFilename( QString format )
{
   if( format.isNull() || format.isEmpty() ) return( 0 );
   
   QString s = "kscan_*." + format.lower();
   QDir files( directory, s );
   char name[20];
   int c = 1;
   
   sprintf( name, "kscan_%04x.%s", c, (const char*) format.lower() );

   while( files.exists( name, false ) ) {
      sprintf( name, "kscan_%04x.%s", ++c,  (const char*)format.lower() );
   }
   
   return( name );
}

/**
 *   This function gets a filename from the parent.
 **/
ImgSaveStat ImgSaver::saveImage( QImage *image, QString filename )
{
   QString format = QImage::imageFormat( filename );
   if( format == "" ) 
      format = "BMP";
   
   QFileInfo fi( filename );

   if( fi.isRelative() )
      filename = directory + filename;

   return( save( image, filename, format, "" ) );
}



 
ImgSaveStat ImgSaver::savePreview( QImage *image )
{
   if( !image ||  image->isNull() )
      return( ISS_ERR_PARAM );
   
   QString format = findFormat( PT_PREVIEW );

   // Previewfile always comes absolute
      
   ImgSaveStat stat = save( image, previewfile, format, "" );

#if 0
   if( stat == ISS_OK )
   {
      KConfig *konf = kapp->getConfig();
      konf->setGroup( OP_FILE_GROUP );

      konf->writeEntry( OP_PREVIEW_FILE,   previewfile );
      konf->writeEntry( OP_PREVIEW_FORMAT, format );
   }
#endif
   return( stat );
}

/*
 * findFormat does all the stuff with the dialog.
 */
QString ImgSaver::findFormat( picType type )
{
   // Preview always as bitmap
   if( type == PT_PREVIEW )
      return( "BMP" );

   if( type == PT_THUMBNAIL )
      return( "BMP" );
   
   // real images 
   if( ask_for_format )
   {
      FormatDialog fd( 0, "FormatDialog", &all_formats );

      // set default values
      QString defFormat = getFormatForType( type );
      fd.setSelectedFormat( defFormat );
      
      if( fd.exec() )
      {
	 QString format = fd.getFormat();
	 debug( "Storing to format <%s>", (const char*) format );
	 if( fd.rememberFormat() )
	 {
	    storeFormatForType( type, format );
	 }
	 subformat = fd.getSubFormat();

	 return( format );
      }
   }
   return( "" );
   
}

QString ImgSaver::getFormatForType( picType type ) const
{
   KConfig *konf = KGlobal::config ();
   CHECK_PTR( konf );
   konf->setGroup( OP_FILE_GROUP );

   QString f;
   
   switch( type )
   {
      case PT_COLOR_IMAGE:
	 f = konf->readEntry( OP_FORMAT_COLOR, "BMP" );
	 break;
      case PT_GRAY_IMAGE:
	 f = konf->readEntry( OP_FORMAT_GRAY, "BMP" );
	 break;
      case PT_BW_IMAGE:
	 f = konf->readEntry( OP_FORMAT_BW, "BMP" );
	 break;
      case PT_HICOLOR_IMAGE:
	 f = konf->readEntry( OP_FORMAT_HICOLOR, "BMP" );
	 break;
      default:
	 f = "BMP";
	 break;
   }
   return( f );
}


void ImgSaver::storeFormatForType( picType type, QString format )
{
   KConfig *konf = KGlobal::config ();
   CHECK_PTR( konf );
   konf->setGroup( OP_FILE_GROUP );

   switch( type )
   {
      case PT_COLOR_IMAGE:
	 konf->writeEntry( OP_FORMAT_COLOR, format );
	 break;
      case PT_GRAY_IMAGE:
	 konf->writeEntry( OP_FORMAT_GRAY, format  );
	 break;
      case PT_BW_IMAGE:
	 konf->writeEntry( OP_FORMAT_BW, format );
	 break;
      case PT_HICOLOR_IMAGE:
	 konf->writeEntry( OP_FORMAT_HICOLOR, format );
	 break;
      default:
	 debug( "Wrong Type  - cant store format setting" );
	 break;
   }
}


QString ImgSaver::findSubFormat( QString format )
{
   debug( "Searching Subformat for %s", (const char*) format );
   return( subformat );
   
}

/**
   private save() does the work to save the image.
**/
ImgSaveStat ImgSaver::save( QImage *image, QString filename,
			    const char *format,
			    const char *subformat )
{

   bool result = false;
   
   if( ! format || !image ) return( ISS_ERR_PARAM );
   
   if( image )
   {
      // remember the last processed file - only the filename - no path
      QFileInfo fi( filename );

      if( fi.exists() && !fi.isWritable() )
      {
	 debug( "Cant write to file <%s>, cant save !", (const char*) filename );
	 result = false;
	 return( ISS_ERR_PERM );
      }

      /* Check the format, is it writable ? */
      if( ! KImageIO::canWrite( format ) )
      {
	 debug( "Cant write format <%s>", (const char*) format );
	 result = false;
	 return( ISS_ERR_FORMAT_NO_WRITE );
      }
      
      debug( "ImgSaver: saving image to <%s> as <%s/%s>", (const char*) filename,
	     format, subformat );

      result = image->save( filename, format );

      
      last_file = fi.fileName();
   }

   if( result )
      return( ISS_OK );
   else {
      last_file = "";
      return( ISS_ERR_UNKNOWN );
   }
	 
}


void ImgSaver::readConfig( void )
{

   KConfig *konf = KGlobal::config ();
   CHECK_PTR( konf );
   konf->setGroup( OP_FILE_GROUP );
   ask_for_format = konf->readBoolEntry( OP_FILE_ASK_FORMAT, true );

   konf->setGroup( OP_PREVIEW_GROUP );

   /* This is a bit ugly, but will be fixed later */
   /* The save root is defined to $HOME/.kscan in scanpackager, this should be
    * handled in at a central point. TODO ! */
   QDir home = QDir::home();
   previewfile = home.absPath() + "/.kscan/preview.bmp";
   
   previewfile = konf->readPathEntry( OP_PREVIEW_FILE, previewfile );

}





QString ImgSaver::errorString( ImgSaveStat stat )
{
   QString re;
   
   switch( stat ) {
      case ISS_OK:           re = I18N( " Image save OK      " ); break;
      case ISS_ERR_PERM:     re = I18N( " permission Error   " ); break;
      case ISS_ERR_FILENAME: re = I18N( " bad filename       " ); break;
      case ISS_ERR_NO_SPACE: re = I18N( " no space on device " ); break;
      case ISS_ERR_UNKNOWN:  re = I18N( " unknown error      " ); break;
      case ISS_ERR_PARAM:    re = I18N( " Parameter wrong    " ); break;
      default: re = "";
   }
   return( re );

}

