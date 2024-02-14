/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2008-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#ifndef ADDDEVICEDIALOG_H
#define ADDDEVICEDIALOG_H

#include "kookascan_export.h"

#include "dialogbase.h"

class QComboBox;
class KLineEdit;
class QPushButton;

/**
 * @short A dialogue to allow the user to manually enter a scan device.
 *
 * Intended to be used in the case where the scanner device cannot be
 * automatically detected by SANE.
 *
 * @author Jonathan Marten
 */

class KOOKASCAN_EXPORT AddDeviceDialog : public DialogBase
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param parent Parent widget
     * @param caption Caption for the dialogue
     **/
    explicit AddDeviceDialog(QWidget *parent, const QString &caption);

    /**
     * Destructor.
     *
     **/
    virtual ~AddDeviceDialog() {}

    /**
     * Get the entered value from the "Scanner device name" field.
     *
     * @return The SANE device name as entered by the user.
     **/
    QByteArray getDevice() const;

    /**
     * Get the entered value from the "Description" field.
     *
     * @return The device description as entered by the user.
     **/
    QString getDescription() const;

    /**
     * Get the entered value from the "Manufacturer" field.
     *
     * @return The device manufacturer as entered by the user.
     **/
    QString getManufacturer() const;

    /**
     * Get the selected value from the "Device type" combo box.
     *
     * @return The device type as selected by the user.
     **/
    QByteArray getType() const;

protected slots:
    void slotTextChanged();
    void slotLinkActivated(const QString &link);

private:
    KLineEdit *mDevEdit;
    KLineEdit *mManufacturerEdit;
    KLineEdit *mDescEdit;
    QComboBox *mTypeCombo;
};

#endif							// ADDDEVICEDIALOG_H
