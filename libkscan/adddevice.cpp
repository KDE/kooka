/* This file is part of the KDE Project
   Copyright (C) 2000 Jonathan Marten <jjm@keelhaul.me.uk>  

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

#include <klineedit.h>
#include <klocale.h>
#include <kactivelabel.h>

#include <qlabel.h>
#include <qvbox.h>
#include <qlayout.h>

#include "adddevice.h"


AddDeviceDialog::AddDeviceDialog(QWidget *parent,const QString &caption)
	: KDialogBase(parent,NULL,true,caption,Ok|Cancel)
{
	QVBox *vb = makeVBoxMainWidget();

	QVBoxLayout *vl = static_cast<QVBoxLayout *>(vb->layout());

	new KActiveLabel(i18n("<qt>\
<p>If your scanner has not been automatically detected, you can specify it here. \
The <b>Scanner device name</b> should be a backend name (with optional parameters) \
that is understood by SANE, see <a href=\"man:/sane\">sane(7)</a> or \
<a href=\"man:/sane-dll\">sane-dll(5)</a> for more information on available backends. \
The <b>Description</b> many be any string that you can use to identify the scanner later.\
<br>\
<p>For the information that needs to be entered here, try to locate the device using the \
<a href=\"man:/sane-find-scanner\">sane-find-scanner(1)</a> command. For a \
USB or networked HP scanner using <a href=\"http://hplip.sourceforge.net/\">HPLIP</a>, \
try using the <u>hp-probe</u> command to locate it, for example \
'hp-probe&nbsp;-b&nbsp;usb' or 'hp-probe&nbsp;-b&nbsp;net'. \
If the scanner is found, then enter the device name displayed by these commands; note \
that if using HPLIP then \"hp:\" needs to be be replaced by \"hpaio:\".\
<br>\
<p>If these commands fail to locate your scanner, then it may not be supported \
by SANE. Check the SANE documentation for a \
<a href=\"http://www.sane-project.org/sane-supported-devices.html\">list of supported devices</a>.\
<br><br>"),vb);
	vl->addSpacing(KDialog::spacingHint());

	QLabel *l = new QLabel(i18n("Scanner device name:"),vb);
	mDevEdit = new KLineEdit(vb);
	connect(mDevEdit,SIGNAL(textChanged(const QString &)),
		this,SLOT(slotTextChanged()));
	l->setBuddy(mDevEdit);

	l = new QLabel(i18n("Description:"),vb);
	mDescEdit = new KLineEdit(vb);
	connect(mDescEdit,SIGNAL(textChanged(const QString &)),
		this,SLOT(slotTextChanged()));
	l->setBuddy(mDescEdit);

	slotTextChanged();
}


void AddDeviceDialog::slotTextChanged()
{
	bool ok = !mDevEdit->text().stripWhiteSpace().isEmpty() &&
		!mDescEdit->text().stripWhiteSpace().isEmpty();
	enableButtonOK(ok);
}


QString AddDeviceDialog::getDevice() const
{
	return (mDevEdit->text());
}


QString AddDeviceDialog::getDescription() const
{
	return (mDescEdit->text());
}


#include "adddevice.moc"
