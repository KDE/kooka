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

#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocale.h>

#include "kscanoption.h"
#include "scanglobal.h"


// TODO: is this class more complex than it needs to be?
//
// It's used for (a) saving and loading option sets in the config files,
// either the default or explicitly named "Scan Parameters";  (b) backing
// up and restoring the current scan options while previewing.  Both
// of these just need to store option names and values - it should not
// be necessary to copy the entire KScanOption with its complications of
// deep copying or not, needing a KScanOption assignment operator, and
// the "strays" list maintained here.  Not having to do opt.get() or
// opt.set() here may, just possibly, go some way towards eliminating
// the KScanDevice global data.
//
// There's a memory leak here too!  KScanDevice uses a KScanOptSet to save
// the current scanner values while they are changed for previewing; this
// is allocated with 'new' the first time that it is required and reused
// subsequently.  Before each preview the set is clear()'ed which does indeed
// clear the saved options in it - but it doesn't delete them if they were
// allocated by us, in the second branch of backupOption().  So the
// strayCatsList will grow without bounds, and the options within it will
// stay around for ever.

#define SAVESET_GROUP		"Save Set"
#define SAVESET_KEY_SETDESC	"SetDesc"
#define SAVESET_KEY_SCANNER	"ScannerName"


// Debugging options
#define DEBUG_BACKUP


KScanOptSet::KScanOptSet(const QByteArray &setName)
{
    kDebug() << setName;

    mSetName = setName;
    mSetDescription = "";
}


KScanOptSet::~KScanOptSet()
{
    kDebug() << mSetName << strayCatsList.count() << "strays of" << count() << "total";
    qDeleteAll(strayCatsList);
    strayCatsList.clear();				/* deep copies from backupOption */
}


KScanOption *KScanOptSet::get(const QByteArray &optName) const
{
    return (value(optName));
}


QByteArray KScanOptSet::getValue(const QByteArray &optName) const
{
    const KScanOption *re = get(optName);

    QByteArray retStr;
    if (re!=NULL) retStr = re->get();
    else kDebug() << "option" << optName << "not available";
    return (retStr);
}


/** Allocate a new option and store it **/
bool KScanOptSet::backupOption(const KScanOption &opt)
{
    const QByteArray &optName = opt.getName();
#ifdef DEBUG_BACKUP
    kDebug() << "set" << mSetName << "option" << optName;
#endif // DEBUG_BACKUP
    bool retval = true;

    if (optName.isNull()) retval = false;
    else 
    {
        KScanOption *newopt = value(optName);
        if (newopt!=NULL)
        {
            /** The option already exists **/
            /* Copy the new one into the old one.
               TODO: checken Zuweisungoperatoren OK ?
               = check assignment operator OK? */
#ifdef DEBUG_BACKUP
            kDebug() << "already exists";
#endif // DEBUG_BACKUP
            *newopt = opt;
        }
        else
        {
#ifdef DEBUG_BACKUP
            const QByteArray &qq = opt.get();
            kDebug() << "new with value" << qq;
#endif // DEBUG_BACKUP
            KScanOption *newopt = new KScanOption(opt);
            if (newopt!=NULL)
            {
                insert(optName,newopt);
                strayCatsList.append(newopt);
            }
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
#ifdef DEBUG_BACKUP
    kDebug() << "set" << mSetName;
#endif // DEBUG_BACKUP

    for (ConstIterator it = src.constBegin(); it!=src.constEnd(); ++it)
    {
#ifdef DEBUG_BACKUP
        kDebug() << "option" << it.key();
#endif // DEBUG_BACKUP
        backupOption( *it.value());
    }
#ifdef DEBUG_BACKUP
    kDebug() << "done";
#endif // DEBUG_BACKUP
}


void KScanOptSet::saveConfig(const QString &scannerName,
                             const QString &setName,
                             const QString &desc) const
{
    kDebug() << "Saving set" << setName << "for scanner" << scannerName
             << "with" << count() << "options";

    QString cfgName = (!setName.isEmpty() ? setName : "default");
    KConfigGroup grp = ScanGlobal::self()->configGroup(QString("%1 %2").arg(SAVESET_GROUP, cfgName));
    grp.writeEntry(SAVESET_KEY_SETDESC, desc);
    grp.writeEntry(SAVESET_KEY_SCANNER, scannerName);

    for (KScanOptSet::const_iterator it = constBegin();
         it!=constEnd(); ++it)
    {
        KScanOption *so = (*it);
        if (!so->isReadable()) continue;

        const QString line = so->configLine();
        if (line!=PARAM_ERROR)
        {
            const QString optName = so->getName();
            kDebug() << optName << "=" << line;
            grp.writeEntry(optName,line);
        }
    }

    grp.sync();
    kDebug() << "done";
}


bool KScanOptSet::load(const QString &scannerName)
{
    kDebug() << "Reading into" << mSetName		// our name, as constructed
             << "-" << count() << "options"
             << strayCatsList.count() << "strays";

    QString grpName = QString("%1 %2").arg(SAVESET_GROUP, mSetName.constData());
    const KConfigGroup grp = ScanGlobal::self()->configGroup(grpName);
    if (!grp.exists())
    {
        kDebug() << "Group" << grpName << "does not exist in configuration!";
        return (false);
    }

    const StringMap emap = grp.entryMap();
    for (StringMap::const_iterator it = emap.constBegin();
         it!=emap.constEnd(); ++it)
    {
        QString optName = it.key().toLatin1();
        if (optName==SAVESET_KEY_SETDESC) continue;
        if (optName==SAVESET_KEY_SCANNER) continue;

        KScanOption optset(optName.toLatin1());
        if (!optset.isValid()) continue;
        if (!optset.isSoftwareSettable()) continue;

        QByteArray val = it.value().toLatin1();
        kDebug() << optName << "=" << val;

        optset.set(val);
        backupOption(optset);
    }

    kDebug() << "done"
             << "-" << count() << "options"
             << strayCatsList.count() << "strays";

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
