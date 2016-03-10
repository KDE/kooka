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

bool ImageFormat::isValid() const
{
    return (!mFormat.isEmpty());
}

QByteArray ImageFormat::name() const
{
    return (mFormat);
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

static KService::Ptr service(const QByteArray &format)
{
    const KService::List services = KServiceTypeTrader::self()->query(
                                        "QImageIOPlugins",
                                        QString("'%1' ~in [X-KDE-ImageFormat]").arg(format.constData()));
    if (services.count() == 0) {
        //qDebug() << "no service found for image format" << *this;
        return (KService::Ptr());
    }

    return (services.first());
}

// Get the MIME type for a KImageIO (=Qt) image format.  This is the reverse
// operation to KImageIO::typeForMime(), which really ought to be provided by
// KImageIO but isn't.

QMimeType ImageFormat::mime() const
{
    KService::Ptr srv = service(mFormat);
    if (!srv) return (QMimeType());

    QString name = srv->property("X-KDE-MimeType").toString();
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(name);
    //if (!mime.isValid()) {
        //qDebug() << "no MIME type for image format" << *this;
    //}

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
    QMimeType mp = mime();
    if (mp.isValid()) {
        QString ext = mp.preferredSuffix();
        if (!ext.isEmpty()) suf = ext;
    }

    if (suf.startsWith('.')) suf = suf.mid(1);
    //qDebug() << "for" << *this << "returning" << suf;
    return (suf);
}

// Similar to KImageIO::isSupported(), but uses the service information
// directly instead of going to the MIME type and back again.

bool ImageFormat::canWrite() const
{
    KService::Ptr srv = service(mFormat);
    if (!srv) return (false);
    return (srv->property("X-KDE-Write").toBool());
}

// Similar to KImageIO::typeForMime(), but uses KMimeType::is() for the
// MIME type comparison instead of textual names.  See the top of this
// file for why.
//
// The X-KDE-ImageFormat property is a list of possible formats.
// Normally we assume that there will only be one, but possibly
// there may be more.  This lookup just takes the first one.

ImageFormat ImageFormat::formatForMime(const QMimeType &mime)
{
    if (!mime.isValid()) return (ImageFormat());

    QStringList formats;
    const KService::List services = KServiceTypeTrader::self()->query("QImageIOPlugins");
    foreach (const KService::Ptr &service, services) {
        if (mime.inherits(service->property("X-KDE-MimeType").toString())) {
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

ImageFormat ImageFormat::formatForUrl(const QUrl &url)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForUrl(url);
    return (formatForMime(mime));
}
