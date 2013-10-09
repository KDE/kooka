/************************************************************************
 *									*
 *  This file is part of Kooka, a KDE scanning/OCR application.		*
 *									*
 *  Copyright (C) 2013 Jonathan Marten <jjm@keelhaul.me.uk>		*
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
 *  You should have received a copy of the GNU General Public License	*
 *  along with this program;  see the file COPYING.  If not, write to	*
 *  the Free Software Foundation, Inc., 51 Franklin Street,		*
 *  Fifth Floor, Boston, MA 02110-1301, USA.				*
 *									*
 ************************************************************************/

#ifndef IMAGETRANSFORM_H
#define IMAGETRANSFORM_H

#include <qthread.h>
#include <qimage.h>


class KUrl;
class KookaView;


class ImageTransform : public QThread
{
    Q_OBJECT

public:
    enum Operation
    {
        Rotate90,					// 90 degrees clockwise
        Rotate180,					// 180 degrees
        Rotate270,					// 90 degrees anticlockwise
        MirrorHorizontal,				// mirror horizontally
        MirrorVertical,					// mirror vertically
        MirrorBoth					// effectively same as Rotate180
    };

    ImageTransform(const QImage &img, ImageTransform::Operation op,
                   QString fileName, QObject *parent = NULL);
    virtual ~ImageTransform();

signals:
    void statusMessage(const QString &message);
    void done(const KUrl &imageUrl);

protected:
    virtual void run();

private:
    ImageTransform::Operation mOperation;
    QString mFileName;
    QImage mImage;
};

#endif							// IMAGETRANSFORM_H
