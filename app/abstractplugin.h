
#ifndef ABSTRACTPLUGIN_H
#define ABSTRACTPLUGIN_H

#include <qobject.h>

#include "kookacore_export.h"


struct KOOKACORE_EXPORT AbstractPluginInfo
{
    QString key;					// plugin unique key
    QString name;					// user visible name of plugin
    QString icon;					// icon name to accompany it
    QString description;				// comment, with rich text markup
};


/**
 * @short Base class of all plugins
 *
 *
 * @author Jonathan Marten
 **/

class AbstractPlugin;
Q_DECLARE_INTERFACE(AbstractPlugin, "org.kde.kooka.AbstractPlugin")


class KOOKACORE_EXPORT AbstractPlugin : public QObject
{
    Q_OBJECT
    Q_INTERFACES(AbstractPlugin)

public:
    const AbstractPluginInfo *pluginInfo() const	{ return (mPluginInfo); }

protected:
    // Only subclasses can use these to create and destroy plugin objects.
    explicit AbstractPlugin(QObject *pnt);
    virtual ~AbstractPlugin();

private:
    // Only the PluginManager can create, destroy and set information for plugins.
    friend class PluginManager;
    AbstractPluginInfo *mPluginInfo;
};

#endif							// ABSTRACTPLUGIN_H
