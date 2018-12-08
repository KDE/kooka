
#ifndef ABSTRACTPLUGIN_H
#define ABSTRACTPLUGIN_H

#include <qobject.h>

#include <kservice.h>

#include "kookacore_export.h"


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
    explicit AbstractPlugin(QObject *pnt);
    virtual ~AbstractPlugin() = default;

    const KService::Ptr pluginService() const			{ return (mService); }

private:
    friend class PluginManager;
    KService::Ptr mService;
};

#endif							// ABSTRACTPLUGIN_H
