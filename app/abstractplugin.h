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
