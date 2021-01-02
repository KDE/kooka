/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2013-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "imagetransform.h"

#include <qurl.h>

#include <klocalizedstring.h>
#include "kooka_logging.h"


ImageTransform::ImageTransform(const QImage &img, ImageTransform::Operation op,
                               const QString &fileName, QObject *parent)
    : QThread(parent)
{
    mImage = img;
    mOperation = op;
    mFileName = fileName;
    qCDebug(KOOKA_LOG) << "operation" << mOperation;
}


void ImageTransform::run()
{
    qCDebug(KOOKA_LOG) << "thread started for operation" << mOperation;

    QImage resultImg;
    QTransform m;

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
        qCWarning(KOOKA_LOG) << "Unknown operation" << mOperation;
        break;
    }

    if (resultImg.save(mFileName)) {
        emit done(QUrl::fromLocalFile(mFileName));
    } else {
        emit statusMessage(i18n("Error updating image %1", mFileName));
    }

    qCDebug(KOOKA_LOG) << "thread finished";
}
