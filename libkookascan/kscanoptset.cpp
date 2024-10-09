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

#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocalizedstring.h>

#include "kscanoption.h"
#include "kscandevice.h"
#include "scansettings.h"
#include "libkookascan_logging.h"


// Debugging options
#undef DEBUG_BACKUP


static QString prefixForCategory(KScanOptSet::Category category)
{
    switch (category)
    {
case KScanOptSet::Params:	return ("Parameters");
case KScanOptSet::Options:	return ("Options");
case KScanOptSet::Preset:	return ("Preset");
default:			qCWarning(LIBKOOKASCAN_LOG) << "unknown category" << category;
				return ("Unknown");
    }
}


// Mapping from a save set name and category to a configuration file group name
static QString groupForSet(const QString &setName, KScanOptSet::Category category)
{
    return (prefixForCategory(category)+' '+setName);
}


KScanOptSet::KScanOptSet(const QString &setName)
{
    mSetName = setName;
    mSetDescription = "";

    if (mSetName.isEmpty()) mSetName = "default";
    qCDebug(LIBKOOKASCAN_LOG) << mSetName;
}


bool KScanOptSet::backupOption(const KScanOption *opt)
{
    if (opt == nullptr || !opt->isValid()) {
        return (false);
    }

    const QByteArray optName = opt->getName();
    if (optName.isNull()) {
        qCDebug(LIBKOOKASCAN_LOG) << "option has no name";
        return (false);
    }

    if (!opt->isReadable()) {
        qCDebug(LIBKOOKASCAN_LOG) << "option is not readable" << optName;
        return (false);
    }

    const QByteArray val = opt->get();
#ifdef DEBUG_BACKUP
    if (contains(optName)) qCDebug(LIBKOOKASCAN_LOG) << "replace" << optName << "with value" << val;
    else qCDebug(LIBKOOKASCAN_LOG) << "add" << optName << "with value" << val;
#endif // DEBUG_BACKUP
    insert(optName, val);
    return (true);
}

void KScanOptSet::setSetName(const QString &newName)
{
    qCDebug(LIBKOOKASCAN_LOG) << "renaming" << mSetName << "->" << newName;
    mSetName = newName;
}


bool KScanOptSet::saveConfig(KScanOptSet::Category category, const QByteArray &scannerName) const
{
    KConfigGroup grp = configGroup(mSetName, category);

    qCDebug(LIBKOOKASCAN_LOG) << "Saving to group" << grp.name() << "for scanner" << scannerName;

    grp.writeEntry(ScanSettings::self()->saveSetDescItem()->key(), mSetDescription);
    grp.writeEntry(ScanSettings::self()->saveSetScannerItem()->key(), scannerName);

    for (KScanOptSet::const_iterator it = constBegin();
            it != constEnd(); ++it) {
        grp.writeEntry(QString(it.key()), it.value());
    }

    grp.sync();
    qCDebug(LIBKOOKASCAN_LOG) << "done with" << count() << "options";
    return (true);
}


bool KScanOptSet::loadConfig(KScanOptSet::Category category, const QByteArray &scannerName)
{
    const KConfigGroup grp = configGroup(mSetName, category);
    if (!grp.exists())
    {
        qCDebug(LIBKOOKASCAN_LOG) << "No such group" << grp.name();
        return (false);
    }

    qCDebug(LIBKOOKASCAN_LOG) << "Loading from group" << grp.name() << "for scanner" << scannerName;

    const QMap<QString, QString> emap = grp.entryMap();
    for (QMap<QString, QString>::const_iterator it = emap.constBegin();
         it != emap.constEnd(); ++it)
    {
        const QString optName = it.key();
        const QByteArray optValue = it.value().toLatin1();
        if (optName==ScanSettings::self()->saveSetDescItem()->key())
        {						// restore this as saved
            mSetDescription = optValue;
        }
        else if (optName==ScanSettings::self()->saveSetScannerItem()->key())
        {						// check this but ignore
            if (!scannerName.isEmpty() && scannerName!=optValue)
            {
                qCDebug(LIBKOOKASCAN_LOG) << "was saved for scanner" << optValue;
            }
        }
        else insert(optName.toLatin1(), optValue);
    }

    qCDebug(LIBKOOKASCAN_LOG) << "done with" << count() << "options";
    return (true);
}


// Used by the "Scan Presets" dialogue.
QStringList KScanOptSet::listSavedSets(KScanOptSet::Category category)
{
    QStringList ret;

    ScanSettings::self()->load();			// ensure refreshed

    // Do not use ScanSettings::self()->config() directly here,
    // see ScanGlobal::init().
    const KConfig *globalConfig = ScanSettings::self()->config();
    const KSharedConfig::Ptr conf = KSharedConfig::openConfig(globalConfig->name(), KConfig::SimpleConfig);

    const QString prefix = prefixForCategory(category)+' ';
    const QStringList groupList = conf->groupList();
    for (const QString &grpName : std::as_const(groupList))
    {
        if (!grpName.startsWith(prefix)) continue;

        const QString setName = grpName.mid(prefix.length());
        if (!setName.isEmpty())
        {
            qCDebug(LIBKOOKASCAN_LOG) << "found group" << grpName  << "-> set" << setName;
            ret.append(setName);
        }
    }

    return (ret);
}


void KScanOptSet::deleteSet(const QString &setName, KScanOptSet::Category category)
{
    const QString grpName = groupForSet(setName, category);
    qCDebug(LIBKOOKASCAN_LOG) << grpName;
    KConfig *conf = ScanSettings::self()->config();
    conf->deleteGroup(grpName);
    conf->sync();
}


KConfigGroup KScanOptSet::configGroup(const QString &groupName, KScanOptSet::Category category)
{
    Q_ASSERT(!groupName.isEmpty());
    return (ScanSettings::self()->config()->group(groupForSet(groupName, category)));
}
