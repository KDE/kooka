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
#include <kseparator.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kio/jobclasses.h>
#include <kio/file.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>

#include <qdir.h>
#include <qlayout.h>
#include <qfileinfo.h>
#include <qimage.h>
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
#define OP_PREVIEW_FORMAT   "PreviewFormat"
#define OP_FILE_GROUP      "Files"

FormatDialog::FormatDialog( QWidget *parent, const char *name )
   :KDialogBase( parent, "FormDialog", true,
                 /* Tabbed,*/ i18n( "Kooka save assistant" ),
		 Ok|Cancel, Ok, parent,  name )

{
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
   l0->setText( i18n( "<B>Save Assistant</B><P>Select an image format to save the scanned image." ));
   bigdad->addWidget( l0 );

   KSeparator* sep = new KSeparator( KSeparator::HLine, page);
   bigdad->addWidget( sep );
   
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
   l1->setText( i18n( "Available image formats:" ));
   
   lb_format = new QListBox( page, "ListBoxFormats" );
   CHECK_PTR(lb_format);

#ifdef USE_KIMAGEIO
   QStringList fo = KImageIO::types();
#else
   QStringList fo = QImage::outputFormatList();
#endif
   kdDebug(28000) << "#### have " << fo.count() << " image types" << endl;
   lb_format->insertStringList( fo );
   connect( lb_format, SIGNAL( highlighted(const QString&)),
	    SLOT( showHelp(const QString&)));
   
   // Insert label for helptext
   l_help = new QLabel( page );
   CHECK_PTR(l_help);
   l_help->setFrameStyle( QFrame::Panel|QFrame::Sunken );
   l_help->setText( i18n("-No format selected-" ));
   l_help->setAlignment( AlignVCenter | AlignHCenter );

   // Insert Selbox for subformat
   l2 = new QLabel( page );
   CHECK_PTR(l2);
   l2->setText( i18n( "Select the image sub-format" ));
   cb_subf = new QComboBox( page, "ComboSubFormat" );
   CHECK_PTR( cb_subf );

   // Checkbox to store setting
   
#if 0 
   QFrame *vf = new QFrame(page);
   vf->setFrameStyle( QFrame::Panel|QFrame::Sunken );
#endif
   cbRemember = new QCheckBox(i18n("Remember this format for this image type"),
			      page );

   QCheckBox *cbDontAsk  = new QCheckBox(i18n("Don't ask again for this format"),
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
   QString helptxt = format_help[ item ];

   if( !helptxt.isEmpty() ) {
      // Set the hint
      l_help->setText( helptxt );

      // and check subformats
      check_subformat( helptxt );
   } else {
      l_help->setText( i18n("-no hint available-" ));
   }      
}

void FormatDialog::check_subformat( const QString & format )
{
   // not yet implemented
   kdDebug(28000) << "This is format in check_subformat: " << format << endl;
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


QCString FormatDialog::getSubFormat( void ) const
{
   // Not yet...
   return( "" );
}

#include "formathelp.h"
void FormatDialog::buildHelp( void )
{
   format_help.insert( QString::fromLatin1("BMP"), HELP_BMP );
   format_help.insert( QString::fromLatin1("PNM"), HELP_PNM );
   format_help.insert( QString::fromLatin1("JPEG"), HELP_JPG );
   format_help.insert( QString::fromLatin1("JPG"), HELP_JPG );
   format_help.insert( QString::fromLatin1("EPS"), HELP_EPS );
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

ImgSaver::ImgSaver(  QWidget *parent, const KURL dir_name )
   : QObject( parent )
{

   if( dir_name.isEmpty() || dir_name.protocol() != "file" )
   {
      kdDebug(28000) << "ImageServer initialised with wrong dir " << dir_name.url() << endl;
      directory = kookaImgRoot();
   }
   else
   {
      /* A path was given */
      if( dir_name.protocol() != "file"  )
      {
	 kdDebug(28000) << "ImgSaver: Can only save local image, sorry !" << endl;
      }
      else
      {
	 QString filename = dir_name.url();
	 directory  = filename.remove(0, 5);  // remove file: from beginning
	 if( directory.right(1) != "/" ) directory += "/";
      }
   }

   kdDebug(28000) << "ImageSaver uses dir <" << directory << endl;
   createDir( directory );
   readConfig();

   last_file = "";
   last_format ="";
   
}


ImgSaver::ImgSaver( QWidget *parent )
   :QObject( parent )
{
   directory = kookaImgRoot();
   createDir( directory );

   readConfig();

   last_file = "";
   last_format ="";
   
}


/* Needs a full qualified directory name */
void ImgSaver::createDir( const QString& dir )
{
   KURL url( dir );
 	
   if( ! KIO::NetAccess::exists(url) )
   {
      kdDebug(28000) << "Wrn: Directory <" << dir << "> does not exist -> try to create  !" << endl;
      // if( mkdir( QFile::encodeName( dir ), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
      if( KIO::mkdir( KURL(dir)))
      {
        KMessageBox::sorry(0, i18n("The directory\n%1\n does not exist and could not be created !\n"
                        "Please check the permissions.").arg(dir));
      }
   }
#if 0
   if( ! fi.isWritable() )
   {
        KMessageBox::sorry(0, i18n("The directory\n%1\n is not writeable.\nPlease check the permissions.")
                .arg(dir));
   }
#endif
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

   /* Find out what kind of image it is  */
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

   if( format.isEmpty() || format == "Skipped"  )
   {
    	kdDebug(28000) << "Save canceled by user -> no save !" << endl;
    	return( ISS_SAVE_CANCELED );
   }

   kdDebug(28000) << "saveImage: Directory is " << directory << endl;
   QString fi = directory + "/" + createFilename( format );
   kdDebug(28000) << "saveImage: saving file <" << fi << ">" << endl;
   stat = save( image, fi,
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
   
   sprintf( name, "kscan_%04x.%s", c, (format.lower()).latin1() );

   kdDebug(28000) << "CRASH WITH: " << name << endl;

   while( files.exists( name, false ) ) {
      sprintf( name, "kscan_%04x.%s", ++c, (format.lower()).latin1() );
   }
   
   return( name );
}

/**
 *   This function gets a filename from the parent.
 **/
ImgSaveStat ImgSaver::saveImage( QImage *image, QString filename, QString imgFormat )
{
    QString format = imgFormat;
    
    if( imgFormat.isNull() || imgFormat.isEmpty())
	QImage::imageFormat( filename );
    /* Check if the filename is local */
    KURL url( filename );
    if( url.protocol() != "file" )
    {
	kdDebug(29000) << "ImgSaver: Can only save local image, sorry !" << endl;
	return( ISS_ERR_PROTOCOL );
    }
    QString fname = filename.remove(0, 5);  // remove file: from beginning

    QFileInfo fi( fname );
    if( fi.isRelative() )
	filename = directory +"/"+ filename;
    
    kdDebug(28000) << "saveImage: Saving "<< filename << " in format " << format << endl;
    if( format == "" ) 
	format = "BMP";

    return( save( image, filename, format, "" ) );
}




 
ImgSaveStat ImgSaver::savePreview( QImage *image )
{
   if( !image ||  image->isNull() )
      return( ISS_ERR_PARAM );
   
   QString format = findFormat( PT_PREVIEW );

   // Previewfile always comes absolute
      
   ImgSaveStat stat = save( image, previewfile, format, "" );

   if( stat == ISS_OK )
   {
      KConfig *konf = KGlobal::config ();
      konf->setGroup( OP_FILE_GROUP );

      konf->writeEntry( OP_PREVIEW_FILE,   previewfile );
      konf->writeEntry( OP_PREVIEW_FORMAT, format );
   }

   return( stat );
}

/*
 * findFormat does all the stuff with the dialog.
 */
QString ImgSaver::findFormat( picType type )
{
   QString format;
   // Preview always as bitmap
   if( type == PT_PREVIEW )
   {
      KConfig *konf = KGlobal::config ();
      konf->setGroup( OP_FILE_GROUP );
      format = konf->readEntry( OP_PREVIEW_FORMAT, "nothing" );

      if( format == "nothing" )
      {
	 format = startFormatDialog( type );
      }
   }

   if( type == PT_THUMBNAIL )
   {
      return( "BMP" );
   }
   
   // real images 
   if( type != PT_PREVIEW && type != PT_THUMBNAIL && ask_for_format )
   {
      format = startFormatDialog( type );
   }
   return( format );
   
}

QString ImgSaver::startFormatDialog( picType type)
{
   FormatDialog fd( 0, "FormatDialog" );
      
   // set default values
   if( type != PT_PREVIEW )
   {
      QString defFormat = getFormatForType( type );
      fd.setSelectedFormat( defFormat );
   }
      
   if( fd.exec() )
   {
      QString format = fd.getFormat();
      kdDebug(28000) << "Storing to format <" << format << ">" << endl;
      if( fd.rememberFormat() )
      {
	 storeFormatForType( type, format );
      }
      subformat = fd.getSubFormat();

      return( format );
   }
   return( i18n("Skipped") );
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
	 kdDebug(28000) << "Wrong Type  - cant store format setting" << endl;
	 break;
   }
}


QString ImgSaver::findSubFormat( QString format )
{
   kdDebug(28000) << "Searching Subformat for " << format << endl;
   return( subformat );
   
}

/**
   private save() does the work to save the image.
   the filename must be complete and local.
**/
ImgSaveStat ImgSaver::save( QImage *image, const QString &filename,
			    const QString &format,
			    const QString &subformat )
{

   bool result = false;
   kdDebug(28000) << "in ImgSaver::save: saving " << filename << endl;
   if( ! format || !image )
   {
      kdDebug(28000) << "ImgSaver ERROR: Wrong parameter Format <" << format << "> or image" << endl;
      return( ISS_ERR_PARAM );
   }
   
   if( image )
   {
      // remember the last processed file - only the filename - no path
      QFileInfo fi( filename );

      if( fi.exists() && !fi.isWritable() )
      {
	 kdDebug(28000) << "Cant write to file <" << filename << ">, cant save !" << endl;
	 result = false;
	 return( ISS_ERR_PERM );
      }

      /* Check the format, is it writable ? */
#ifdef USE_KIMAGEIO
      if( ! KImageIO::canWrite( format ) )
      {
	 kdDebug(28000) << "Cant write format <" << format << ">" << endl;
	 result = false;
	 return( ISS_ERR_FORMAT_NO_WRITE );
      }
#endif
      kdDebug(28000) << "ImgSaver: saving image to <" << filename << "> as <" << format << "/" << subformat <<">" << endl;

      result = image->save( filename, format.latin1() );

      
      last_file = fi.absFilePath();
      last_format = format.latin1();
   }

   if( result )
      return( ISS_OK );
   else {
      last_file = "";
      last_format = "";
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
   previewfile = kookaImgRoot() + "preview.bmp";
   
   previewfile = konf->readPathEntry( OP_PREVIEW_FILE, previewfile );

}





QString ImgSaver::errorString( ImgSaveStat stat )
{
   QString re;
   
   switch( stat ) {
      case ISS_OK:           re = i18n( " image save OK      " ); break;
      case ISS_ERR_PERM:     re = i18n( " permission error   " ); break;
      case ISS_ERR_FILENAME: re = i18n( " bad filename       " ); break;
      case ISS_ERR_NO_SPACE: re = i18n( " no space on device " ); break;
      case ISS_ERR_FORMAT_NO_WRITE: re = i18n( " could not write image format " ); break;
      case ISS_ERR_PROTOCOL: re = i18n( " can not write file using that protocol "); break;
      case ISS_SAVE_CANCELED: re = i18n( " user canceled saving " ); break;
      case ISS_ERR_UNKNOWN:  re = i18n( " unknown error      " ); break;
      case ISS_ERR_PARAM:    re = i18n( " parameter wrong    " ); break;
	 
      default: re = "";
   }
   return( re );

}

QString ImgSaver::kookaImgRoot( void )
{
   QString dir = (KGlobal::dirs())->saveLocation( "data", "ScanImages", true );

   if( dir.right(1) != "/" )
      dir += "/";
   
   return( dir );

}


#if 0
/* Returns the path relativ to the kookaImgRoot. */
QString ImgSaver::relativeToImgRoot( QString path )
{
   QString dir = kookaImgRoot(); /* Always with trailing / */
   QString res = path;
   
   int idx = path.find( dir, 0 ); /* Search if dir is in path */
   if( idx > -1 )
   {
      /* was found */
      int dirlen = dir.length();
      int rightlen = path.length() - dirlen - idx -1;
      res = path.right(rightlen);
      kdDebug(28000) << "relativToImgRoot returns " << res << endl;
      
   }
   return( res );
}
#endif
#include "img_saver.moc"
