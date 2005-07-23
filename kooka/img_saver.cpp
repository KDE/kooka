/***************************************************************************
                          img_saver.cpp  -  description
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
#include <kmessagebox.h>
#include <kdebug.h>
#include <kio/jobclasses.h>
#include <kio/file.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <kinputdialog.h>

#include <qdir.h>
#include <qlayout.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qmessagebox.h>
#include <qvbox.h>
#include <qbuttongroup.h>

#include "resource.h"
#include "img_saver.h"
#include "previewer.h"
#include "kookaimage.h"

FormatDialog::FormatDialog( QWidget *parent, const QString&, const char *name )
   :KDialogBase( parent, name, true,
                 /* Tabbed,*/ i18n( "Kooka Save Assistant" ),
		 Ok|Cancel, Ok )

{
   buildHelp();
   // readConfig();
   // QFrame *page = addPage( QString( "Save the image") );
   QFrame *page = new QFrame( this );
   page->setFrameStyle( QFrame::Box | QFrame::Sunken );
   Q_CHECK_PTR( page );
   setMainWidget( page );

   QVBoxLayout *bigdad = new QVBoxLayout( page, marginHint(), spacingHint());
   Q_CHECK_PTR(bigdad);

   // some nice words
   QLabel *l0 = new QLabel( page );
   Q_CHECK_PTR(l0);
   l0->setText( i18n( "<B>Save Assistant</B><P>Select an image format to save the scanned image." ));
   bigdad->addWidget( l0 );

   KSeparator* sep = new KSeparator( KSeparator::HLine, page);
   bigdad->addWidget( sep );

   // Layout-Boxes
   // QHBoxLayout *hl1= new QHBoxLayout( );  // Caption
   QHBoxLayout *lhBigMiddle = new QHBoxLayout( spacingHint() );  // Big middle
   Q_CHECK_PTR(lhBigMiddle);
   bigdad->addLayout( lhBigMiddle );
   QVBoxLayout *lvFormatSel = new QVBoxLayout( spacingHint() );  // Selection List
   Q_CHECK_PTR(lvFormatSel);
   lhBigMiddle->addLayout( lvFormatSel );

   // Insert Scrolled List for formats
   QLabel *l1 = new QLabel( page );
   Q_CHECK_PTR(l1);
   l1->setText( i18n( "Available image formats:" ));

   lb_format = new QListBox( page, "ListBoxFormats" );
   Q_CHECK_PTR(lb_format);

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
   Q_CHECK_PTR(l_help);
   l_help->setFrameStyle( QFrame::Panel|QFrame::Sunken );
   l_help->setText( i18n("-No format selected-" ));
   l_help->setAlignment( AlignVCenter | AlignHCenter );
   l_help->setMinimumWidth(230);

   // Insert Selbox for subformat
   l2 = new QLabel( page );
   Q_CHECK_PTR(l2);
   l2->setText( i18n( "Select the image sub-format" ));
   cb_subf = new QComboBox( page, "ComboSubFormat" );
   Q_CHECK_PTR( cb_subf );

   // Checkbox to store setting
   cbDontAsk  = new QCheckBox(i18n("Don't ask again for the save format if it is defined."),
			      page );
   Q_CHECK_PTR( cbDontAsk );

   QFrame *hl = new QFrame(page);
   Q_CHECK_PTR( hl );
   hl->setFrameStyle( QFrame::HLine|QFrame::Sunken );

   // bigdad->addWidget( l_caption, 1 );
   lvFormatSel->addWidget( l1, 1 );
   lvFormatSel->addWidget( lb_format, 6 );
   lvFormatSel->addWidget( l2, 1 );
   lvFormatSel->addWidget( cb_subf, 1 );

   lhBigMiddle->addWidget( l_help, 2 );
   //bigdad->addStretch(1);
   bigdad->addWidget( hl, 1 );
   bigdad->addWidget( cbDontAsk , 2 );

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


QString FormatDialog::getFormat( ) const
{
   int item = lb_format->currentItem();

   if( item > -1 )
   {
      const QString f = lb_format->text( item );
      return( f );
   }
   return( "BMP" );
}


QCString FormatDialog::getSubFormat( ) const
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


/* ********************************************************************** */

ImgSaver::ImgSaver(  QWidget *parent, const KURL dir_name )
   : QObject( parent )
{

   if( dir_name.isEmpty() || dir_name.protocol() != "file" )
   {
      kdDebug(28000) << "ImageServer initialised with wrong dir " << dir_name.url() << endl;
      directory = Previewer::galleryRoot();
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
	 directory = dir_name.directory(true, false);
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
   directory = Previewer::galleryRoot();
   createDir( directory );

   readConfig();

   last_file = "";
   last_format ="";

}


/* Needs a full qualified directory name */
void ImgSaver::createDir( const QString& dir )
{
   KURL url( dir );

   if( ! KIO::NetAccess::exists(url, false, 0) )
   {
      kdDebug(28000) << "Wrn: Directory <" << dir << "> does not exist -> try to create  !" << endl;
      // if( mkdir( QFile::encodeName( dir ), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
      if( KIO::mkdir( KURL(dir)))
      {
        KMessageBox::sorry(0, i18n("The folder\n%1\n does not exist and could not be created;\n"
                        "please check the permissions.").arg(dir));
      }
   }
#if 0
   if( ! fi.isWritable() )
   {
        KMessageBox::sorry(0, i18n("The directory\n%1\n is not writeable;\nplease check the permissions.")
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
      if( image->depth() == 1 || image->numColors() == 2 )
      {
	 kdDebug(28000) << "This is black And White!" << endl;
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
    	kdDebug(28000) << "Save canceled by user -> no save !" << endl;
    	return( ISS_SAVE_CANCELED );
   }

   kdDebug(28000) << "saveImage: Directory is " << directory << endl;
   QString filename = createFilename( format );

   KConfig *konf = KGlobal::config ();
   konf->setGroup( OP_FILE_GROUP );

   if( konf->readBoolEntry( OP_ASK_FILENAME, false ) )
   {
       bool ok;
       QString text = KInputDialog::getText( i18n( "Filename" ), i18n("Enter filename:"),
					     filename, &ok );

       if(ok)
       {
	   filename = text;
       }
   }
       
   QString fi = directory + "/" + filename;

   if( extension(fi).isEmpty() )
   {
       if( ! fi.endsWith( "." )  )
       {
	   fi+= ".";
       }
       fi+=format.lower();
   }


   kdDebug(28000) << "saveImage: saving file <" << fi << ">" << endl;
   stat = save( image, fi, format, subformat );

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
    long c = 1;

    QString num;
    num.setNum(c);
    QString fname = "kscan_" + num.rightJustify(4, '0') + "." + format.lower();

    while( files.exists( fname ) ) {
        num.setNum(++c);
        fname = "kscan_" + num.rightJustify(4, '0') + "." + format.lower();
    }

    return( fname );
}

/**
 *   This function gets a filename from the parent. The filename must not be relative.
 **/
ImgSaveStat ImgSaver::saveImage( QImage *image, const KURL& filename, const QString& imgFormat )
{
    QString format = imgFormat;

    /* Check if the filename is local */
    if( !filename.isLocalFile())
    {
	kdDebug(29000) << "ImgSaver: Can only save local image, sorry !" << endl;
	return( ISS_ERR_PROTOCOL );
    }

    QString localFilename;
    localFilename = filename.directory( false, true) + filename.fileName();

    kdDebug(28000) << "saveImage: Saving "<< localFilename << " in format " << format << endl;
    if( format.isEmpty() )
	format = "BMP";

    return( save( image, localFilename, format, "" ) );
}


/*
 * findFormat does all the stuff with the dialog.
 */
QString ImgSaver::findFormat( picType type )
{
   QString format;
   KConfig *konf = KGlobal::config ();
   konf->setGroup( OP_FILE_GROUP );

   if( type == PT_THUMBNAIL )
   {
      return( "BMP" );
   }

   // real images
   switch( type )
   {
      case PT_THUMBNAIL:
	 format = konf->readEntry( OP_FORMAT_THUMBNAIL, "BMP" );
	 kdDebug( 28000) << "Format for Thumbnails: " << format << endl;
	 break;
      case PT_PREVIEW:
	 format = konf->readEntry( OP_PREVIEW_FORMAT, "BMP" );
	 kdDebug( 28000) << "Format for Preview: " << format << endl;
	 break;
      case PT_COLOR_IMAGE:
	 format = konf->readEntry( OP_FORMAT_COLOR, "nothing" );
	 kdDebug( 28000 ) <<  "Format for Color: " << format << endl;
	 break;
      case PT_GRAY_IMAGE:
	 format = konf->readEntry( OP_FORMAT_GRAY, "nothing" );
	 kdDebug( 28000 ) <<  "Format for Gray: " << format << endl;
	 break;
      case PT_BW_IMAGE:
	 format = konf->readEntry( OP_FORMAT_BW, "nothing" );
	 kdDebug( 28000 ) <<  "Format for BlackAndWhite: " << format << endl;
	 break;
      case PT_HICOLOR_IMAGE:
	 format = konf->readEntry( OP_FORMAT_HICOLOR, "nothing" );
	 kdDebug( 28000 ) <<  "Format for HiColorImage: " << format << endl;
	 break;
      default:
	 format = "nothing";
	 kdDebug( 28000 ) <<  "ERR: Could not find image type !" << endl;

	 break;
   }

   if( type != PT_PREVIEW ) /* Use always bmp-Default for preview scans */
   {
      if( format == "nothing" || ask_for_format )
      {
	 format = startFormatDialog( type );
      }
   }
   return( format );

}

QString ImgSaver::picTypeAsString( picType type ) const
{
   QString res;

   switch( type )
   {
      case PT_COLOR_IMAGE:
	 res = i18n( "palleted color image (16 or 24 bit depth)" );
	 break;
      case PT_GRAY_IMAGE:
	 res = i18n( "palleted gray scale image (16 bit depth)" );
	 break;
      case PT_BW_IMAGE:
	 res = i18n( "lineart image (black and white, 1 bit depth)" );
	 break;
      case PT_HICOLOR_IMAGE:
	 res = i18n( "high (or true-) color image, not palleted" );
	 break;
      default:
	 res = i18n( "Unknown image type" );
	 break;
   }
   return( res );
}


QString ImgSaver::startFormatDialog( picType type)
{

   FormatDialog fd( 0, picTypeAsString( type ), "FormatDialog" );

   // set default values
   if( type != PT_PREVIEW )
   {
      QString defFormat = getFormatForType( type );
      fd.setSelectedFormat( defFormat );
   }

   QString format;
   if( fd.exec() )
   {
      format = fd.getFormat();
      kdDebug(28000) << "Storing to format <" << format << ">" << endl;
      bool ask = fd.askForFormat();
      kdDebug(28000)<< "Store askFor is " << ask << endl;
      storeFormatForType( type, format, ask );
      subformat = fd.getSubFormat();
   }
   return( format );
}


/*
 *  This method returns true if the image format given in format is remembered
 *  for that image type.
 */
bool ImgSaver::isRememberedFormat( picType type, QString format ) const
{
   if( getFormatForType( type ) == format )
   {
      return( true );
   }
   else
   {
      return( false );
   }

}




QString ImgSaver::getFormatForType( picType type ) const
{
   KConfig *konf = KGlobal::config ();
   Q_CHECK_PTR( konf );
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


void ImgSaver::storeFormatForType( picType type, QString format, bool ask )
{
   KConfig *konf = KGlobal::config ();
   Q_CHECK_PTR( konf );
   konf->setGroup( OP_FILE_GROUP );

   konf->writeEntry( OP_FILE_ASK_FORMAT, ask );
   ask_for_format = ask;

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
   konf->sync();
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
      QString dirPath = fi.dirPath();
      QDir dir = QDir( dirPath );

      if( ! dir.exists() )
      {
	 /* The dir to save in always should exist, except in the first preview save */
	 kdDebug(28000) << "Creating dir " << dirPath << endl;
	 if( !dir.mkdir( dirPath ) )
	 {
	    kdDebug(28000) << "ERR: Could not create directory" << endl;
	 }
      }

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
   Q_CHECK_PTR( konf );
   konf->setGroup( OP_FILE_GROUP );
   ask_for_format = konf->readBoolEntry( OP_FILE_ASK_FORMAT, true );

   QDir home = QDir::home();
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

QString ImgSaver::extension( const KURL& url )
{
   QString extension = url.fileName();

   int dotPos = extension.findRev( '.' );
   if( dotPos > 0 )
   {
      int len = extension.length();
      extension = extension.right( len - dotPos -1 );
   }
   else
   {
      /* No extension was supplied */
      extension = QString();
   }
   return extension;
}


bool ImgSaver::renameImage( const KURL& fromUrl, KURL& toUrl, bool askExt,  QWidget *overWidget )
{
   /* Check if the provided filename has a extension */
   QString extTo = extension( toUrl );
   QString extFrom = extension( fromUrl );
   KURL targetUrl( toUrl );

   if( extTo.isEmpty() && !extFrom.isEmpty() )
   {
      /* Ask if the extension should be added */
      int result = KMessageBox::Yes;
      QString fName = toUrl.fileName();
      if( ! fName.endsWith( "." )  )
      {
	 fName += ".";
      }
      fName += extFrom;

      if( askExt )
      {

	 QString s;
	 s = i18n("The filename you supplied has no file extension.\nShould the correct one be added automatically? ");
	 s += i18n( "That would result in the new filename: %1" ).arg( fName);

	 result = KMessageBox::questionYesNo(overWidget, s, i18n( "Extension Missing"),
					     i18n("Add Extension"), i18n("Do Not Add"),
					     "AutoAddExtensions" );
      }

      if( result == KMessageBox::Yes )
      {
	 targetUrl.setFileName( fName );
	 kdDebug(28000) << "Rename file to " << targetUrl.prettyURL() << endl;
      }
   }
   else if( !extFrom.isEmpty() && extFrom != extTo )
   {
       if( ! ((extFrom.lower() == "jpeg" && extTo.lower() == "jpg") ||
	      (extFrom.lower() == "jpg"  && extTo.lower() == "jpeg" )))
       {
	   /* extensions differ -> TODO */
	   KMessageBox::error( overWidget,
			       i18n("Format changes of images are currently not supported."),
			       i18n("Wrong Extension Found" ));
	   return(false);
       }
   }

   bool success = false;

   if( KIO::NetAccess::exists( targetUrl, false,0 ) )
   {
      kdDebug(28000)<< "Target already exists - can not copy" << endl;
   }
   else
   {
      if( KIO::file_move(fromUrl, targetUrl) )
      {
	 success = true;
      }
   }
   return( success );
}


QString ImgSaver::tempSaveImage( KookaImage *img, const QString& format, int colors )
{

    KTempFile *tmpFile = new KTempFile( QString(), "."+format.lower());
    tmpFile->setAutoDelete( false );
    tmpFile->close();

    KookaImage tmpImg;

    if( colors != -1 && img->numColors() != colors )
    {
	// Need to convert image
	if( colors == 1 || colors == 8 || colors == 24 || colors == 32 )
	{
	    tmpImg = img->convertDepth( colors );
	    img = &tmpImg;
	}
	else
	{
	    kdDebug(29000) << "ERROR: Wrong color depth requested: " << colors << endl;
	    img = 0;
	}
    }

    QString name;
    if( img )
    {
	name = tmpFile->name();

	if( ! img->save( name, format.latin1() ) ) name = QString();
    }
    delete tmpFile;
    return name;
}

bool ImgSaver::copyImage( const KURL& fromUrl, const KURL& toUrl, QWidget *overWidget )
{

   /* Check if the provided filename has a extension */
   QString extTo = extension( toUrl );
   QString extFrom = extension( fromUrl );
   KURL targetUrl( toUrl );

   if( extTo.isEmpty() && !extFrom.isEmpty())
   {
      /* Ask if the extension should be added */
      int result = KMessageBox::Yes;
      QString fName = toUrl.fileName();
      if( ! fName.endsWith( "." ))
	 fName += ".";
      fName += extFrom;

      QString s;
      s = i18n("The filename you supplied has no file extension.\nShould the correct one be added automatically? ");
      s += i18n( "That would result in the new filename: %1" ).arg( fName);

      result = KMessageBox::questionYesNo(overWidget, s, i18n( "Extension Missing"),
					  i18n("Add Extension"), i18n("Do Not Add"),
					  "AutoAddExtensions" );

      if( result == KMessageBox::Yes )
      {
	 targetUrl.setFileName( fName );
      }
   }
   else if( !extFrom.isEmpty() && extFrom != extTo )
   {
      /* extensions differ -> TODO */
       if( ! ((extFrom.lower() == "jpeg" && extTo.lower() == "jpg") ||
	      (extFrom.lower() == "jpg"  && extTo.lower() == "jpeg" )))
       {
	   KMessageBox::error( overWidget, i18n("Format changes of images are currently not supported."),
			       i18n("Wrong Extension Found" ));
	   return(false);
       }
   }

   KIO::Job *copyjob = KIO::copy( fromUrl, targetUrl, false );

   return( copyjob ? true : false );
}


/* extension needs to be added */

#include "img_saver.moc"
