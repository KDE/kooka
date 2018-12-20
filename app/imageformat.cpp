/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2009-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include "imageformat.h"

#include <qdebug.h>
#include <qmimetype.h>
#include <qmimedatabase.h>
#include <qimagereader.h>
#include <qimagewriter.h>

// This class provides two facilities:
//
// (a) The opaque ImageFormat data type, which is used throughout to represent
//     a Qt image format name.  It is a separate type from standard strings
//     so that it does not get confused with a MIME type (always passed
//     around as a QMimeType) or a file extension (always handled as a QString).
//
// (b) Its static functions provide some useful lookups not available from
//     the QImage* classes.
//
// In KDE Frameworks this class no longer uses KImageIO or any KDE service
// files, although it does make an assumption about the internal operation
// of QImage* - i.e. that a Qt format name is actually a file extension.


ImageFormat::ImageFormat(const QByteArray &format)
{
    mFormat = format.toUpper();
}

bool ImageFormat::isValid() const
{
    return (!mFormat.isEmpty());
}

const char *ImageFormat::name() const
{
    return (mFormat.constData());
}

ImageFormat::ImageFormat()
{
    mFormat.clear();
}

bool ImageFormat::operator==(const ImageFormat &other)
{
    return (mFormat == other.mFormat);
}

KOOKACORE_EXPORT QDebug operator<<(QDebug stream, const ImageFormat &format)
{
    stream.nospace() << "ImageFormat[" << format.name() << "]";
    return (stream.space());
}


QMimeType ImageFormat::mime() const
{
    // As noted previously and in FormatDialog::FormatDialog(), a Qt image
    // format name is really a file extension.  Therefore we can do this
    // lookup using a file name with that extension.
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(QString("a.")+mFormat, QMimeDatabase::MatchExtension);
    if (!mime.isValid() || mime.isDefault())
    {
        qDebug() << "no MIME type for image format" << *this;
    }
    else
    {
        //qDebug() << "for" << *this << "returning" << mime.name();
    }

    return (mime);
}


QString ImageFormat::extension() const
{
    QString suf = mFormat.toLower();
    QMimeType mp = mime();
    if (mp.isValid()) {
        QString ext = mp.preferredSuffix();
        if (!ext.isEmpty()) suf = ext;
    }

    if (suf.startsWith('.')) suf = suf.mid(1);
    //qDebug() << "for" << *this << "returning" << suf;
    return (suf);
}


bool ImageFormat::canWrite() const
{
    return (QImageWriter::supportedImageFormats().contains(mFormat.toLower()));
}


ImageFormat ImageFormat::formatForMime(const QMimeType &mime)
{
    if (!mime.isValid()) return (ImageFormat());

    // Before Qt 5.12 there was no official way to perform this operation;
    // that is, to find the Qt image format name corresponding to a MIME type.
    // So this does so by assuming that the format name is the first file
    // extension ("suffix") registered for that MIME type which is a
    // supported Qt image type.
    //
    // TODO: could now use QImageReader::imageFormatsForMimeType() in Qt 5.12

    QStringList sufs = mime.suffixes();
    if (sufs.isEmpty()) return (ImageFormat());

    const QList<QByteArray> formats = QImageReader::supportedImageFormats();
    foreach (const QString &suf, sufs)			// find the first matching one
    {
        const QByteArray s = suf.toLocal8Bit();
        if (formats.contains(s.toLower())) return (ImageFormat(s));
    }

#ifdef HAVE_TIFF
    // Qt 5:  This can happen if only QtGui is installed, because TIFF support is
    // now provided in QtImageFormats.  However, if we are built with TIFF library
    // support (for reading multi-page TIFF files) then TIFF reading is also available
    // via that library even if Qt does not support it.
    //
    // The special format name used here is checked by isTiff() below and used
    // in KookaImage::loadFromUrl() to force loading via the TIFF library.
    //
    // TIFF write support will still not be available unless QtImageFormats is
    // installed.
    if (mime.inherits("image/tiff")) return (ImageFormat("TIFFLIB"));
#endif
    return (ImageFormat());				// nothing matched
}


bool ImageFormat::isTiff() const
{
    if (mFormat=="TIF" || mFormat=="TIFF") return (true);
#ifdef HAVE_TIFF
    if (mFormat=="TIFFLIB") return (true);
#endif
    return (false);
}


ImageFormat ImageFormat::formatForUrl(const QUrl &url)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForUrl(url);
    return (formatForMime(mime));
}


static const QList<QMimeType> *sMimeList = nullptr;


void buildMimeTypeList()
{
    QList<QMimeType> *list = new QList<QMimeType>;

    // Each of the lists that we get from Qt (supported image formats and
    // supported MIME types) is sorted alphabetically.  That means that
    // there is no correlation between the two lists.  The formats list
    // may also contain duplicates, e.g. "jpeg" and "jpg" both correspond
    // to MIME type image/jpeg.
    //
    // However, it appears that the Qt image format is actually a file
    // name extension.  That means that we can obtain the MIME type by
    // finding the type for a file with that extension.

    QList<QByteArray> supportedFormats = QImageWriter::supportedImageFormats();
    //qDebug() << "have" << supportedFormats.count() << "image formats:" << supportedFormats;
    QList<QByteArray> supportedTypes = QImageWriter::supportedMimeTypes();
    //qDebug() << "have" << supportedTypes.count() << "mime types:" << supportedTypes;

    // Although the format list from standard Qt 5 (unlike Qt 4) does not
    // appear to contain any duplicates or mixed case, it is always possible
    // that a plugin could introduce some.  So the apparently pointless loop
    // here filters the list.
    QList<QByteArray> formatList;
    foreach (const QByteArray &format, supportedFormats)
    {
        const QByteArray f = format.toLower();
        if (!formatList.contains(f)) formatList.append(f);
    }
    qDebug() << "have" << formatList.count() << "image types"
             << "from" << supportedFormats.count() << "supported";

    // Even after filtering the list as above, there will be MIME type
    // duplicates (e.g. JPG and JPEG both map to image/jpeg and produce
    // the same results).  So the list is filtered again to eliminate
    // duplicate MIME types.
    //
    // As a side effect, this completely eliminates any formats that do
    // not have a defined MIME type.  None of those affected (currently
    // BW, RGBA, XV) seem to be of any use.

    QMimeDatabase db;
    foreach (const QByteArray &format, formatList)
    {
        QMimeType mime = db.mimeTypeForFile(QString("a.")+format, QMimeDatabase::MatchExtension);
        if (!mime.isValid() || mime.isDefault())
        {
            qDebug() << "No MIME type for format" << format;
            continue;
        }

        // ImageFormat::formatForMime() should now work even in the presence
        // of MIME aliases, but double check that it works at this stage.
        ImageFormat fmt = ImageFormat::formatForMime(mime);
        if (!fmt.isValid())
        {
            qDebug() << "MIME type" << mime.name() << "does not map back to format" << format;
            continue;
        }

        bool seen = false;
        foreach (const QMimeType &mt, *list)
        {
            if (mime.inherits(mt.name()))
            {
                seen = true;
                break;
            }
        }

        if (!seen) list->append(mime);
    }

    if (list->isEmpty()) qWarning() << "No MIME types available!";
    else qDebug() << "have" << list->count() << "unique MIME types";
    sMimeList = list;
}


const QList<QMimeType> *ImageFormat::mimeTypes()
{
    if (sMimeList==nullptr) buildMimeTypeList();
    return (sMimeList);
}
