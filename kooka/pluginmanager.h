
#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <kservice.h>

#include "kookacore_export.h"


class QObject;
class AbstractPlugin;


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
     * @param name The name of the plugin
     * @return A loaded plugin instance object of that type,
     * or @c nullptr if no plugin could be found or loaded.
     **/
    AbstractPlugin *loadPlugin(PluginManager::PluginType type, const QString &name);

private:
    PluginManager();
    ~PluginManager() = default;
};

#endif							// PLUGINMANAGER_H
