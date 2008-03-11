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

#include <qdir.h>
#include <qlayout.h>
#include <qcombobox.h>

#include <kglobal.h>
#include <kconfig.h>
#include <kimageio.h>
#include <kseparator.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <kinputdialog.h>

#include "previewer.h"
#include "kookaimage.h"
#include "formatdialog.h"

#include "imgsaver.h"
#include "imgsaver.moc"


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

   last_format ="";

}


ImgSaver::ImgSaver( QWidget *parent )
   :QObject( parent )
{
   directory = Previewer::galleryRoot();
   createDir( directory );

   readConfig();

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
ImgSaveStat ImgSaver::saveImage(const QImage *image)
{
    if (image==NULL) return (ISS_ERR_PARAM);

    ImgSaver::ImageType imgType = ImgSaver::ImgNone;	// find out what kind of image it is
    if (image->depth()>8) imgType = ImgSaver::ImgHicolor;
    else
    {
        if (image->depth()==1 || image->numColors()==2) imgType = ImgSaver::ImgBW;
        else
        {
            if (image->allGray()) imgType = ImgSaver::ImgGray;
            else imgType = ImgSaver::ImgColor;
        }
    }

    QString format = findFormat(imgType);		// find/ask for image format
    QString subformat = findSubFormat(format);
    if (format.isEmpty()) return (ISS_SAVE_CANCELED);	// dialogue cancelled

    QString filename = createFilename(format);		// find an unused filename

    KConfig *konf = KGlobal::config();
    konf->setGroup(OP_FILE_GROUP);
    if (konf->readBoolEntry(OP_ASK_FILENAME,false))	// do we ask for a file name?
    {
        bool ok;
	// TODO: combine this file name request with the format dialogue, if used
        QString text = KInputDialog::getText(i18n("Filename"),i18n("Enter image file name:"),
                                             filename,&ok);

        if (!ok) return (ISS_SAVE_CANCELED);
        filename = text;
    }

    QString fi = directory+"/"+filename;		// full path to save
    if (extension(fi).isEmpty())			// does it have an extension?
    {
        if (!fi.endsWith(".")) fi +=  ".";
#ifdef USE_KIMAGEIO
        fi += KImageIO::suffix(format);
#else
        fi += format.lower();
#endif
    }

    return (save(image,fi,format,subformat));
}



/**
 *   This function uses a filename provided by the caller.
 **/
ImgSaveStat ImgSaver::saveImage(const QImage *image,const KURL &url,const QString &imgFormat)
{
    QString format = imgFormat;
    if (format.isEmpty()) format = "BMP";

    return (save(image,url,format,QString::null));
}


/**
   private save() does the work to save the image.
   the filename must be complete and local.
**/
ImgSaveStat ImgSaver::save(const QImage *image,const KURL &url,
                           const QString &format,const QString &subformat)
{
    if (format.isEmpty() || image==NULL) return (ISS_ERR_PARAM);

    kdDebug(28000) << k_funcinfo << "to " << url.prettyURL()
                   << " in format <" << format << ">"
                   << " subformat <" << subformat << ">" << endl;

    last_format = format.latin1();			// save for error message later
    last_url = url;

    if (!url.isLocalFile())				// file must be local
    {
	kdDebug(29000) << k_funcinfo << "Can only save local files" << endl;
	return (ISS_ERR_PROTOCOL);
    }

    QString filename = url.path();			// local file path
    QFileInfo fi(filename);				// information for that
    QString dirPath = fi.dirPath();			// containing directory

    QDir dir(dirPath);
    if (!dir.exists())					// should always exist, except
    {							// for first preview save
        kdDebug(28000) << "Creating dir <" << dirPath << ">" << endl;
        if (!dir.mkdir(dirPath))
        {
	    kdDebug(28000) << k_funcinfo << "Could not create directory <" << dirPath << ">" << endl;
            return (ISS_ERR_MKDIR);
        }
    }

    if (fi.exists() && !fi.isWritable())
    {
        kdDebug(28000) << k_funcinfo << "Cannot overwrite existing file <" << filename << ">" << endl;
        return (ISS_ERR_PERM);
    }

#ifdef USE_KIMAGEIO
    if (!KImageIO::canWrite(format))			// check the format, is it writable?
    {
        kdDebug(28000) << k_funcinfo << "Cannot write format <" << format << ">" << endl;
        return (ISS_ERR_FORMAT_NO_WRITE);
    }
#endif

    bool result = image->save(filename,format.latin1());
    return (result ? ISS_OK : ISS_ERR_UNKNOWN);
}


/**
 *  This member creates a filename for the image to save.
 *  This is done by numbering all existing files and adding
 *  one
 **/
QString ImgSaver::createFilename(QString format)
{
    if (format.isEmpty()) return (QString::null);
    const QString suffix = KImageIO::suffix(format);	// file extension for saving

    QString s = "kscan_*."+suffix;
    QDir files(directory,s);				// list of existing files

    QString fname;
    for (unsigned long c = 1; c<=files.count()+1; ++c)	// that must be the upper bound
    {
	fname = "kscan_"+QString::number(c).rightJustify(4,'0')+"."+suffix;
	if (!files.exists(fname)) break;
    }

    return (fname);
}

/*
 * findFormat does all the stuff with the dialog.
 */
QString ImgSaver::findFormat( ImgSaver::ImageType type )
{
   if (type==ImgSaver::ImgThumbnail) return ("BMP");	// thumbnail always this format
   if (type==ImgSaver::ImgPreview) return ("BMP");	// preview always this format
							// real images from here on
   QString format;
   KConfig *konf = KGlobal::config ();
   konf->setGroup( OP_FILE_GROUP );

   switch( type )
   {
      case ImgSaver::ImgColor:
	 format = konf->readEntry( OP_FORMAT_COLOR, "nothing" );
	 kdDebug( 28000 ) <<  "Format for Color: " << format << endl;
	 break;
      case ImgSaver::ImgGray:
	 format = konf->readEntry( OP_FORMAT_GRAY, "nothing" );
	 kdDebug( 28000 ) <<  "Format for Gray: " << format << endl;
	 break;
      case ImgSaver::ImgBW:
	 format = konf->readEntry( OP_FORMAT_BW, "nothing" );
	 kdDebug( 28000 ) <<  "Format for BlackAndWhite: " << format << endl;
	 break;
      case ImgSaver::ImgHicolor:
	 format = konf->readEntry( OP_FORMAT_HICOLOR, "nothing" );
	 kdDebug( 28000 ) <<  "Format for HiColorImage: " << format << endl;
	 break;
      default:
	 format = "nothing";
	 kdDebug( 28000 ) <<  "ERR: Could not find image type !" << endl;
	 break;
   }

   if (format=="nothing" || ask_for_format) format = startFormatDialog(type);
   return (format);
}


QString ImgSaver::picTypeAsString(ImgSaver::ImageType type)
{
    QString res;

    switch (type)
    {
case ImgSaver::ImgColor:
        res = i18n("indexed color image (up to 8 bit depth)");
	break;

case ImgSaver::ImgGray:
	res = i18n("gray scale image (up to 8 bit depth)");
	break;

case ImgSaver::ImgBW:
	res = i18n("lineart image (black and white, 1 bit depth)");
	break;

case ImgSaver::ImgHicolor:
	res = i18n("high/true color image (more than 8 bit depth)");
	break;

default:
	res = i18n("unknown image type %1").arg(type);
	break;
    }

    return (res);
}


QString ImgSaver::startFormatDialog( ImgSaver::ImageType type)
{

   FormatDialog fd(NULL,type);

   // set default values
   if( type != ImgSaver::ImgPreview )
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
bool ImgSaver::isRememberedFormat( ImgSaver::ImageType type, QString format ) const
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




QString ImgSaver::getFormatForType( ImgSaver::ImageType type ) const
{
   KConfig *konf = KGlobal::config ();
   Q_CHECK_PTR( konf );
   konf->setGroup( OP_FILE_GROUP );

   QString f;

   switch( type )
   {
      case ImgSaver::ImgColor:
	 f = konf->readEntry( OP_FORMAT_COLOR, "BMP" );
	 break;
      case ImgSaver::ImgGray:
	 f = konf->readEntry( OP_FORMAT_GRAY, "BMP" );
	 break;
      case ImgSaver::ImgBW:
	 f = konf->readEntry( OP_FORMAT_BW, "BMP" );
	 break;
      case ImgSaver::ImgHicolor:
	 f = konf->readEntry( OP_FORMAT_HICOLOR, "BMP" );
	 break;
      default:
	 f = "BMP";
	 break;
   }
   return( f );
}


void ImgSaver::storeFormatForType( ImgSaver::ImageType type, QString format, bool ask )
{
   KConfig *konf = KGlobal::config ();
   Q_CHECK_PTR( konf );
   konf->setGroup( OP_FILE_GROUP );

   konf->writeEntry( OP_FILE_ASK_FORMAT, ask );
   ask_for_format = ask;

   switch( type )
   {
      case ImgSaver::ImgColor:
	 konf->writeEntry( OP_FORMAT_COLOR, format );
	 break;
      case ImgSaver::ImgGray:
	 konf->writeEntry( OP_FORMAT_GRAY, format  );
	 break;
      case ImgSaver::ImgBW:
	 konf->writeEntry( OP_FORMAT_BW, format );
	 break;
      case ImgSaver::ImgHicolor:
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


void ImgSaver::readConfig( void )
{

   KConfig *konf = KGlobal::config ();
   Q_CHECK_PTR( konf );
   konf->setGroup( OP_FILE_GROUP );
   ask_for_format = konf->readBoolEntry( OP_FILE_ASK_FORMAT, true );
}


QString ImgSaver::errorString(ImgSaveStat stat)
{
    QString re;
    switch (stat)
    {
case ISS_OK:			re = i18n("Save OK");			break;
case ISS_ERR_PERM:		re = i18n("Permission denied");		break;
case ISS_ERR_FILENAME:		re = i18n("Bad file name");		break;  // never used
case ISS_ERR_NO_SPACE:		re = i18n("No space left on device");	break;	// never used
case ISS_ERR_FORMAT_NO_WRITE:	re = i18n("Cannot write image format '%1'").arg(last_format);
									break;
case ISS_ERR_PROTOCOL:		re = i18n("Cannot write using protocol '%1'").arg(last_url.protocol());
									break;
case ISS_SAVE_CANCELED:		re = i18n("User cancelled saving");	break;
case ISS_ERR_MKDIR:		re = i18n("Cannot create directory");	break;
case ISS_ERR_UNKNOWN:		re = i18n("Save failed");		break;
case ISS_ERR_PARAM:		re = i18n("Bad parameter");		break;
default:			re = i18n("Unknown status %1").arg(stat);
				break;
    }
    return (re);
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


bool ImgSaver::renameImage( const KURL& fromUrl, const KURL& toUrl, bool askExt,  QWidget *overWidget )
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
