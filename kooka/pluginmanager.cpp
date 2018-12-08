//////////////////////////////////////////////////////////////////////////

#include "pluginmanager.h"

#include <qpluginloader.h>
#include <qdir.h>
#include <qcoreapplication.h>
#include <qdebug.h>

#include <kservicetypetrader.h>

#include "abstractplugin.h"


static PluginManager *sInstance = NULL;


PluginManager::PluginManager()
{
    QStringList pluginPaths = QCoreApplication::libraryPaths();
    qDebug() << "initial paths" << pluginPaths;

    // Assume that the first path entry is the standard install location.
    // Our plugins will be in a subdirectory of that.
    Q_ASSERT(!pluginPaths.isEmpty());
    QString installPath = pluginPaths.takeFirst();
    pluginPaths.prepend(installPath+"/"+QCoreApplication::applicationName());

    // Also add a subdirectory of the executable directory,
    // for use when running in place.  In order to get the most up-to-date
    // plugins, this needs to have priority over all other install locations.
    pluginPaths.prepend(QCoreApplication::applicationDirPath()+"/ocr");

    // Put back the standard install location, for locating KParts and
    // other plugins which may be needed.
    pluginPaths.append(installPath);

    qDebug() << "using paths" << pluginPaths;
    QCoreApplication::setLibraryPaths(pluginPaths);
}


PluginManager *PluginManager::self()
{
    if (sInstance==NULL)
    {
        sInstance = new PluginManager();
        qDebug() << "allocated global instance";
    }
    return (sInstance);
}


AbstractPlugin *PluginManager::loadPlugin(PluginManager::PluginType type, const QString &name)
{
    qDebug() << "want type" << type << name;

    const KService::List list = KServiceTypeTrader::self()->query("Kooka/OcrPlugin",
                                                                  QString("[X-KDE-PluginInfo-Name]=='%1'").arg(name));
    qDebug() << "query count" << list.count();
    if (list.isEmpty())
    {
        qWarning() << "No plugin services found";
        return (nullptr);
    }

    if (list.count()>1) qWarning() << "Multiple plugin services found, using only the first";
							// should not happen, names are unique
    const KService::Ptr service = list.first();
    qDebug() << "  name" << service->name();
    qDebug() << "  icon" << service->icon();
    qDebug() << "  library" << service->library();

    KPluginLoader loader(*service);
    AbstractPlugin *plugin = loader.factory()->create<AbstractPlugin>();
    qDebug() << "loaded plugin" << plugin;

    if (plugin!=nullptr) plugin->mService = service;
    return (plugin);
}
