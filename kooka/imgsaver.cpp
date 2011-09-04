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

#include "imgsaver.h"

#include <qdir.h>
#include <qregexp.h>

#include <kglobal.h>
#include <kconfig.h>
#include <kimageio.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <ktemporaryfile.h>
#include <kmimetype.h>

#include <kio/job.h>
#include <kio/netaccess.h>

#include "imageformat.h"
#include "kookaimage.h"
#include "kookapref.h"
#include "formatdialog.h"

#include "libkscan/imagemetainfo.h"


/* Needs a full qualified directory name */
void createDir(const QString &dir)
{
    KUrl url(dir);
    if (!KIO::NetAccess::exists(url, KIO::NetAccess::DestinationSide, NULL))
    {
        kDebug() << "directory" << dir << "does not exist, try to create";
        // if( mkdir( QFile::encodeName( dir ), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
        if (KIO::mkdir(url))
        {
            KMessageBox::sorry(NULL, i18n("<qt>"
                                          "The folder<br>"
                                          "<filename>%2</filename><br>"
                                          "does not exist and could not be created.<br>"
                                          "<br>"
                                          "%1", 
                                          KIO::NetAccess::lastErrorString(),
                                          dir),
                               i18n("Error creating directory"));
        }
    }
#if 0
    if (!fi.isWritable())
    {
        KMessageBox::sorry(NULL, i18n("<qt>The directory<br><filename>%1</filename><br> is not writable, please check the permissions.", dir));
    }
#endif
}


ImgSaver::ImgSaver(const KUrl &dir)
    : mSaveUrl(KUrl()),
      mSaveFormat("")
{
    if (dir.isValid() && !dir.isEmpty() && dir.protocol()=="file")
    {							// can use specified place
        m_saveDirectory = dir.directory(KUrl::ObeyTrailingSlash);
        kDebug() << "specified directory" << m_saveDirectory;
    }
    else						// cannot, so use default
    {
        m_saveDirectory = KookaPref::galleryRoot();
        kDebug() << "default directory" << m_saveDirectory;
    }

    createDir(m_saveDirectory);				// ensure save location exists
}


QString extension(const KUrl &url)
{
    return (KMimeType::extractKnownExtension(url.pathOrUrl()));
}


ImgSaver::ImageSaveStatus ImgSaver::getFilenameAndFormat(ImageMetaInfo::ImageType type)
{
    kDebug() << "for type=" << type;

    if (type==ImageMetaInfo::Unknown) return (ImgSaver::SaveStatusParam);

    QString saveFilename = createFilename();		// find next unused filename
    ImageFormat saveFormat = findFormat(type);		// find saved image format
    QString saveSubformat = findSubFormat(saveFormat);	// currently not used
							// get dialogue preferences
    const KConfigGroup grp = KGlobal::config()->group(OP_SAVER_GROUP);
    m_saveAskFilename = grp.readEntry(OP_SAVER_ASK_FILENAME, false);
    m_saveAskFormat = grp.readEntry(OP_SAVER_ASK_FORMAT, false);

    kDebug() << "before dialogue,"
             << "ask_filename=" << m_saveAskFilename
             << "ask_format=" << m_saveAskFormat
             << "filename=" << saveFilename
             << "format=" << saveFormat
             << "subformat=" << saveSubformat;

    while (!saveFormat.isValid() || m_saveAskFormat || m_saveAskFilename)
    {							// is a dialogue neeeded?
        FormatDialog fd(NULL,type,m_saveAskFormat,saveFormat,m_saveAskFilename,saveFilename);
        if (!fd.exec()) return (ImgSaver::SaveStatusCanceled);
							// do the dialogue
        saveFilename = fd.getFilename();		// get filename as entered
        if (fd.useAssistant())				// redo with format options
        {
            m_saveAskFormat = true;
            continue;
        }

        saveFormat = fd.getFormat();			// get results from that
        saveSubformat = fd.getSubFormat();

        if (saveFormat.isValid())			// have a valid format
        {
            if (fd.alwaysUseFormat()) storeFormatForType(type, saveFormat);
            break;					// save format for future
        }
    }

    QString fi = m_saveDirectory+QDir::separator()+saveFilename;
							// full path to save
    QString ext = saveFormat.extension();
    if (extension(fi)!=ext)				// already has correct extension?
    {
        fi +=  ".";					// no, add it on
        fi += ext;
    }

    mSaveUrl = fi;
    mSaveFormat = saveFormat;
    mSaveSubformat = saveSubformat;

    kDebug() << "after dialogue,"
             << "filename=" << saveFilename
             << "format=" << mSaveFormat
             << "subformat=" << mSaveSubformat
             << "url=" << mSaveUrl;
    return (ImgSaver::SaveStatusOk);
}


