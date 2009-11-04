
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

#ifndef IMAGEFORMAT_H
#define IMAGEFORMAT_H

#include <kurl.h>
#include <kmimetype.h>

class QDebug;


class ImageFormat
{

public:
    ImageFormat(const QByteArray &format);

    bool isValid() const { return (!mFormat.isEmpty()); }
    QByteArray name() const { return (mFormat); }

    KMimeType::Ptr mime() const;
    QString extension() const;
    bool canWrite() const;

    static ImageFormat formatForUrl(const KUrl &url);
    static ImageFormat formatForMime(const KMimeType::Ptr &mime);

    bool operator==(const ImageFormat &other);
    friend QDebug operator<<(QDebug stream, const ImageFormat &format);

protected:
    explicit ImageFormat();

private:

    QByteArray mFormat;
};


#endif							// IMAGEFORMAT_H
