/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "destinationgallery.h"

#include <qdebug.h>

#include <kpluginfactory.h>

#include "scangallery.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationGalleryFactory, "kookadestination-gallery.json", registerPlugin<DestinationGallery>();)
#include "destinationgallery.moc"


DestinationGallery::DestinationGallery(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationGallery")
{
    qDebug();
}


bool DestinationGallery::scanStarting(ScanImage::ImageType type)
{
    qDebug() << "type" << type;
    return (gallery()->prepareToSave(type));
}


void DestinationGallery::imageScanned(ScanImage::Ptr img)
{
    qDebug() << "image size" << img->size();
    gallery()->addImage(img);
}


QString DestinationGallery::scanDestinationString()
{
    return (gallery()->saveURL().url(QUrl::PreferLocalFile));
}
