/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2018 Jonathan Marten <jjm@keelhaul.me.uk>		*
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

#include "pluginmanager.h"

#include <kpluginmetadata.h>
#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include "abstractplugin.h"
#include "kooka_logging.h"


static PluginManager *sInstance = nullptr;


PluginManager::PluginManager()
{
}


PluginManager *PluginManager::self()
{
    if (sInstance==nullptr)
    {
        sInstance = new PluginManager();
        qCDebug(KOOKA_LOG) << "allocated global instance";
    }
    return (sInstance);
}


static QString commentAsRichText(const QString &comment)
{
    // The 'comment' returned from KService is a QString which may have KUIT markup.
    // The conversion from that to a KLocalizedString, then back to a QString, actually
    // implements the KUIT markup.  The "@info" context ensures that the markup is
    // converted to rich text (HTML).
    //
    // There is no need to specify a translation domain, because the string from the
    // service desktop file will already have been translated.

    return (kxi18nc("@info", comment.toLocal8Bit().constData()).toString());
}


static QString pluginTypeString(PluginManager::PluginType type)
{
    switch (type)
    {
case PluginManager::OcrPlugin:			return ("ocr");
case PluginManager::DestinationPlugin:		return ("destination");
    }
    Q_UNREACHABLE();
    return QString();
}


AbstractPlugin *PluginManager::loadPlugin(PluginManager::PluginType type, const QString &name)
{
    qCDebug(KOOKA_LOG) << "want type" << type << name;

    AbstractPlugin *plugin = mLoadedPlugins.value(type);
    if (plugin!=nullptr)				// a plugin is loaded
    {
        if (name==plugin->pluginInfo()->key)		// wanted plugin is already loaded
        {
            qCDebug(KOOKA_LOG) << "already loaded";
            return (plugin);
        }

        qCDebug(KOOKA_LOG) << "unloading current" << plugin->pluginInfo()->key;
        delete plugin;
        plugin = nullptr;
    }

    if (name.isEmpty())					// just want to unload current
    {
        mLoadedPlugins[type] = nullptr;			// note that nothing is loaded
        return (nullptr);				// no more to do
    }

    KPluginMetaData md(QStringLiteral("kooka_") + pluginTypeString(type) + QStringLiteral("/") + name);

    plugin = KPluginFactory::instantiatePlugin<AbstractPlugin>(md).plugin;

    if (plugin!=nullptr)
    {
        qCDebug(KOOKA_LOG) << "created plugin from library" << md.fileName();

        AbstractPluginInfo *info = new AbstractPluginInfo;
        info->key = md.pluginId();
        info->name = md.name();
        info->icon = md.iconName();
        info->description = commentAsRichText(md.description());

        plugin->mPluginInfo = info;
    }
    else qCWarning(KOOKA_LOG) << "Cannot create plugin from library" << md.fileName();

    mLoadedPlugins[type] = plugin;
    return (plugin);
}


QMap<QString,AbstractPluginInfo> PluginManager::allPlugins(PluginManager::PluginType type) const
{
    qCDebug(KOOKA_LOG) << "want all of type" << type;

    QMap<QString,AbstractPluginInfo> plugins;

    const QVector<KPluginMetaData> list = KPluginMetaData::findPlugins(QStringLiteral("kooka_") + pluginTypeString(type));

    qCDebug(KOOKA_LOG) << "query count" << list.count();
    if (list.isEmpty()) qCWarning(KOOKA_LOG) << "No plugin services found";
    else
    {
        for (const KPluginMetaData &service : list)
        {
            qCDebug(KOOKA_LOG) << "  found" << service.pluginId();

            struct AbstractPluginInfo info;
            info.key = service.pluginId();
            info.name = service.name();
            info.icon = service.iconName();
            info.description = commentAsRichText(service.description());

            plugins[info.key] = info;
        }
    }

    return (plugins);
}


AbstractPlugin *PluginManager::currentPlugin(PluginManager::PluginType type) const
{
    return (mLoadedPlugins.value(type));
}
