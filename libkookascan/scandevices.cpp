/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <freitag@suse.de>		*
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

#include "scandevices.h"

#include <klocalizedstring.h>

#include "scanglobal.h"
#include "scansettings.h"
#include "libkookascan_logging.h"



ScanDevices *ScanDevices::self()
{
    static ScanDevices *instance = new ScanDevices();
    return (instance);
}


ScanDevices::ScanDevices()
{
    qCDebug(LIBKOOKASCAN_LOG);

    if (!ScanGlobal::self()->init()) {
        return;    // do sane_init() if necessary
    }

    // Be ready to Implement the proxy option before querying for any scanners
    mUseNetworkProxy = ScanSettings::startupUseProxy();
    qCDebug(LIBKOOKASCAN_LOG) << "Use network proxy?" << mUseNetworkProxy;

    const bool netaccess = ScanSettings::startupOnlyLocal();
    qCDebug(LIBKOOKASCAN_LOG) << "Query for network scanners?" << netaccess;

    SANE_Device const **dev_list = nullptr;
    deactivateNetworkProxy();
    SANE_Status status = sane_get_devices(&dev_list, (netaccess ? SANE_TRUE : SANE_FALSE));
    reactivateNetworkProxy();
    if (status!=SANE_STATUS_GOOD)
    {
        qCWarning(LIBKOOKASCAN_LOG) << "sane_get_devices() failed, status" << status;
        return;						// no point carrying on
    }

    const SANE_Device *dev;
    for (int devno = 0; (dev = dev_list[devno]) != nullptr; ++devno) {
        mScannerNames.append(dev->name);
        mScannerDevices.insert(dev->name, dev);
        qCDebug(LIBKOOKASCAN_LOG) << "SANE found scanner:" << dev->name << "=" << deviceDescription(dev->name);
    }

    QString typeFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "libkookascan/scantypes.dat");
    qCDebug(LIBKOOKASCAN_LOG) << "Scanner type file" << typeFile;
    if (typeFile.isEmpty()) typeFile = "/dev/null";
    mTypeConfig = new KConfig(typeFile, KConfig::SimpleConfig);

    // TODO: handling the 3 lists is tedious, use a scanner data structure

    QStringList devs = ScanSettings::userDevices();
    if (!devs.isEmpty())
    {
        const int num = devs.count();

        QStringList descs = ScanSettings::userDescriptions();
        if (descs.count()<num)				// ensure list correct length
        {
            for (int i = descs.count(); i<num; ++i) descs.append("Unknown");
            ScanSettings::setUserDescriptions(descs);
            ScanSettings::self()->save();
        }

        QStringList types = ScanSettings::userTypes();
        if (types.count()<num)				// ensure list correct length
        {
            for (int i = types.count(); i<num; ++i) types.append("scanner");
            ScanSettings::setUserTypes(types);
            ScanSettings::self()->save();
        }

        QStringList manufs = ScanSettings::userManufacturers();
        if (manufs.count()<num)				// ensure list correct length
        {
            for (int i = manufs.count(); i<num; ++i) manufs.append("");
            ScanSettings::setUserManufacturers(manufs);
            ScanSettings::self()->save();
        }

        for (int i = 0; i<num; ++i)
        {
            const QByteArray name = devs[i].toLocal8Bit();
            if (mScannerNames.contains(name)) continue;	// avoid duplication
            addUserSpecifiedDevice(name, manufs[i], descs[i], types[i].toLocal8Bit(), true);
            qCDebug(LIBKOOKASCAN_LOG) << "Configured scanner:" << name << "=" << deviceDescription(name);
        }
    }
}

