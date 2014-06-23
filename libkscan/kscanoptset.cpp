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
#include "kscandevice.h"
#include "scansettings.h"


// Debugging options
#define DEBUG_BACKUP


// Mappings between a save set name and a configuration file group name
static QString groupName(const QString &setName)
{
    return (ScanSettings::self()->saveSetDescItem()->group()+" "+setName);
}


static QString groupSetName(const QString &grpName)
{
    QString prefix = ScanSettings::self()->saveSetDescItem()->group();
    if (!grpName.startsWith(prefix)) return (QString::null);
    return (grpName.mid(prefix.length()+1));
}


KScanOptSet::KScanOptSet(const QString &setName)
{
    mSetName = setName;
    mSetDescription = "";

    if (mSetName.isEmpty()) mSetName = "default";
    kDebug() << mSetName;
}


KScanOptSet::~KScanOptSet()
{
    kDebug() << mSetName << "with" << count() << "options";
}


QByteArray KScanOptSet::getValue(const QByteArray &optName) const
{
    return (value(optName));
}


bool KScanOptSet::backupOption(const KScanOption *opt)
{
    if (opt==NULL || !opt->isValid()) return (false);

    const QByteArray optName = opt->getName();
    if (optName.isNull())
    {
        kDebug() << "option has no name";
        return (false);
    }

    if (!opt->isReadable())
    {
        kDebug() << "option is not readable" << optName;
        return (false);
    }

    const QByteArray val = opt->get();
#ifdef DEBUG_BACKUP
    if (contains(optName)) kDebug() << "replace" << optName << "with value" << QString(val);
    else kDebug() << "add" << optName << "with value" << QString(val);
#endif // DEBUG_BACKUP
    insert(optName, val);
    return (true);
}


void KScanOptSet::setSetName(const QString &newName)
{
    kDebug() << "renaming" << mSetName << "->" << newName;
    mSetName = newName;
}


void KScanOptSet::setDescription(const QString &desc)
{
    mSetDescription = desc;
}


void KScanOptSet::saveConfig(const QByteArray &scannerName,
                             const QString &desc) const
{
    kDebug() << "Saving set" << mSetName << "for scanner" << scannerName
             << "with" << count() << "options";

    QString grpName = groupName(mSetName);
    KConfigGroup grp = KScanDevice::configGroup(grpName);
    grp.writeEntry(ScanSettings::self()->saveSetDescItem()->key(), desc);
    grp.writeEntry(ScanSettings::self()->saveSetScannerItem()->key(), scannerName);

    for (KScanOptSet::const_iterator it = constBegin();
         it!=constEnd(); ++it)
    {
        kDebug() << " " << it.key() << "=" << it.value();
        grp.writeEntry(QString(it.key()), it.value());
    }

    grp.sync();
    kDebug() << "done";
}


bool KScanOptSet::loadConfig(const QByteArray &scannerName)
{
    kDebug() << "Loading set" << mSetName << "for scanner" << scannerName;

    QString grpName = groupName(mSetName);
    const KConfigGroup grp = KScanDevice::configGroup(grpName);
    if (!grp.exists())
    {
        kDebug() << "Group" << grpName << "does not exist in configuration!";
        return (false);
    }

    const QMap<QString, QString> emap = grp.entryMap();
    for (QMap<QString, QString>::const_iterator it = emap.constBegin();
         it!=emap.constEnd(); ++it)
    {
        QString optName = it.key();
        if (optName==ScanSettings::self()->saveSetDescItem()->key()) continue;
							// ignore this as saved
        if (optName==ScanSettings::self()->saveSetScannerItem()->key())
        {						// check this but ignore
            if (!scannerName.isEmpty() && scannerName!=it.value())
            {
                kDebug() << "was saved for scanner" << it.value();
            }
            continue;
        }

        kDebug() << " " << it.key() << "=" << it.value();
        insert(it.key().toLatin1(), it.value().toLatin1());
    }

    kDebug() << "done with" << count() << "options";
    return (true);
}


KScanOptSet::StringMap KScanOptSet::readList()
{
    StringMap ret;

    const QStringList groups = ScanSettings::self()->config()->groupList();
    for (QStringList::const_iterator it = groups.constBegin(); it!=groups.constEnd(); ++it)
    {
        QString grp = (*it);
        QString set = groupSetName(grp);
        if (!set.isEmpty())
        {
            if (set==startupSetName()) continue;	// don't return this one
            kDebug() << "group" << grp  << "-> set" << set;
            const KConfigGroup g = KScanDevice::configGroup(grp);
            ret[set] = g.readEntry(ScanSettings::self()->saveSetDescItem()->key(), i18n("No description"));
        }
    }

    return (ret);
}


void KScanOptSet::deleteSet(const QString &setName)
{
    const QString grpName = groupName(setName);
    KConfig *conf = ScanSettings::self()->config();
    conf->deleteGroup(grpName);
    conf->sync();
}
