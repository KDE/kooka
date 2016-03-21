/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "kscanoptset.h"

#include <qstring.h>
#include <qdebug.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocalizedstring.h>

#include "kscanoption.h"
#include "kscandevice.h"
#include "scansettings.h"


// Debugging options
#undef DEBUG_BACKUP


// Mappings between a save set name and a configuration file group name
static QString groupForSet(const QString &setName)
{
    return (ScanSettings::self()->saveSetDescItem()->group()+" "+setName);
}


static QString setNameFromGroup(const QString &grpName)
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
    //qDebug() << mSetName;
}

KScanOptSet::~KScanOptSet()
{
    //qDebug() << mSetName << "with" << count() << "options";
}

QByteArray KScanOptSet::getValue(const QByteArray &optName) const
{
    return (value(optName));
}

bool KScanOptSet::backupOption(const KScanOption *opt)
{
    if (opt == NULL || !opt->isValid()) {
        return (false);
    }

    const QByteArray optName = opt->getName();
    if (optName.isNull()) {
        //qDebug() << "option has no name";
        return (false);
    }

    if (!opt->isReadable()) {
        //qDebug() << "option is not readable" << optName;
        return (false);
    }

    const QByteArray val = opt->get();
#ifdef DEBUG_BACKUP
    if (contains(optName)) qDebug() << "replace" << optName << "with value" << QString(val);
    else qDebug() << "add" << optName << "with value" << QString(val);
#endif // DEBUG_BACKUP
    insert(optName, val);
    return (true);
}

void KScanOptSet::setSetName(const QString &newName)
{
    //qDebug() << "renaming" << mSetName << "->" << newName;
    mSetName = newName;
}

void KScanOptSet::setDescription(const QString &desc)
{
    mSetDescription = desc;
}

void KScanOptSet::saveConfig(const QByteArray &scannerName,
                             const QString &desc) const
{
    //qDebug() << "Saving set" << mSetName << "for scanner" << scannerName
    //<< "with" << count() << "options";

    QString grpName = groupForSet(mSetName);
    KConfigGroup grp = KScanDevice::configGroup(grpName);
    grp.writeEntry(ScanSettings::self()->saveSetDescItem()->key(), desc);
    grp.writeEntry(ScanSettings::self()->saveSetScannerItem()->key(), scannerName);

    for (KScanOptSet::const_iterator it = constBegin();
            it != constEnd(); ++it) {
        //qDebug() << " " << it.key() << "=" << it.value();
        grp.writeEntry(QString(it.key()), it.value());
    }

    grp.sync();
    //qDebug() << "done";
}

bool KScanOptSet::loadConfig(const QByteArray &scannerName)
{
    QString grpName = groupForSet(mSetName);
    const KConfigGroup grp = KScanDevice::configGroup(grpName);
    if (!grp.exists()) {
        //qDebug() << "Group" << grpName << "does not exist in configuration!";
        return (false);
    }

    //qDebug() << "Loading set" << mSetName << "for scanner" << scannerName;

    const QMap<QString, QString> emap = grp.entryMap();
    for (QMap<QString, QString>::const_iterator it = emap.constBegin();
            it != emap.constEnd(); ++it) {
        QString optName = it.key();
        if (optName==ScanSettings::self()->saveSetDescItem()->key()) continue;
							// ignore this as saved
        if (optName==ScanSettings::self()->saveSetScannerItem()->key())
        {						// check this but ignore
            if (!scannerName.isEmpty() && scannerName!=it.value())
            {
                //qDebug() << "was saved for scanner" << it.value();
            }
            continue;
        }

        //qDebug() << " " << it.key() << "=" << it.value();
        insert(it.key().toLatin1(), it.value().toLatin1());
    }

    //qDebug() << "done with" << count() << "options";
    return (true);
}

KScanOptSet::StringMap KScanOptSet::readList()
{
    StringMap ret;

    ScanSettings::self()->load();			// ensure refreshed
    KConfig *conf = ScanSettings::self()->config();
    QStringList groups = conf->groupList();
    //groups.sort(); qDebug() << groups;
    foreach (const QString &grp, groups)
    {
        QString set = setNameFromGroup(grp);
        if (!set.isEmpty())
        {
            if (set==startupSetName()) continue;	// don't return this one
            qDebug() << "found group" << grp  << "-> set" << set;
            const KConfigGroup g = KScanDevice::configGroup(grp);
            ret[set] = g.readEntry(ScanSettings::self()->saveSetDescItem()->key(), i18n("No description"));
        }
    }

    return (ret);
}

void KScanOptSet::deleteSet(const QString &setName)
{
    const QString grpName = groupForSet(setName);
    //qDebug() << grpName;
    KConfig *conf = ScanSettings::self()->config();
    conf->deleteGroup(grpName);
    conf->sync();
}