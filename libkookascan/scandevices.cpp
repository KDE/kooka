/* This file is part of the KDE Project
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>
   Copyright (C) 2009 Jonathan Marten <jjm@keelhaul.me.uk>

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

#include "scandevices.h"

#include <QDebug>
#include <KLocalizedString>
#include <kglobal.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include "scanglobal.h"

#define USERDEV_GROUP       "User Specified Scanners"
#define USERDEV_DEVS        "Devices"
#define USERDEV_DESC        "Description"
#define USERDEV_TYPE        "Type"

ScanDevices *sInstance = NULL;

ScanDevices *ScanDevices::self()
{
    if (sInstance == NULL) {
        sInstance = new ScanDevices();
    }
    return (sInstance);
}

ScanDevices::ScanDevices()
{
    //qDebug();

    if (!ScanGlobal::self()->init()) {
        return;    // do sane_init() if necessary
    }

    const KConfigGroup grp1 = ScanGlobal::self()->configGroup();
    bool netaccess = grp1.readEntry(STARTUP_ONLY_LOCAL, false);
    //qDebug() << "Query for network scanners" << (netaccess ? "not enabled" : "enabled");

    SANE_Device const **dev_list = NULL;
    SANE_Status status = sane_get_devices(&dev_list, (netaccess ? SANE_TRUE : SANE_FALSE));
    if (status != SANE_STATUS_GOOD) {
        //qDebug() << "sane_get_devices() failed, status" << status;
        return;                     // no point carrying on
    }

    const SANE_Device *dev;
    for (int devno = 0; (dev = dev_list[devno]) != NULL; ++devno) {
        mScannerNames.append(dev->name);
        mScannerDevices.insert(dev->name, dev);
        //qDebug() << "SANE found scanner:" << dev->name << "=" << deviceDescription(dev->name);
    }

    KConfigGroup grp2 = ScanGlobal::self()->configGroup(USERDEV_GROUP);
    QStringList devs = grp2.readEntry(USERDEV_DEVS, QStringList());
    if (devs.count() > 0) {
        QStringList descs = grp2.readEntry(USERDEV_DESC, QStringList());
        if (descs.count() < devs.count()) {     // ensure list corrent length
            for (int i = descs.count(); i < devs.count(); ++i) {
                descs.append("Unknown");
            }
            grp2.writeEntry(USERDEV_DESC, descs);
        }

        QStringList types = grp2.readEntry(USERDEV_TYPE, QStringList());
        if (types.count() < devs.count()) {     // ensure list correct length
            for (int i = types.count(); i < devs.count(); ++i) {
                types.append("scanner");
            }
            grp2.writeEntry(USERDEV_TYPE, types);
        }

        QStringList::const_iterator it2 = descs.constBegin();
        QStringList::const_iterator it3 = types.constBegin();
        for (QStringList::const_iterator it1 = devs.constBegin();
                it1 != devs.constEnd(); ++it1, ++it2, ++it3) {
            // avoid duplication
            QByteArray name = (*it1).toLocal8Bit();
            if (mScannerNames.contains(name)) {
                continue;
            }
            addUserSpecifiedDevice(name, (*it2), (*it3).toLocal8Bit(), true);
            //qDebug() << "Configured scanner:" << name << "=" << deviceDescription(name);
        }
    }
}

ScanDevices::~ScanDevices()
{
}

void ScanDevices::addUserSpecifiedDevice(const QByteArray &backend,
        const QString &description,
        const QByteArray &type,
        bool dontSave)
{
    if (backend.isEmpty()) {
        return;
    }

    if (mScannerNames.contains(backend)) {
        //qDebug() << "device" << backend << "already exists, not adding";
        return;
    }

    QByteArray devtype = (!type.isEmpty() ? type : "scanner");
    //qDebug() << "adding" << backend << "desc" << description
    //<< "type" << devtype << "dontSave" << dontSave;

    if (!dontSave) {                // add new device to config
        // get existing device lists
        KConfigGroup grp = ScanGlobal::self()->configGroup(USERDEV_GROUP);
        QStringList devs = grp.readEntry(USERDEV_DEVS, QStringList());
        QStringList descs = grp.readEntry(USERDEV_DESC, QStringList());
        QStringList types = grp.readEntry(USERDEV_TYPE, QStringList());

        int i = devs.indexOf(backend);
        if (i >= 0) {               // see if already in list
            descs[i] = description;         // if so just update
            types[i] = devtype;
        } else {
            devs.append(backend);           // add new entry to lists
            descs.append(description);
            types.append(devtype);
        }

        grp.writeEntry(USERDEV_DEVS, devs);
        grp.writeEntry(USERDEV_DESC, descs);
        grp.writeEntry(USERDEV_TYPE, types);
        grp.sync();
    }

    SANE_Device *userdev = new SANE_Device;

    // Need a permanent copy of the strings, because SANE_Device only holds
    // pointers to them.  Unfortunately there is a memory leak here, the
    // 'userdev' object and its three QByteArray's are never deleted.
    // There is only a limited number of these objects in most applications,
    // so hopefully it won't matter too much.

    userdev->name = *(new QByteArray(backend));
    userdev->model = *(new QByteArray(description.toLocal8Bit()));
    userdev->type = *(new QByteArray(devtype));
    userdev->vendor = "User specified";

    mScannerNames.append(backend);
    mScannerDevices.insert(backend, userdev);
}

const SANE_Device *ScanDevices::deviceInfo(const QByteArray &backend) const
{
    if (!mScannerNames.contains(backend)) {
        return (NULL);
    }
    return (mScannerDevices[backend]);
}

QString ScanDevices::deviceDescription(const QByteArray &backend) const
{
    if (!mScannerNames.contains(backend)) {
        return (i18n("Unknown device '%1'", backend.constData()));
    }

    const SANE_Device *scanner = mScannerDevices[backend];
    return (QString("%1 %2").arg(scanner->vendor, scanner->model));
}
