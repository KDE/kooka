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
#include <qregexp.h>

#include <kglobal.h>
#include <kconfig.h>
#include <kimageio.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <ktempfile.h>
#include <kio/job.h>
#include <kio/netaccess.h>

#include "previewer.h"
#include "kookaimage.h"
#include "formatdialog.h"

#include "imgsaver.h"
#include "imgsaver.moc"


ImgSaver::ImgSaver(QWidget *parent,const KURL &dir)
    : QObject(parent)
{
    if (dir.isValid() && !dir.isEmpty() && dir.protocol()=="file")
    {							// can use specified place
        m_saveDirectory = dir.directory(true,false);
        kdDebug(28000) << k_funcinfo << "specified directory " << m_saveDirectory << endl;
    }
    else						// cannot, so use default
    {
        m_saveDirectory = Previewer::galleryRoot();
        kdDebug(28000) << k_funcinfo << "default directory " << m_saveDirectory << endl;
    }

    createDir(m_saveDirectory);				// ensure save location exists
    last_format = "";					// nothing saved yet
}


/* Needs a full qualified directory name */
void ImgSaver::createDir(const QString &dir)
{
    KURL url(dir);
    if (!KIO::NetAccess::exists(url,false,0))
    {
        kdDebug(28000) << k_funcinfo << "directory <" << dir << "> does not exist, try to create" << endl;
        // if( mkdir( QFile::encodeName( dir ), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
        if (KIO::mkdir(url))
        {
            KMessageBox::sorry(0,i18n("<qt>The folder<br><b>%1</b><br>does not exist and could not be created").arg(dir));
        }
    }
#if 0
    if (!fi.isWritable())
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

    QString saveFilename = createFilename();		// find next unused filename
    QString saveFormat = findFormat(imgType);		// find saved image format
    QString saveSubformat = findSubFormat(saveFormat);	// currently not used

    KConfig *konf = KGlobal::config();
    konf->setGroup(OP_SAVER_GROUP);			// get dialogue preferences
    m_saveAskFilename = konf->readBoolEntry(OP_SAVER_ASK_FILENAME,false);
    m_saveAskFormat = konf->readBoolEntry(OP_SAVER_ASK_FORMAT,false);

    kdDebug(28000) << k_funcinfo << "before dialogue,"
                   << " ask_filename=" << m_saveAskFilename
                   << " ask_format=" << m_saveAskFormat
                   << " filename=[" << saveFilename << "]"
                   << " format=[" << saveFormat << "]"
                   << " subformat=[" << saveSubformat << "]"
                   << endl;


    while (saveFormat.isEmpty() || m_saveAskFormat || m_saveAskFilename)
    {							// is a dialogue neeeded?
        FormatDialog fd(NULL,imgType,m_saveAskFormat,saveFormat,m_saveAskFilename,saveFilename);
        if (!fd.exec()) return (ISS_SAVE_CANCELED);	// do the dialogue

        saveFilename = fd.getFilename();		// get filename as entered
        if (fd.useAssistant())				// redo with format options
        {
            m_saveAskFormat = true;
            continue;
        }

        saveFormat = fd.getFormat();			// get results from that
        saveSubformat = fd.getSubFormat();

        if (!saveFormat.isEmpty())			// have a valid format
        {
            if (fd.alwaysUseFormat()) storeFormatForType(imgType,saveFormat);
            break;					// save format for future
        }
    }

    kdDebug(28000) << k_funcinfo << "after dialogue,"
                   << " filename=[" << saveFilename << "]"
                   << " format=[" << saveFormat << "]"
                   << " subformat=[" << saveSubformat << "]"
                   << endl;

    QString fi = m_saveDirectory+"/"+saveFilename;	// full path to save
#ifdef USE_KIMAGEIO
    QString ext = KImageIO::suffix(saveFormat);		// extension it should have
#else
    QString ext = saveFormat.lower();
#endif
    if (extension(fi)!=ext)				// already has correct extension?
    {
        fi +=  ".";					// no, add it on
        fi += ext;
    }

    return (save(image,fi,saveFormat,saveSubformat));	// save image to that file
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
	kdDebug(28000) << k_funcinfo << "Can only save local files" << endl;
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
 *  Find the next filename to use for the image to save.
 *  This is done by enumerating and checking against all existing files,
 *  regardless of format - because we have not resolved the format yet.
 **/
QString ImgSaver::createFilename()
{
    QDir files(m_saveDirectory,"kscan_[0-9][0-9][0-9][0-9].*");
    QStringList l(files.entryList());
    l.gres(QRegExp("\\..*$"),"");

    QString fname;
    for (unsigned long c = 1; c<=l.count()+1; ++c)	// that must be the upper bound
    {
	fname = "kscan_"+QString::number(c).rightJustify(4,'0');
	if (!l.contains(fname)) break;
    }

    kdDebug(28000) << k_funcinfo << "returning '" << fname << "'" << endl;
    return (fname);
}


/*
 * findFormat looks to see if there is a previously saved file format for
 * the image type in question.
 */

QString ImgSaver::findFormat(ImgSaver::ImageType type)
{
    if (type==ImgSaver::ImgThumbnail) return ("BMP");	// thumbnail always this format
    if (type==ImgSaver::ImgPreview) return ("BMP");	// preview always this format
							// real images from here on
    QString format = getFormatForType(type);
    kdDebug(28000) << k_funcinfo << "format for type " << type << " = " << format << endl;
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




/*
 *  This method returns true if the image format given in format is remembered
 *  for that image type.
 */
bool ImgSaver::isRememberedFormat(ImgSaver::ImageType type,const QString &format)
{
    return (getFormatForType(type)==format);
}




const char *configKeyFor(ImgSaver::ImageType type)
{
    switch (type)
    {
case ImgSaver::ImgColor:    return (OP_FORMAT_COLOR);
case ImgSaver::ImgGray:     return (OP_FORMAT_GRAY);
case ImgSaver::ImgBW:       return (OP_FORMAT_BW);
case ImgSaver::ImgHicolor:  return (OP_FORMAT_HICOLOR);

default:                    kdDebug(28000) << k_funcinfo << "unknown type " << type << endl;
                            return (OP_FORMAT_UNKNOWN);
    }
}



QString ImgSaver::getFormatForType(ImgSaver::ImageType type)
{
    KConfig *konf = KGlobal::config();
    konf->setGroup(OP_SAVER_GROUP);
    return (konf->readEntry(configKeyFor(type)));
}


void ImgSaver::storeFormatForType(ImgSaver::ImageType type,const QString &format)
{
    KConfig *konf = KGlobal::config();
    konf->setGroup(OP_SAVER_GROUP);

    //  We don't save OP_FILE_ASK_FORMAT here, this is the global setting
    //  "Always use the Save Assistant" from the Kooka configuration which
    //  is a preference option affecting all image types.  To get automatic
    //  saving in the preferred format, turn off that option in "Configure
    //  Kooka - Image Saver" and select "Always use this format for this type
    //  of file" when saving an image.  As long as an image of that type has
    //  scanned and saved, then the Save Assistant will not subsequently
    //  appear for that image type.
    //
    //  This means that turning on the "Always use the Save Assistant" option
    //  will do exactly what it says.
    //
    //  konf->writeEntry( OP_FILE_ASK_FORMAT, ask );
    //  m_saveAskFormat = ask;

    konf->writeEntry(configKeyFor(type),format);
    konf->sync();
}



QString ImgSaver::findSubFormat(const QString &format)
{
    kdDebug(28000) << k_funcinfo << "for " << format << endl;
    return (QString::null);				// no subformats currently used
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


QString ImgSaver::extension(const KURL &url)
{
    QString extension = url.fileName();

    int dotPos = extension.findRev('.');		// find last separator
    if( dotPos>0) extension = extension.mid(dotPos+1);	// extract from filename
    else extension = QString::null;			// no extension
    return (extension);
}


bool ImgSaver::renameImage( const KURL& fromUrl, const KURL& toUrl, bool askExt,  QWidget *overWidget )
{
   /* Check if the provided filename has a extension */
   QString extTo = extension( toUrl );
   QString extFrom = extension( fromUrl );
   KURL targetUrl( toUrl );

   if( extTo.isEmpty() && !extFrom.isEmpty() )
   {
      /* Ask if the extension
 should be added */
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
	    kdDebug(28000) << "ERROR: Wrong color depth requested: " << colors << endl;
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
