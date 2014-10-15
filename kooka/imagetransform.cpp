/************************************************************************
 *                                  *
 *  This file is part of Kooka, a KDE scanning/OCR application.     *
 *                                  *
 *  Copyright (C) 2013 Jonathan Marten <jjm@keelhaul.me.uk>     *
 *                                  *
 *  Kooka is free software; you can redistribute it and/or modify it    *
 *  under the terms of the GNU Library General Public License as    *
 *  published by the Free Software Foundation and appearing in the  *
 *  file COPYING included in the packaging of this file;  either    *
 *  version 2 of the License, or (at your option) any later version.    *
 *                                  *
 *  As a special exception, permission is given to link this program    *
 *  with any version of the KADMOS OCR/ICR engine (a product of     *
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting  *
 *  executable without including the source code for KADMOS in the  *
 *  source distribution.                        *
 *                                  *
 *  This program is distributed in the hope that it will be useful, *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of  *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   *
 *  GNU General Public License for more details.            *
 *                                  *
 *  You should have received a copy of the GNU General Public License   *
 *  along with this program;  see the file COPYING.  If not, write to   *
 *  the Free Software Foundation, Inc., 51 Franklin Street,     *
 *  Fifth Floor, Boston, MA 02110-1301, USA.                *
 *                                  *
 ************************************************************************/

#include "imagetransform.h"

#include <kurl.h>
#include <QDebug>
#include <klocale.h>

ImageTransform::ImageTransform(const QImage &img, ImageTransform::Operation op,
                               QString fileName, QObject *parent)
    : QThread(parent)
{
    mImage = img;
    mOperation = op;
    mFileName = fileName;
    //qDebug() << "for operation" << mOperation;
}

ImageTransform::~ImageTransform()
{
}

void ImageTransform::run()
{
    //qDebug() << "thread started for operation" << mOperation;

    QImage resultImg;
    QMatrix m;

    switch (mOperation) {
    case ImageTransform::Rotate90:
        emit statusMessage(i18n("Rotate image +90 degrees"));
        m.rotate(+90);
        resultImg = mImage.transformed(m);
        break;

    case ImageTransform::MirrorBoth:
    case ImageTransform::Rotate180:
        emit statusMessage(i18n("Rotate image 180 degrees"));
        resultImg = mImage.mirrored(true, true);
        break;

    case ImageTransform::Rotate270:
        emit statusMessage(i18n("Rotate image -90 degrees"));
        m.rotate(-90);
        resultImg = mImage.transformed(m);
        break;

    case ImageTransform::MirrorHorizontal:
        emit statusMessage(i18n("Mirror image horizontally"));
        resultImg = mImage.mirrored(true, false);
        break;

    case ImageTransform::MirrorVertical:
        emit statusMessage(i18n("Mirror image vertically"));
        resultImg = mImage.mirrored(false, true);
        break;

    default:
        //qDebug() << "Unknown operation" << mOperation;
        break;
    }

    if (resultImg.save(mFileName)) {
        emit done(KUrl(mFileName));
    } else {
        emit statusMessage(i18n("Error updating image %1", mFileName));
    }

    //qDebug() << "thread finished";
}
