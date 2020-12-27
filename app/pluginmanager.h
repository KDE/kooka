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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "kookacore_export.h"

#include <qstring.h>
#include <qmap.h>


class AbstractPlugin;
class AbstractPluginInfo;


/**
 * @short Manages plugins
 *
 * @author Jonathan Marten
 **/

class KOOKACORE_EXPORT PluginManager
{
public:
    enum PluginType
    {
        OcrPlugin,
        DestinationPlugin
    };

    /**
     * Get the singleton instance, creating it if necessary.
     *
     * @return the instance
     **/
    static PluginManager *self();

    /**
     * Load and create an instance object for a plugin of the specified type.
     *
     * @param type The type of the plugin required
     * @param name The name of the plugin, or a null string to unload the plugin
     * @return A loaded plugin instance object of that type,
     * or @c nullptr if the specified @p name is null or if no plugin could
     * be found or loaded.
     *
     * @note Only one plugin of a particular type can be loaded;  if a plugin
     * of the specified type already exists then it is unloaded and deleted.
     *
     * If a plugin is successfully loaded, it is available via @c currentPlugin().
     **/
    AbstractPlugin *loadPlugin(PluginManager::PluginType type, const QString &name);

    /**
     * Enumerate (but do not load) all of the available plugins of a specified type.
     *
     * @param type The type of the plugins required
     * @return A map (name -> information) of all the plugins
     **/
    QMap<QString,AbstractPluginInfo> allPlugins(PluginManager::PluginType type) const;

    /**
     * Get the currently loaded instance of a plugin of the specified type.
     *
     * @param type The type of the plugin required
     * @return The loaded plugin instance object of that type,
     * or @c nullptr if no plugin of that type is currently loaded
     **/
    AbstractPlugin *currentPlugin(PluginManager::PluginType type) const;

private:
    PluginManager();
    ~PluginManager() = default;

    QMap<PluginManager::PluginType,AbstractPlugin *> mLoadedPlugins;
};

#endif							// PLUGINMANAGER_H
