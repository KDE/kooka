/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kscanoptset.h"

#include <qstring.h>
#include <q3asciidict.h>

#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocale.h>

//#include "kscandevice.h"
#include "kscanoption.h"
#include "scanglobal.h"



#define SAVESET_GROUP		"Save Set"
#define SAVESET_KEY_SETDESC	"SetDesc"
#define SAVESET_KEY_SCANNER	"ScannerName"


KScanOptSet::KScanOptSet(const QByteArray &setName)
{
    mSetName = setName;
    mSetDescription = "";
}


KScanOptSet::~KScanOptSet()
{
    kDebug() << "have" << strayCatsList.count() << "strays";
    qDeleteAll(strayCatsList);
    strayCatsList.clear();				/* deep copies from backupOption */
}


KScanOption *KScanOptSet::get(const QByteArray &optName) const
{
	return value(optName);
}


QByteArray KScanOptSet::getValue(const QByteArray &optName) const
{
    const KScanOption *re = get(optName);

    QByteArray retStr;
    if (re!=NULL) retStr = re->get();
    else kDebug() << "option" << optName << "not available";
    return (retStr);
}


bool KScanOptSet::backupOption(const KScanOption &opt)
{
    bool retval = true;

    /** Allocate a new option and store it **/
    const QByteArray &optName = opt.getName();
    if (optName.isNull()) retval = false;

    if (retval)
    {
		KScanOption *newopt = value(optName);

        if (newopt!=NULL)
        {
            /** The option already exists **/
            /* Copy the new one into the old one.
               TODO: checken Zuweisungoperatoren OK ?
               = check assignment operator OK? */
            *newopt = opt;
        }
        else
        {
            const QByteArray &qq = opt.get();
            kDebug() << "Value is now" << qq;
            KScanOption *newopt = new KScanOption(opt);
            strayCatsList.append(newopt);

            if (newopt!=NULL) insert(optName,newopt);
            else retval = false;
		}
    }

    return (retval);
}


void KScanOptSet::setDescription(const QString &desc)
{
    mSetDescription = desc;
}


void KScanOptSet::backupOptionDict(const KScanOptSet &src)
{
	ConstIterator it = src.begin();

	while (it != src.end())
    {
		kDebug() << "option" << it.key();
		backupOption( *it.value());
        ++it;
    }
}


void KScanOptSet::saveConfig(const QString &scannerName, const QString &setName,
                             const QString &desc)
{
    kDebug() << "scanner" << scannerName << "set" << setName;

    QString cfgName = (!setName.isEmpty() ? setName : "default");
    KConfigGroup grp = ScanGlobal::self()->configGroup(QString("%1 %2").arg(SAVESET_GROUP, cfgName));
    grp.writeEntry(SAVESET_KEY_SETDESC, desc);
    grp.writeEntry(SAVESET_KEY_SCANNER, scannerName);

    ConstIterator it = begin();
    while (it != end())
    {
        const QString line = it.value()->configLine();
        if (line!=PARAM_ERROR)
        {
            const QString optName = it.value()->getName();
            kDebug() << "writing" << optName << "=" << line;
            grp.writeEntry(optName,line);
        }
        ++it;
    }

    grp.sync();
    kDebug() << "done";
}


bool KScanOptSet::load(const QString &scannerName)
{
    kDebug() << "Reading" << mSetName;;			// of the KScanOptSet, as constructed

    QString grpName = QString("%1 %2").arg(SAVESET_GROUP, mSetName.constData());
    const KConfigGroup grp = ScanGlobal::self()->configGroup(grpName);
    if (!grp.exists())
    {
        kDebug() << "Group" << grpName << "does not exist in configuration!";
        return (false);
    }

    const StringMap strMap = grp.entryMap();
    for (StringMap::const_iterator it = strMap.constBegin(); it!=strMap.constEnd(); ++it)
    {
        QString optName = it.key().toLatin1();
        if (optName==SAVESET_KEY_SETDESC) continue;
        if (optName==SAVESET_KEY_SCANNER) continue;

        KScanOption optset(optName.toLatin1());

        QByteArray val = (*it).toLatin1();
        kDebug() << "read" << optName << "=" << val;

        optset.set(val);
        backupOption(optset);
    }

    kDebug() << "done";
    return (true);
}


KScanOptSet::StringMap KScanOptSet::readList()
{
    const QString groupName = SAVESET_GROUP;
    StringMap ret;

    const KConfigGroup grp = ScanGlobal::self()->configGroup(groupName);
    const QStringList groups = grp.config()->groupList();
    for (QStringList::const_iterator it = groups.constBegin(); it!=groups.constEnd(); ++it)
    {
        QString grp = (*it);
        kDebug() << "group" << grp;
        if (grp.startsWith(groupName))
        {
            QString set = grp.mid(groupName.length()+1);
            if (set==DEFAULT_OPTIONSET) continue;	// don't show this one

            const KConfigGroup g = ScanGlobal::self()->configGroup(grp);
            ret[set] = g.readEntry(SAVESET_KEY_SETDESC, i18n("No description"));
        }
    }

    return (ret);
}


void KScanOptSet::deleteSet(const QString &setName)
{
    const QString grpName = QString("%1 %2").arg(SAVESET_GROUP, setName);
    KConfig *conf = ScanGlobal::self()->configGroup().config();
    conf->deleteGroup(grpName);
    conf->sync();
}
