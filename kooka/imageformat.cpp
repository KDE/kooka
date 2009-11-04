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

#include "imageformat.h"

#include <qdebug.h>

#include <kdebug.h>
#include <kurl.h>
#include <kimageio.h>


// This class provides two facilities:
//
// (a) The opaque ImageFormat data type, which is used throughout to represent
//     a KImageIO/Qt image format name.  It is a separate type from standard
//     strings so that it does not get confused with a MIME type (always passed
//     around as a KMimeType::Ptr) or a file extension (always handled as
//     a QString).
// 
// (b) Its static functions provide some useful lookups not available from
//     KImageIO.


ImageFormat::ImageFormat(const QByteArray &format)
{
    mFormat = format.toUpper();
}


ImageFormat::ImageFormat()
{
    mFormat.clear();
}


bool ImageFormat::operator==(const ImageFormat &other)
{
    return (mFormat==other.mFormat);
}


QDebug operator<<(QDebug stream, const ImageFormat &format)
{
    stream.nospace() << "ImageFormat[" << format.name().constData() << "]";
    return (stream.space());
}


// Get the MIME type for a Qt image format.  This assumes that the
// KDE MIME type system will always recognise a file with the Qt
// image format name as extension.

KMimeType::Ptr ImageFormat::mime() const
{
    QString suf = mFormat.toLower();
    KMimeType::Ptr mime = KMimeType::findByPath("/tmp/x."+suf, 0, true);

    if (mime->isDefault())
    {
        kDebug() << "no MIME type for image format" << *this;
        return (KMimeType::Ptr());
    }
    else return (mime);
}


// KImageIO::suffix(format) seems to be no longer present, with no
// equivalent available.  The lookup here is to get the preferred
// file extension for the Qt image format.
//
// The dot is not included in the result, unlike KMimeType::mainExtension()

QString ImageFormat::extension() const
{
    QString suf = mFormat.toLower();
    KMimeType::Ptr mp = mime();
    if (!mp.isNull())
    {
        QString ext = mp->mainExtension();
        if (!ext.isEmpty()) suf = ext;
    }

    if (suf.startsWith('.')) suf = suf.mid(1);
    kDebug() << "for" << *this << "returning" << suf;
    return (suf);
}


// Wrapper for KImageIO::isSupported()

bool ImageFormat::canWrite() const
{
    KMimeType::Ptr mp = mime();
    if (mp.isNull()) return (false);
    return (KImageIO::isSupported(mp->name(), KImageIO::Writing));
}


// KImageIO::typeForMime() returns a list of possible formats.
// Normally we assume that there will only be one, but possibly
// there may be more.  This lookup just takes the first one.

ImageFormat ImageFormat::formatForMime(const KMimeType::Ptr &mime)
{
    if (mime.isNull()) return (ImageFormat());

    QStringList formats = KImageIO::typeForMime(mime->name());
    int fcount = formats.count();			// get format from file name
    if (fcount==0)					// no MIME/Qt type found
    {
        kDebug() << "no format found for MIME type" << mime->name();
        return (ImageFormat());
    }
    if (fcount>1)					// more than one type found
    {
        kDebug() << "found" << fcount << "formats for MIME type" << mime->name();
    }

    return (ImageFormat(formats.first().toLocal8Bit()));
}


ImageFormat ImageFormat::formatForUrl(const KUrl &url)
{
    KMimeType::Ptr mime = KMimeType::findByUrl(url, 0, true);
    return (formatForMime(mime));
}
