//////////////////////////////////////////////////////////////////////////

#include "abstractplugin.h"


AbstractPlugin::AbstractPlugin(QObject *pnt)
    : QObject(pnt)
{
    mPluginInfo = nullptr;
}


AbstractPlugin::~AbstractPlugin()
{
    delete mPluginInfo;
}
