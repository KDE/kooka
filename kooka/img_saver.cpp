/***************************************************************************
                          img_saver.cpp  -  description                              
                             -------------------                                         
    begin                : Mon Dec 27 1999                                           
    copyright            : (C) 1999 by Klaas Freitag                         
    email                : Klaas.Freitag@gmx.de                                     
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


#ifdef HAVE_KDE
#include <kconfig.h>
#include <kscan.h>
#endif
#include <qdir.h>
#include <qlayout.h>
#include <qfileinfo.h>
#include <qmessagebox.h>

#include "img_saver.h"

#ifndef HAVE_KDE
#define i18n(x) (x)
#endif

FormatDialog::FormatDialog( QWidget *parent, const char *name,  
			    QStrList *formats = 0)
   : QDialog( parent, name, true )
{
   if( ! formats ) {
      // BIG ERROR - something wrong with ImageIO
      return;
   }
   buildHelp();
   // readConfig();
   setCaption( i18n( "Save Assistent" ));
   // Layout-Boxes
   QVBoxLayout *bigdad = new QVBoxLayout( this, 2 );
   // QHBoxLayout *hl1= new QHBoxLayout( );  // Caption
   QHBoxLayout *hl2= new QHBoxLayout( 10 );  // Big middle
   QHBoxLayout *hl3= new QHBoxLayout( 10 );  // Buttons
   QVBoxLayout *vl1= new QVBoxLayout( 10 );  // Selection List
   QVBoxLayout *vl2= new QVBoxLayout( 10 );  // Help and subformat

   
   // Insert Scrolled List for formats
   QLabel *l1 = new QLabel( this );
   l1->setText( i18n( "Available image formats:" ));
   // QSize s = l1->sizeHint();
   // l1->setFixedHeight( s.height());

   lb_format = new QListBox( this, "ListBoxFormats" );
   lb_format->insertStrList( formats );
   connect( lb_format, SIGNAL( highlighted(const QString&)),
	    SLOT( showHelp(const QString&)));
   
   // Insert label for helptext
   l_help = new QLabel( this );
   l_help->setFrameStyle( QFrame::Panel ); //| QFrame::Sunken | WordBreak);
   l_help->setText( i18n("-No format selected-" ));
   l_help->setAlignment( AlignTop | AlignCenter );

   // Insert Selbox for subformat
   l2 = new QLabel( this );
   l2->setText( i18n( "Select the image-sub-format" ));
   // s = l2->sizeHint();
   // l2->setFixedHeight( s.height());
   cb_subf = new QComboBox( this, "ComboSubFormat" );

   // OK- and Cancelbutton
   b_ok     = new QPushButton( i18n("OK"), this, "ButtOK" );
   connect( b_ok, SIGNAL(clicked()), SLOT(accept()) );
   // b_ok->setDefault( true );
   b_cancel = new QPushButton( i18n("Cancel"), this, "ButtCancel" );
   connect( b_cancel, SIGNAL(clicked()), SLOT(reject()) );
   // Connect Doubleclick on List to OK
   // connect( lb_format, SIGNAL(
   // Layout-Hirarchie
   // QLabel *l_caption = new QLabel( i18n("Save Assistent"), this );
   // l_caption->setFrameStyle( QFrame::Panel  | QFrame::Sunken );

   // bigdad->addWidget( l_caption, 1 );
   bigdad->addLayout( hl2, 6 );
   bigdad->addLayout( hl3, 1 );

   hl2->addLayout( vl1,4 ); // Layout for Selection List
   hl2->addLayout( vl2,6 ); // Layout for Help and Subformat

   // s = cb_subf->sizeHint();
   // cb_subf->setFixedHeight( s.height());
   // vl1->addStretch( 1 );
   vl1->addWidget( l1, 1 );
   vl1->addWidget( lb_format, 10  );
   vl1->addStretch( 1 );

   // Right gap: Help and subformat
   vl2->addStretch( 1 );
   vl2->addWidget( l_help ,8);
   vl2->addStretch( 1 );
   vl2->addWidget( l2 );
   vl2->addWidget( cb_subf, 3 );
   vl2->addStretch( 2 );

   hl3->addStretch( 1 );
   hl3->addWidget( b_cancel,5 );
   hl3->addStretch( 1 );
   hl3->addWidget( b_ok, 5);
   hl3->addStretch( 1 );

   // activate Layouts
   bigdad->activate();
   // resize( QSize( 394, 292 ));
   
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
      l_help->setText( i18n("-no hint available-" ));
   }      
}

void FormatDialog::check_subformat( const char *format )
{
   // not yet implemented
   cb_subf->setEnabled( false );
   // l2 = Label "select subformat" ->bad name :-|
   l2->setEnabled( false );
}

QString FormatDialog::getFormat( void )
{
   int item = lb_format->currentItem();

   if( item > -1 ){
      const char *f = lb_format->text( item );
      if( f )
	 return( f );
   }
   return( "BMP" );
}


QString FormatDialog::getSubFormat( void )
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



/* ********************************************************************** */

ImgSaver::ImgSaver(  QWidget *parent, const char *name=0, const char *dir=0 )
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
   
   QString format = findFormat( PT_IMAGE );
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

   debug( "Checking for file %s", name );
   
   while( files.exists( name, false ) ) {
      sprintf( name, "kscan_%04x.%s", ++c,  (const char*)format.lower() );
      debug( "Checking for file %s", name );
   }
   
   return( name );
}

/**
 *   This function gets a filename from the parent.
 **/
ImgSaveStat ImgSaver::saveImage( QImage *image, QString filename )
{
   const char *format = "BMP";
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

#ifdef HAVE_KDE
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

QString ImgSaver::findFormat( picType type )
{
   // Preview always as bitmap
   if( type == PT_PREVIEW )
      return( "BMP" );

   if( type == PT_THUMBNAIL )
      return( "BMP" );
   
   // real images 
   if( ask_for_format ) {

      FormatDialog fd( 0, "FormatDialog", &all_formats );

      if( fd.exec() ) {
	 		QString format = fd.getFormat();
	 		subformat = fd.getSubFormat();

	 		return( format );
      }
   }
   return( "" );
   
}


QString ImgSaver::findSubFormat( QString format )
{
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
   
   if( image ) {
      debug( "ImgSaver: saving image to <%s> as <%s>", (const char*) filename,
	     (const char*) format );
      result = image->save( filename, format );

      // remember the last processed file - only the filename - no path
      QFileInfo fi( filename );
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

#ifdef HAVE_KDE
   KConfig *konf = kapp->getConfig();
   konf->setGroup( OP_FILE_GROUP );

   ask_for_format = konf->readBoolEntry( OP_FILE_ASK_FORMAT, true );

   konf->setGroup( OP_PREVIEW_GROUP );
   previewfile = konf->readEntry( OP_PREVIEW_FILE, "~/.ksane_preview" );
#else
	previewfile = directory + "preview.bmp";

	ask_for_format = true;
#endif	
}

QString ImgSaver::errorString( ImgSaveStat stat )
{
   QString re;
   
   switch( stat ) {
      case ISS_OK:           re = i18n( " Image save OK      " ); break;
      case ISS_ERR_PERM:     re = i18n( " permission Error   " ); break;
      case ISS_ERR_FILENAME: re = i18n( " bad filename       " ); break;
      case ISS_ERR_NO_SPACE: re = i18n( " no space on device " ); break;
      case ISS_ERR_UNKNOWN:  re = i18n( " unknown error      " ); break;
      case ISS_ERR_PARAM:    re = i18n( " Parameter wrong    " ); break;
      default: re = "";
   }
   return( re );

}

