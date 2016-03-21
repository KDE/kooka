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

    // Assuming that the format name is the first file extension ("suffix")
    // registered for that MIME type which is a supported Qt image type.

    QStringList sufs = mime.suffixes();
    if (sufs.isEmpty()) return (ImageFormat());

    foreach (const QString &suf, sufs)			// find the first matching one
    {
        const QByteArray s = suf.toLocal8Bit();
        if (QImageReader::supportedImageFormats().contains(s.toLower())) return (ImageFormat(s));
    }
    return (ImageFormat());				// nothing matched
}


ImageFormat ImageFormat::formatForUrl(const QUrl &url)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForUrl(url);
    return (formatForMime(mime));
}