void ScanDevices::addUserSpecifiedDevice(const QByteArray &backend,
                                         const QString &manufacturer,
                                         const QString &description,
                                         const QByteArray &type,
                                         bool dontSave)
{
    if (backend.isEmpty()) {
        return;
    }

    if (mScannerNames.contains(backend)) {
        qCDebug(LIBKOOKASCAN_LOG) << "device" << backend << "already exists, not adding";
        return;
    }

    QByteArray devtype = (!type.isEmpty() ? type : "scanner");
    qCDebug(LIBKOOKASCAN_LOG) << "adding" << backend << "manuf" << manufacturer
                              << "desc" << description << "type" << devtype
                              << "dontSave" << dontSave;

    if (!dontSave)					// add new device to config
    {							// get existing device lists
        QStringList devs = ScanSettings::userDevices();
        QStringList manufs = ScanSettings::userManufacturers();
        QStringList descs = ScanSettings::userDescriptions();
        QStringList types = ScanSettings::userTypes();

        int i = devs.indexOf(backend);
        if (i >= 0) {					// see if already in list
            descs[i] = description;			// if so just update
            types[i] = devtype;
            manufs[i] = manufacturer;
        } else {
            devs.append(backend);			// add new entry to lists
            descs.append(description);
            types.append(devtype);
            manufs.append(manufacturer);
        }

        ScanSettings::setUserDevices(devs);
        ScanSettings::setUserManufacturers(manufs);
        ScanSettings::setUserDescriptions(descs);
        ScanSettings::setUserTypes(types);
        ScanSettings::self()->save();
    }

    SANE_Device *userdev = new SANE_Device;

    // Need a permanent copy of the strings, because SANE_Device only holds
    // pointers to them.  Unfortunately there is a memory leak here, the
    // 'userdev' object and its four QByteArray's are never deleted.
    // There is only a limited number of these objects in most applications,
    // so hopefully it won't matter too much.

    userdev->name = (new QByteArray(backend))->constData();
    userdev->model = (new QByteArray(description.toLocal8Bit()))->constData();
    userdev->type = (new QByteArray(devtype))->constData();

    QString s = manufacturer;
    if (s.isEmpty()) s = i18nc("Value used for manufacturer if none entered", "User specified");
    userdev->vendor = (new QByteArray(s.toLocal8Bit()))->constData();

    mScannerNames.append(backend);
    mScannerDevices.insert(backend, userdev);
}

const SANE_Device *ScanDevices::deviceInfo(const QByteArray &backend) const
{
    if (!mScannerNames.contains(backend)) {
        return (nullptr);
    }
    return (mScannerDevices[backend]);
}


QString ScanDevices::deviceDescription(const QByteArray &backend) const
{
    if (!mScannerNames.contains(backend)) return (i18n("Unknown device '%1'", backend.constData()));

    const SANE_Device *scanner = mScannerDevices[backend];
    QString result = QString::fromLocal8Bit(scanner->vendor);
    if (!result.isEmpty()) result += ' ';
    result += QString::fromLocal8Bit(scanner->model);
    return (result);
}


QString ScanDevices::deviceIconName(const QByteArray &backend)
{
    QString itemIcon = "scanner";
    QString devBase = QString(backend).section(':', 0, 0);
    QString ii = mTypeConfig->group("Devices").readEntry(devBase, "");
    qCDebug(LIBKOOKASCAN_LOG) << "for device" << devBase << "icon" << ii;
    if (!ii.isEmpty()) itemIcon = ii;
    else
    {
        // This is only possible if the backend is known, so that
        // the device type can be read from it.
        if (mScannerNames.contains(backend))
        {
            const SANE_Device *dev = mScannerDevices[backend];
            ii = typeIconName(dev->type);
            qCDebug(LIBKOOKASCAN_LOG) << "for type" << dev->type << "icon" << ii;
            if (!ii.isEmpty()) itemIcon = ii;
        }
    }

    return (itemIcon);
}


QString ScanDevices::typeIconName(const QByteArray &devType)
{
    return (mTypeConfig->group("Types").readEntry(devType, ""));
}


void ScanDevices::deactivateNetworkProxy()
{
    if (mUseNetworkProxy) return;			// want to use proxy, do nothing
    qCDebug(LIBKOOKASCAN_LOG);

    // Remove the proxy variables from the process environment.
    //
    // Doing this globally on startup causes a problem in that any processes
    // spawned from the main Kooka process, including any application invoked
    // by the scan destination plugin or "Open With", will also not have any
    // proxy settings.  Therefore the variables are not changed in the environment
    // until a SANE operation which will need network access is due to be
    // performed.  At this point the original settings of these variables are
    // read and saved, before they are cleared.  The saved settings are restored
    // afterwards.

    mSavedHttpProxy = qgetenv("http_proxy");
    mSavedHttpsProxy = qgetenv("https_proxy");
    mSavedFtpProxy = qgetenv("ftp_proxy");
    mSavedNoProxy = qgetenv("no_proxy");

    qunsetenv("http_proxy");
    qunsetenv("https_proxy");
    qunsetenv("ftp_proxy");
    qunsetenv("no_proxy");
}


void ScanDevices::reactivateNetworkProxy()
{
    if (mUseNetworkProxy) return;			// want to use proxy, do nothing
    qCDebug(LIBKOOKASCAN_LOG);

    qputenv("http_proxy", mSavedHttpProxy);
    qputenv("https_proxy", mSavedHttpsProxy);
    qputenv("ftp_proxy", mSavedFtpProxy);
    qputenv("no_proxy", mSavedNoProxy);
}
