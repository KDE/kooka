/* This file is part of the KDE Project             -*- mode:c++ -*-
   Copyright (C) 2008 Jonathan Marten <jjm@keelhaul.me.uk>

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

#ifndef ADDDEVICE_H
#define ADDDEVICE_H

#include "libkscanexport_export.h"

#include <QDialog>

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

class LIBKSCANEXPORT_EXPORT AddDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param parent Parent widget
     * @param caption Caption for the dialogue
     **/
    AddDeviceDialog(QWidget *parent, const QString &caption);

    /**
     * Destructor.
     *
     **/
    virtual ~AddDeviceDialog() {};

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
     * Get the selected value from the "Device type" combo box.
     *
     * @return The device type as selected by the user.
     **/
    QByteArray getType() const;

protected slots:
    void slotTextChanged();

private:
    KLineEdit *mDevEdit;
    KLineEdit *mDescEdit;
    QComboBox *mTypeCombo;
    QPushButton *mOkButton;
};

#endif                          // ADDDEVICE_H
