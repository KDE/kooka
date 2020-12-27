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

#include <qpluginloader.h>
#include <qdir.h>
#include <qcoreapplication.h>
#include <qdebug.h>

#include <kservicetypetrader.h>
#include <kplugininfo.h>
#include <klocalizedstring.h>

#include "abstractplugin.h"


static PluginManager *sInstance = nullptr;


PluginManager::PluginManager()
{
    QStringList pluginPaths = QCoreApplication::libraryPaths();
    qDebug() << "initial paths" << pluginPaths;

    // Assume that the first path entry is the standard install location.
    // Our plugins will be in a subdirectory of that.
    Q_ASSERT(!pluginPaths.isEmpty());
    QString installPath = pluginPaths.takeFirst();
    pluginPaths.prepend(installPath+"/"+QCoreApplication::applicationName());

    // Also add the plugin build directories, for use when running in place.
    // In order to get the most up-to-date plugins, these need to have priority
    // over all other install locations.
    QDir dir1(QCoreApplication::applicationDirPath()+"/../plugins");
    const QStringList subdirs1 = dir1.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    foreach (const QString &subdir1, subdirs1)
    {
        QDir dir2(QCoreApplication::applicationDirPath()+"/../plugins/"+subdir1);
        const QStringList subdirs2 = dir2.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
        foreach (const QString &subdir2, subdirs2)
        {
            const QString subdir = subdir1+"/"+subdir2;
            const QString path = subdir+"/Makefile";
            if (dir1.exists(path)) pluginPaths.prepend(dir1.absoluteFilePath(subdir));
        }
    }

    // Put back the standard install location, for locating KParts and
    // other plugins which may be needed.
    pluginPaths.append(installPath);

    qDebug() << "using paths" << pluginPaths;
    QCoreApplication::setLibraryPaths(pluginPaths);
}


PluginManager *PluginManager::self()
{
    if (sInstance==nullptr)
    {
        sInstance = new PluginManager();
        qDebug() << "allocated global instance";
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
case PluginManager::OcrPlugin:			return ("Kooka/OcrPlugin");
case PluginManager::DestinationPlugin:		return ("Kooka/DestinationPlugin");
default:					return ("Kooka/UnknownPlugin");
    }
}


AbstractPlugin *PluginManager::loadPlugin(PluginManager::PluginType type, const QString &name)
{
    qDebug() << "want type" << type << name;

    AbstractPlugin *plugin = mLoadedPlugins.value(type);
    if (plugin!=nullptr)				// a plugin is loaded
    {
        if (name==plugin->pluginInfo()->key)		// wanted plugin is already loaded
        {
            qDebug() << "already loaded";
            return (plugin);
        }

        qDebug() << "unloading current" << plugin->pluginInfo()->key;
        delete plugin;
        plugin = nullptr;
    }

    if (name.isEmpty())					// just want to unload current
    {
        mLoadedPlugins[type] = nullptr;			// note that nothing is loaded
        return (nullptr);				// no more to do
    }

    const KService::List list = KServiceTypeTrader::self()->query(pluginTypeString(type),
                                                                  QString("[DesktopEntryName]=='%1'").arg(name));
    qDebug() << "query count" << list.count();
    if (list.isEmpty()) qWarning() << "No plugin services found";
    else
    {
        if (list.count()>1) qWarning() << "Multiple plugin services found, using only the first";
							// should not happen, names are unique
        const KService::Ptr service = list.first();
        const QString lib = service->library();
        qDebug() << "  name" << service->name();
        qDebug() << "  icon" << service->icon();
        qDebug() << "  library" << lib;

        KPluginLoader loader(*service);
        if (loader.factory()==nullptr)
        {
            qWarning() << "Cannot load plugin library" << lib << "from service";
        }
        else
        {
            plugin = loader.factory()->create<AbstractPlugin>();
            if (plugin!=nullptr)
            {
                qDebug() << "created plugin from library" << lib;

                AbstractPluginInfo *info = new AbstractPluginInfo;
                info->key = service->desktopEntryName();
                info->name = service->name();
                info->icon = service->icon();
                info->description = commentAsRichText(service->comment());

                plugin->mPluginInfo = info;
            }
            else qWarning() << "Cannot create plugin from library" << lib;
        }
    }

    mLoadedPlugins[type] = plugin;
    return (plugin);
}


QMap<QString,AbstractPluginInfo> PluginManager::allPlugins(PluginManager::PluginType type) const
{
    qDebug() << "want all of type" << type;

    QMap<QString,AbstractPluginInfo> plugins;

    const KService::List list = KServiceTypeTrader::self()->query(pluginTypeString(type));
    qDebug() << "query count" << list.count();
    if (list.isEmpty()) qWarning() << "No plugin services found";
    else
    {
        foreach (const KService::Ptr service, qAsConst(list))
        {
            qDebug() << "  found" << service->desktopEntryName();

            struct AbstractPluginInfo info;
            info.key = service->desktopEntryName();
            info.name = service->name();
            info.icon = service->icon();
            info.description = commentAsRichText(service->comment());

            plugins[info.key] = info;
        }
    }

    return (plugins);
}


AbstractPlugin *PluginManager::currentPlugin(PluginManager::PluginType type) const
{
    return (mLoadedPlugins.value(type));
}
