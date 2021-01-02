/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2020      Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "destinationclipboard.h"

#include <qclipboard.h>
#include <qguiapplication.h>

#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationClipboardFactory, "kookadestination-clipboard.json", registerPlugin<DestinationClipboard>();)
#include "destinationclipboard.moc"


DestinationClipboard::DestinationClipboard(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationClipboard")
{
}


bool DestinationClipboard::scanStarting(ScanImage::ImageType type)
{
    return (true);					// no preparation, can always start
}


void DestinationClipboard::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();
    QGuiApplication::clipboard()->setImage(*img.data());
}


KLocalizedString DestinationClipboard::scanDestinationString()
{
    return (ki18n("Copying image to clipboard"));
}
