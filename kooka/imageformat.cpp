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

#include <QDebug>
#include <kurl.h>
#include <kservice.h>
#include <kservicetypetrader.h>

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
//
// Strangely, this class does not actually use KImageIO at all! - although the
// format dialogue, which is the source of the list of image types, does
// use KImageIO::types() to get the initial list.  The main reason for this
// is that the MIME type comparisons in KImageIO (kdelibs/kio/kio/kimageio.cpp)
// are all done using the textual MIME type name.  This doesn't work for MIME
// type aliases, and especially in the case where a format's QImageIOPlugins
// service file specifies a MIME type alias instead of the canonical name
// (e.g. JP2).  From the KMimeType::is() API documentation:
//
//   Do not use name()=="somename" anymore, to check for a given mimetype.
//   For mimetype inheritance to work, use is("somename") instead.
//
// So we query the services directly and use is() for MIME type comparison.
//
// TODO: fix KImageIO MIME type comparison, add missing KImageIO::mimeForType(),
// then eliminate the above and simplify this file.

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
    return (mFormat == other.mFormat);
}

QDebug operator<<(QDebug stream, const ImageFormat &format)
{
    stream.nospace() << "ImageFormat[" << format.name().constData() << "]";
    return (stream.space());
}

// Get the QImageIOPlugins service for this format.
//
// Note that the X-KDE-ImageFormat property can be a list with its entries
// in indeterminate case, therefore using the '~in' operator.

KService::Ptr ImageFormat::service() const
{
    const KService::List services = KServiceTypeTrader::self()->query(
                                        "QImageIOPlugins",
                                        QString("'%1' ~in [X-KDE-ImageFormat]").arg(mFormat.constData()));
    if (services.count() == 0) {
        //qDebug() << "no service found for image format" << *this;
        return (KService::Ptr());
    }

    return (services.first());
}

// Get the MIME type for a KImageIO (=Qt) image format.  This is the reverse
// operation to KImageIO::typeForMime(), which really ought to be provided by
// KImageIO but isn't.

KMimeType::Ptr ImageFormat::mime() const
{
    KService::Ptr srv = service();
    if (!srv) {
        return (KMimeType::Ptr());
    }

    QString name = srv->property("X-KDE-MimeType").toString();
    KMimeType::Ptr mime = KMimeType::mimeType(name);
    if (mime.isNull()) {
        //qDebug() << "no MIME type for image format" << *this;
        return (KMimeType::Ptr());
    }

    //qDebug() << "for" << *this << "returning" << mime->name();
    return (mime);
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
    if (!mp.isNull()) {
        QString ext = mp->mainExtension();
        if (!ext.isEmpty()) {
            suf = ext;
        }
    }

    if (suf.startsWith('.')) {
        suf = suf.mid(1);
    }
    //qDebug() << "for" << *this << "returning" << suf;
    return (suf);
}

// Similar to KImageIO::isSupported(), but uses the service information
// directly instead of going to the MIME type and back again.

bool ImageFormat::canWrite() const
{
    KService::Ptr srv = service();
    if (!srv) {
        return (false);
    }
    return (srv->property("X-KDE-Write").toBool());
}

// Similar to KImageIO::typeForMime(), but uses KMimeType::is() for the
// MIME type comparison instead of textual names.  See the top of this
// file for why.
//
// The X-KDE-ImageFormat property is a list of possible formats.
// Normally we assume that there will only be one, but possibly
// there may be more.  This lookup just takes the first one.

ImageFormat ImageFormat::formatForMime(const KMimeType::Ptr &mime)
{
    if (mime.isNull()) {
        return (ImageFormat());
    }

    QStringList formats;
    const KService::List services = KServiceTypeTrader::self()->query("QImageIOPlugins");
    foreach (const KService::Ptr &service, services) {
        if (mime->is(service->property("X-KDE-MimeType").toString())) {
            formats = service->property("X-KDE-ImageFormat").toStringList();
            break;
        }
    }

    int fcount = formats.count();           // how many formats found?
    if (fcount == 0) {              // no image format found
        ////qDebug() << "no format found for MIME type" << mime->name();
        return (ImageFormat());
    }
    if (fcount > 1) {               // more than one type found
        ////qDebug() << "found" << fcount << "formats for MIME type" << mime->name();
    }

    return (ImageFormat(formats.first().toLocal8Bit().trimmed()));
}

ImageFormat ImageFormat::formatForUrl(const KUrl &url)
{
    KMimeType::Ptr mime = KMimeType::findByUrl(url, 0, true);
    return (formatForMime(mime));
}