// This tells us the image information available at the start of a scan.
// If it is of any use, then resolve the filename and format to use
// and save them for when they are needed.

ImgSaver::ImageSaveStatus ImgSaver::setImageInfo(const ImageMetaInfo *info)
{
    if (info==NULL) return (ImgSaver::SaveStatusParam);
    return (getFilenameAndFormat(info->getImageType()));
}


/**
 *   This function asks the user for a filename or creates
 *   one by itself, depending on the settings
 **/
ImgSaver::ImageSaveStatus ImgSaver::saveImage(const QImage *image)
{
    if (image==NULL) return (ImgSaver::SaveStatusParam);

    if (!mSaveFormat.isValid())				// see if have this already
    {							// if not, get from image now
        kDebug() << "format not resolved yet";
        ImgSaver::ImageSaveStatus stat = getFilenameAndFormat(ImageMetaInfo::findImageType(image));
        if (stat!=ImgSaver::SaveStatusOk) return (stat);
    }

    if (!mSaveUrl.isValid() || !mSaveFormat.isValid())	// must have these now
    {
        kDebug() << "format not resolved!";
        return (ImgSaver::SaveStatusParam);
    }
							// save image to file
    return (save(image, mSaveUrl, mSaveFormat, mSaveSubformat));
}



/**
 *   This function uses a filename provided by the caller.
 **/
ImgSaver::ImageSaveStatus ImgSaver::saveImage(const QImage *image,
                                              const KUrl &url,
                                              const ImageFormat &format)
{
    return (save(image, url, format));
}


/**
   private save() does the work to save the image.
   the filename must be complete and local.
**/
// TODO: this is essentially the same as the previous function
ImgSaver::ImageSaveStatus ImgSaver::save(const QImage *image,
                                         const KUrl &url,
                                         const ImageFormat &format,
                                         const QString &subformat)
{
    if (image==NULL) return (ImgSaver::SaveStatusParam);

    kDebug() << "to" << url.prettyUrl() << "format" << format << "subformat" << subformat;

    mLastFormat = format.name();			// save for error message later
    mLastUrl = url;

    if (!url.isLocalFile())				// file must be local
    {
	kDebug() << "Can only save local files";
	return (ImgSaver::SaveStatusProtocol);
    }

    QString filename = url.path();			// local file path
    QFileInfo fi(filename);				// information for that
    QString dirPath = fi.path();			// containing directory

    QDir dir(dirPath);
    if (!dir.exists())					// should always exist, except
    {							// for first preview save
        kDebug() << "Creating directory" << dirPath;
        if (!dir.mkdir(dirPath))
        {
	    kDebug() << "Could not create directory" << dirPath;
            return (ImgSaver::SaveStatusMkdir);
        }
    }

    if (fi.exists() && !fi.isWritable())
    {
        kDebug() << "Cannot overwrite existing file" << filename;
        return (ImgSaver::SaveStatusPermission);
    }

    if (!format.canWrite())				// check format, is it writable?
    {
        kDebug() << "Cannot write format" << format;
        return (ImgSaver::SaveStatusFormatNoWrite);
    }

    bool result = image->save(filename, format.name());
    return (result ? ImgSaver::SaveStatusOk : ImgSaver::SaveStatusUnknown);
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
    l.replaceInStrings(QRegExp("\\..*$"),"");

    QString fname;
    for (int c = 1; c<=l.count()+1; ++c)		// that must be the upper bound
    {
	fname = "kscan_"+QString::number(c).rightJustified(4, '0');
	if (!l.contains(fname)) break;
    }

    kDebug() << "returning" << fname;
    return (fname);
}


/*
 * findFormat looks to see if there is a previously saved file format for
 * the image type in question.
 */

