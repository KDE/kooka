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

#include "destinationexport.h"

#include <qcombobox.h>
#include <qaction.h>

#include <kpluginfactory.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>

#include <kio/global.h>

#include <kipi/plugin.h>

#include "scanparamspage.h"
#include "kookasettings.h"
#include "exportkipiinterface.h"
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationExportFactory, "kookadestination-export.json", registerPlugin<DestinationExport>();)
#include "destinationexport.moc"


DestinationExport::DestinationExport(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationExport")
{
    mDummyWidget = new QWidget;
    mKipiInterface = new ExportKipiInterface(this);
    mLoader = new KIPI::PluginLoader;
    mLoader->setInterface(mKipiInterface);
    mLoader->init();
}


DestinationExport::~DestinationExport()
{
    qCDebug(DESTINATION_LOG) << "have saved" << mSavedFiles.count() << "temporary files";
    for (const QUrl &savedFile : mSavedFiles)
    {
        delayedDelete(savedFile);
    }

    delete mLoader;
    delete mKipiInterface;
    delete mDummyWidget;
}


void DestinationExport::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();
    QAction *exportAction = mExportCombo->currentData().value<QAction *>();
    const QString mimeName = mFormatCombo->currentData().toString();
    qCDebug(DESTINATION_LOG) << "export" << exportAction->text() << "mime" << mimeName;

    ImageFormat fmt = getSaveFormat(mimeName, img);	// get format for saving image
    if (!fmt.isValid()) return;				// must have this now
    mSaveUrl = saveTempImage(fmt, img);			// save to temporary file
    if (!mSaveUrl.isValid()) return;			// could not save image
							// tell KIPI where it is
    static_cast<ExportKipiInterface *>(mKipiInterface)->setUrl(mSaveUrl);
    exportAction->trigger();				// do the export action

    mSavedFiles.append(mSaveUrl);
    // Record the temporary file.  There is nothing more to do here,
    // all temporary files will eventually be deleted by the destructor.
}


void DestinationExport::createGUI(ScanParamsPage *page)
{
    qCDebug(DESTINATION_LOG);

    // The MIME types that can be selected for sharing the image.
    QStringList mimeTypes;
    mimeTypes << "image/png" << "image/jpeg";
    mFormatCombo = createFormatCombo(mimeTypes, KookaSettings::destinationExportMime());
    page->addRow(i18n("Image format:"), mFormatCombo);

    // The available export destinations.
    mExportCombo = new QComboBox;
    const QString configuredExport = KookaSettings::destinationExportDest();
    int configuredIndex = -1;

    // Based on ExportMenu::loadKipiItems() in spectacle/src/Gui/ExportMenu.cpp 
    QList<QAction *> exportActions;
    const KIPI::PluginLoader::PluginList pluginList = mLoader->pluginList();
    qCDebug(DESTINATION_LOG) << "have" << pluginList.count() << "plugins";
    for (const auto &pluginInfo : pluginList)
    {
        if (!pluginInfo->shouldLoad()) continue;

        KIPI::Plugin *plugin = pluginInfo->plugin();
        if (plugin==nullptr)
        {
            qCWarning(DESTINATION_LOG) << "Failed to load KIPI plugin from library" << pluginInfo->library();
            continue;
        }

        plugin->setup(mDummyWidget);
        const QList<QAction *> actions = plugin->actions();
        for (QAction *action : actions)
        {
            KIPI::Category category = plugin->category(action);
            if (category==KIPI::ExportPlugin)
            {
                exportActions += action;
            }
            else if (category==KIPI::ImagesPlugin && pluginInfo->library().contains(QLatin1String("kipiplugin_sendimages")))
            {
                exportActions += action;
            }
        }
    }

    qCDebug(DESTINATION_LOG) << "have" << exportActions.count() << "export actions";
    const QString dots = i18nc("as added to action text", "...");
    for (QAction *action : qAsConst(exportActions))
    {
        QString text = KLocalizedString::removeAcceleratorMarker(action->text());
        if (text.endsWith(dots)) text.chop(dots.length());
        qCDebug(DESTINATION_LOG) << "action" << text;
        if (action->objectName()==configuredExport) configuredIndex = mExportCombo->count();
        mExportCombo->addItem(action->icon(), text, QVariant::fromValue<QAction *>(action));
    }

    if (configuredIndex!=-1) mExportCombo->setCurrentIndex(configuredIndex);
    page->addRow(i18n("Online service:"), mExportCombo);
}


KLocalizedString DestinationExport::scanDestinationString()
{
    return (kxi18n("<application>%1</application>").subs(mExportCombo->currentText()));
}


void DestinationExport::saveSettings() const
{
    QAction *exportAction = mExportCombo->currentData().value<QAction *>();
    KookaSettings::setDestinationExportDest(exportAction->objectName());
    KookaSettings::setDestinationExportMime(mFormatCombo->currentData().toString());
}
