/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2021      Jonathan Marten <jjm@keelhaul.me.uk>	*
 *  Based on the KIPI interface for Spectacle, which is:		*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>		*
 *  Copyright (C) 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>	*
 *  Essentially a rip-off of code for Kamoso by:			*
 *  Copyright (C) 2008-2009 by Aleix Pol <aleixpol@kde.org>		*
 *  Copyright (C) 2008-2009 by Alex Fiestas <alex@eyeos.org>		*
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

#include "exportkipiinterface.h"

#include <kfileitem.h>
#include <kio/previewjob.h>

#include "exportinfoshared.h"
#include "exportcollectionshared.h"
#include "exportcollectionselector.h"
#include "destination_logging.h"


ExportKipiInterface::ExportKipiInterface(QObject *pnt)
    : KIPI::Interface(pnt)
{
    qCDebug(DESTINATION_LOG);
}


void ExportKipiInterface::setUrl(const QUrl &url)
{
    qCDebug(DESTINATION_LOG) << url;
    mUrls.clear();
    mUrls.append(url);
}


void ExportKipiInterface::addUrl(const QUrl &url)
{
    qCDebug(DESTINATION_LOG) << url;
    mUrls.append(url);
}


// Request for a KIPI::ImageCollection which refers to the single image.
KIPI::ImageCollection ExportKipiInterface::currentAlbum()
{
    qCDebug(DESTINATION_LOG);
    return (KIPI::ImageCollection(new ExportCollectionShared(mUrls)));
}


// Request for a KIPI::ImageCollection which refers to the
// selected image.  Return the same as above.
KIPI::ImageCollection ExportKipiInterface::currentSelection()
{
    qCDebug(DESTINATION_LOG);
    return (currentAlbum());
}


// Request for a KIPI::ImageCollection listing all of the albums
// available.  Return the single collection.
QList<KIPI::ImageCollection> ExportKipiInterface::allAlbums()
{
    qCDebug(DESTINATION_LOG);

    QList<KIPI::ImageCollection> list;
    list.append(currentAlbum());
    return (list);
}


// Request for a KIPI::ImageInfo with information for the single image.
// Currently returns nothing useful.
KIPI::ImageInfo ExportKipiInterface::info(const QUrl &url)
{
    qCWarning(DESTINATION_LOG) << "KIPI::ImageInfo requested.  Needs to be implemented.";
    return (KIPI::ImageInfo(new ExportInfoShared(this, url)));
}


// Request for the features supported by this hosting application.
// Only image thumbnails are supported.
int ExportKipiInterface::features() const
{
    qCDebug(DESTINATION_LOG);
    return (KIPI::HostSupportsThumbnails);
}


// Request for a KIPI::ImageCollectionSelector.
// Currently returns nothing useful.
KIPI::ImageCollectionSelector *ExportKipiInterface::imageCollectionSelector(QWidget *pnt)
{
    qCWarning(DESTINATION_LOG) << "KIPI::ImageCollectionSelector requested.  Needs to be implemented.";
    return (new ExportCollectionSelector(this, pnt));
}


// Just return the base implementation.
KIPI::UploadWidget *ExportKipiInterface::uploadWidget(QWidget *pnt)
{
    return (new KIPI::UploadWidget(pnt));
}


// Needed, the original comment says "deal with api breakage".
KIPI::MetadataProcessor *ExportKipiInterface::createMetadataProcessor() const
{
    return (nullptr);
}


// Generate thumbnails for the specified images.  Assumed that this will
// only be requested for the current image URL (but this is neither required
// nor enforced).
void ExportKipiInterface::thumbnails(const QList<QUrl> &urls, int size)
{
    qDebug() << "requested size" << size << "for" << urls;

    KFileItemList items;
    for (const QUrl &url : urls)
    {
        items.append(KFileItem(url, KFileItem::SkipMimeTypeFromContent));
    }

    KIO::PreviewJob *job = KIO::filePreview(items, QSize(size, size));
    job->setIgnoreMaximumSize(true);
    job->setScaleType(KIO::PreviewJob::Scaled);
    connect(job, &KIO::PreviewJob::gotPreview, this, [this](const KFileItem &item, const QPixmap &preview)
    {
        qCDebug(DESTINATION_LOG) << "got preview for" << item.url();
        emit gotThumbnail(item.url(), preview);
    });

    connect(job, &KJob::finished, this, [job]()
    {
        if (job->error()) qCDebug(DESTINATION_LOG) << "PreviewJob error" << job->error();
    });

    job->start();
}