ImageFormat ImgSaver::findFormat(ImageMetaInfo::ImageType type)
{
    if (type==ImageMetaInfo::Thumbnail) return (ImageFormat("BMP"));	// thumbnail always this format
    if (type==ImageMetaInfo::Preview) return (ImageFormat("BMP"));	// preview always this format
									// real images from here on
    ImageFormat format = getFormatForType(type);
    kDebug() << "format for type" << type << "=" << format;
    return (format);
}


QString ImgSaver::picTypeAsString(ImageMetaInfo::ImageType type)
{
    QString res;

    switch (type)
    {
case ImageMetaInfo::LowColour:
        res = i18n("indexed color image (up to 8 bit depth)");
	break;

case ImageMetaInfo::Greyscale:
	res = i18n("gray scale image (up to 8 bit depth)");
	break;

case ImageMetaInfo::BlackWhite:
	res = i18n("lineart image (black and white, 1 bit depth)");
	break;

case ImageMetaInfo::HighColour:
	res = i18n("high/true color image (more than 8 bit depth)");
	break;

default:
	res = i18n("unknown image type %1", type);
	break;
    }

    return (res);
}


/*
 *  This method returns true if the image format given in format is remembered
 *  for that image type.
 */
bool ImgSaver::isRememberedFormat(ImageMetaInfo::ImageType type, const ImageFormat &format)
{
    return (getFormatForType(type)==format);
}


const char *configKeyFor(ImageMetaInfo::ImageType type)
{
    switch (type)
    {
case ImageMetaInfo::LowColour:	return (OP_FORMAT_COLOR);
case ImageMetaInfo::Greyscale:	return (OP_FORMAT_GRAY);
case ImageMetaInfo::BlackWhite:	return (OP_FORMAT_BW);
case ImageMetaInfo::HighColour:	return (OP_FORMAT_HICOLOR);

default:			kDebug() << "unknown type" << type;
				return (OP_FORMAT_UNKNOWN);
    }
}


ImageFormat ImgSaver::getFormatForType(ImageMetaInfo::ImageType type)
{
    const KConfigGroup grp = KGlobal::config()->group(OP_SAVER_GROUP);
    return (ImageFormat(grp.readEntry(configKeyFor(type), "").toLocal8Bit()));
}


void ImgSaver::storeFormatForType(ImageMetaInfo::ImageType type, const ImageFormat &format)
{
    KConfigGroup grp = KGlobal::config()->group(OP_SAVER_GROUP);

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

    grp.writeEntry(configKeyFor(type), format.name());
    grp.sync();
}


QString ImgSaver::findSubFormat(const ImageFormat &format)
{
    kDebug() << "for" << format;
    return (QString::null);				// no subformats currently used
}


QString ImgSaver::errorString(ImgSaver::ImageSaveStatus status)
{
    QString re;
    switch (status)
    {
case ImgSaver::SaveStatusOk:
        re = i18n("Save OK");							break;
case ImgSaver::SaveStatusPermission:
        re = i18n("Permission denied");						break;
case ImgSaver::SaveStatusBadFilename:			// never used
        re = i18n("Bad file name");						break;
case ImgSaver::SaveStatusNoSpace:			// never used
        re = i18n("No space left on device");					break;
case ImgSaver::SaveStatusFormatNoWrite:
        re = i18n("Cannot write image format '%1'", mLastFormat.constData());	break;
case ImgSaver::SaveStatusProtocol:
        re = i18n("Cannot write using protocol '%1'", mLastUrl.protocol());	break;
case ImgSaver::SaveStatusCanceled:
        re = i18n("User cancelled saving");					break;
case ImgSaver::SaveStatusMkdir:
        re = i18n("Cannot create directory");					break;
case ImgSaver::SaveStatusUnknown:
        re = i18n("Save failed");						break;
case ImgSaver::SaveStatusParam:
        re = i18n("Bad parameter");						break;
default:
        re = i18n("Unknown status %1", status);					break;
    }
    return (re);
}


