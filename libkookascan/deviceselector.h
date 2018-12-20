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

#ifndef DEVICESELECTOR_H
#define DEVICESELECTOR_H

#include "kookascan_export.h"

#include "dialogbase.h"

#include <kguiitem.h>

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
 * DeviceSelector ds(parent, ScanDevices::self()->allDevices());
 * QByteArray selDevice = ds.getDeviceFromConfig();
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

class KOOKASCAN_EXPORT DeviceSelector : public DialogBase
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param pnt Parent widget.
     * @param backends a list of SANE device names.
     * @param cancelGuiItem GUI item for the "Cancel" button, if required
     * to replace the default.
     **/
    explicit DeviceSelector(QWidget *pnt, const QList<QByteArray> &backends, const KGuiItem &cancelGuiItem = KGuiItem());

    /**
     * Destructor.
     **/
    virtual ~DeviceSelector();

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

#endif							// DEVICESELECTOR_H
