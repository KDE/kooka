/* This file is part of the KDE Project         -*- mode:c++; -*-
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

#ifndef SCANDEVICES_H
#define SCANDEVICES_H

#include "kookascan_export.h"

#include <qbytearray.h>
#include <qlist.h>
#include <qhash.h>

extern "C" {
#include <sane/sane.h>                  // to define SANE_Device
}


class KConfig;


class KOOKASCAN_EXPORT ScanDevices
{
public:
    static ScanDevices *self();

    /**
     *  returns the names of all existing Scan Devices in the system.
     */
    const QList<QByteArray> &allDevices() const
    {
        return (mScannerNames);
    }

    /**
     *  returns the SANE device information for a Scan Device.
     */
    const SANE_Device *deviceInfo(const QByteArray &backend) const;

    /**
     *  returns a readable device description for a Scan Device.
     */
    QString deviceDescription(const QByteArray &backend) const;

    /**
     * Add an explicitly specified device to the list of known ones.
     *
     * @param backend the device name+parameters of the backend to add
     * @param manufacturer the device's manufacturer name
     * @param description a readable description for it
     * @param type the device type name
     * @param dontSave if @c true, don't save the new device in the permanent configuration
     */
    void addUserSpecifiedDevice(const QByteArray &backend,
                                const QString &manufacturer,
                                const QString &description,
                                const QByteArray &type = "",
                                bool dontSave = false);

    QString deviceIconName(const QByteArray &backend);
    QString typeIconName(const QByteArray &devType);

    void deactivateNetworkProxy();
    void reactivateNetworkProxy();

private:
    explicit ScanDevices();
    ~ScanDevices() = default;

private:
    QList<QByteArray> mScannerNames;
    QHash<QByteArray, const SANE_Device *> mScannerDevices;
    KConfig *mTypeConfig;

    bool mUseNetworkProxy;
    QByteArray mSavedHttpProxy;
    QByteArray mSavedHttpsProxy;
    QByteArray mSavedFtpProxy;
    QByteArray mSavedNoProxy;
};

#endif                          // SCANDEVICES_H
