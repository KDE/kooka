/* This file is part of the KDE Project				-*- mode:c++ -*-
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

#include "libkscanexport.h"

#include <kdialog.h>

class KLineEdit;

/**
 *  A dialogue to allow the user to manually enter a scan device, in
 *  the case where it cannot be automatically detected by SANE.
 */

class KSCAN_EXPORT AddDeviceDialog : public KDialog
{
	Q_OBJECT

public:
	AddDeviceDialog(QWidget *parent,const QString &caption);
	virtual ~AddDeviceDialog() {};

	QByteArray getDevice() const;
	QString getDescription() const;

protected slots:
	void slotTextChanged();

private:
	KLineEdit *mDevEdit;
	KLineEdit *mDescEdit;
};

#endif							// ADDDEVICE_H
