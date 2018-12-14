
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
        OcrPlugin
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
