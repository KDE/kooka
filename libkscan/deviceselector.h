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

#ifndef DEVSELECTOR_H
#define DEVSELECTOR_H

#include "libkscanexport.h"

#include <kdialog.h>


class QByteArray;
class QListWidget;
class QCheckBox;

class KGuiItem;


/**
 * @short A dialogue to allow the user to select a scan device.
 *
 * It is possible, depending on the application preferences and
 * the user's previous choices, that user interaction may not be
 * needed.  The dialogue should therefore be used in this way:
 *
 * @code
 * QByteArray selDevice;
 * DeviceSelector ds(parent, ScanDevices::self()->allDevices());
 * selDevice = ds.getDeviceFromConfig();
 * if (selDevice.isEmpty())
 * {
 *     if (ds.exec()==QDialog::Accepted) selDevice = ds.getSelectedDevice();
 * }
 * @endcode
 *
 * If the user checks the "Always use this device at startup" option,
 * this is stored in the global scanner configuration.
 *
 * @author Klaas Freitag
 * @author Eduard Huguet
 * @author Jonathan Marten
 **/

class KSCAN_EXPORT DeviceSelector : public KDialog
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param parent Parent widget.
     * @param backends a list of SANE device names.
     * @param cancelGuiItem GUI item for the "Cancel" button, if required
     * to replace the default.
     **/
    DeviceSelector(QWidget *parent, const QList<QByteArray> &backends, const KGuiItem &cancelGuiItem = KGuiItem());

    /**
     * Destructor.
     **/
    ~DeviceSelector();

    /**
     * Get the device that the user selected.
     *
     * @return The SANE device name
     **/
    QByteArray getSelectedDevice() const;

    /**
     * Check whether the user wants to skip this dialogue in future.
     *
     * @return @c true if the dialogue should be skipped
     **/
    bool getShouldSkip() const;

    /**
     * Get the selected device from the global scanner configuration file,
     * if there is a valid device and the user has requested that it
     * should be used by default.
     *
     * @return The SANE device name, or a null string if it is not
     * available or the user does not want to use it.
     **/
    QByteArray getDeviceFromConfig() const;

private:
    void setScanSources(const QList<QByteArray> &backends);

    QListWidget *mListBox;
    QCheckBox *mSkipCheckbox;

    QStringList mDeviceList;
};


#endif							// DEVSELECTOR_H