QString ImgSaver::tempSaveImage(const KookaImage *img, const ImageFormat &format, int colors)
{
    if (img==NULL) return (QString::null);

    KTemporaryFile tmpFile;
    tmpFile.setSuffix("."+format.extension());
    tmpFile.setAutoRemove(false);

    if (!tmpFile.open())
    {
        kDebug() << "Error opening temp file" << tmpFile.fileName();
        tmpFile.setAutoRemove(true);
        return (QString::null);
    }

    QString name = tmpFile.fileName();
    tmpFile.close();

    const KookaImage *tmpImg = NULL;
    if (colors!=-1 && img->depth()!=colors)		// Need to convert image
    {
        QImage::Format newfmt;
        switch (colors)
        {
case 1:     newfmt = QImage::Format_Mono;
            break;

case 8:     newfmt = QImage::Format_Indexed8;
            break;


case 24:    newfmt = QImage::Format_RGB888;
            break;


case 32:    newfmt = QImage::Format_RGB32;
            break;

default:    kDebug() << "Error: Bad colour depth requested" << colors;
            tmpFile.setAutoRemove(true);
            return (QString::null);
        }

        tmpImg = new KookaImage(img->convertToFormat(newfmt));
        img = tmpImg;
    }

    kDebug() << "Saving to" << name << "in format" << format;
    if (!img->save(name, format.name()))
    {
        kDebug() << "Error saving to" << name;
        tmpFile.setAutoRemove(true);
        name = QString::null;
    }

    if (tmpImg!=NULL) delete tmpImg;
    return (name);
}


bool copyRenameImage(bool isCopying, const KUrl &fromUrl, const KUrl &toUrl, bool askExt, QWidget *overWidget)
{
    QString errorString = QString::null;

    /* Check if the provided filename has a extension */
    QString extFrom = extension(fromUrl);
    QString extTo = extension(toUrl);

    KUrl targetUrl(toUrl);

    if (extTo.isEmpty() && !extFrom.isEmpty())
    {
        /* Ask if the extension should be added */
        int result = KMessageBox::Yes;
        QString fName = toUrl.fileName();
        if (!fName.endsWith( "." )) fName += ".";
        fName += extFrom;

        if (askExt) result = KMessageBox::questionYesNo(overWidget,
                                                        i18n("<qt><p>The file name you supplied has no file extension."
                                                             "<br>Should the original one be added?\n"
                                                             "<p>This would result in the new file name <filename>%1</filename>", fName),
                                                        i18n("Extension Missing"),
                                                        KGuiItem(i18n("Add Extension")),
                                                        KGuiItem(i18n("Do Not Add")),
                                                        "AutoAddExtensions");
        if (result==KMessageBox::Yes) targetUrl.setFileName(fName);
    }
    else if (!extFrom.isEmpty() && extFrom!=extTo)
    {
        KMimeType::Ptr fromType = KMimeType::findByUrl(fromUrl);
        KMimeType::Ptr toType = KMimeType::findByUrl(toUrl);
        if (!toType->is(fromType->name()))
        {
            errorString = "Changing the image format is not currently supported";
        }
    }

    if (errorString.isEmpty())				// no problem so far
    {
        kDebug() << (isCopying ? "Copy" : "Rename") << "->" << targetUrl;

        if (KIO::NetAccess::exists(targetUrl, KIO::NetAccess::DestinationSide, overWidget))
        {						// see if destination exists
            errorString = i18n("Target already exists");
        }
        else
        {
            bool success;
            if (isCopying) success = KIO::NetAccess::file_copy(fromUrl, targetUrl, overWidget);
            else success = KIO::NetAccess::move(fromUrl, targetUrl, overWidget);
            if (!success)				// copy/rename the file
            {
                errorString = KIO::NetAccess::lastErrorString();
            }
        }
    }

    if (!errorString.isEmpty())				// file operation error
    {
        QString msg = (isCopying ? i18n("Unable to copy the file") :
                                   i18n("Unable to rename the file"));
        QString title = (isCopying ? i18n("Error copying file") :
                                     i18n("Error renaming file"));
        KMessageBox::sorry(overWidget,i18n("<qt>"
                                           "<p>%1<br>"
                                           "<filename>%3</filename><br>"
                                           "<br>"
                                           "%2",
                                           msg,
                                           errorString,
                                           fromUrl.prettyUrl()), title);
        return (false);
    }

    return (true);					// file operation succeeded
}


bool ImgSaver::renameImage(const KUrl &fromUrl, const KUrl &toUrl, bool askExt, QWidget *overWidget)
{
    return (copyRenameImage(false, fromUrl, toUrl, askExt, overWidget));
}


bool ImgSaver::copyImage(const KUrl &fromUrl, const KUrl &toUrl, QWidget *overWidget)
{
    return (copyRenameImage(true, fromUrl, toUrl, true, overWidget));
}
