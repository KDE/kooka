/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2025      Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "destinationnull.h"

#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationNullFactory, "kookadestination-null.json", registerPlugin<DestinationNull>();)
#include "destinationnull.moc"


DestinationNull::DestinationNull(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationNull")
{
}


bool DestinationNull::scanStarting(ScanImage::ImageType type)
{
    qCDebug(DESTINATION_LOG) << "image type" << type;
    return (true);
}


bool DestinationNull::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();
    return (true);
}


KLocalizedString DestinationNull::scanDestinationString()
{
    return (ki18n("Discarding image"));
}


void DestinationNull::batchStart(const MultiScanOptions *opts)
{
    qCDebug(DESTINATION_LOG) << "batch options" << qPrintable(opts->toString());
}


void DestinationNull::batchEnd(bool ok)
{
    qCDebug(DESTINATION_LOG) << "batch ok?" << ok;
}


MultiScanOptions::Capabilities DestinationNull::capabilities() const
{
    return (MultiScanOptions::AcceptBatch);
}
