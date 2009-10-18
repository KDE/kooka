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

#include "adddevice.h"
#include "adddevice.moc"

#include <qlabel.h>
#include <qlayout.h>

#include <klocale.h>
#include <klineedit.h>


AddDeviceDialog::AddDeviceDialog(QWidget *parent,const QString &caption)
	: KDialog(parent)
{
    setObjectName("AddDeviceDialog");

    setModal(true);
    setButtons(KDialog::Ok|KDialog::Cancel);
    setCaption(caption);
    showButtonSeparator(true);

    QWidget *w = new QWidget(this);
    setMainWidget(w);

    QVBoxLayout *vl = new QVBoxLayout(w);

    QLabel *lab = new QLabel(i18n("<qt>"
"<p>"
"If your scanner has not been automatically detected, you can specify it here. "
"The <b>Scanner device name</b> should be a backend name (with optional parameters) "
"that is understood by SANE, see <a href=\"man:/sane\">sane(7)</a> or "
"<a href=\"man:/sane-dll\">sane-dll(5)</a> for more information on available backends. "
"The <b>Description</b> may be any string that you can use to identify the scanner later."
"<p>"
"For the information that needs to be entered here, try to locate the device using the "
"<a href=\"man:/sane-find-scanner\">sane-find-scanner(1)</a> command. For a "
"USB or networked HP scanner using <a href=\"http://hplip.sourceforge.net/\">HPLIP</a>, "
"try using the <u>hp-probe</u> command to locate it, for example "
"'hp-probe&nbsp;-b&nbsp;usb' or 'hp-probe&nbsp;-b&nbsp;net'. "
"If the scanner is found, then enter the device name displayed by these commands; note "
"that if using HPLIP then \"hp:\" needs to be be replaced by \"hpaio:\"."
"<p>"
"If these commands fail to locate your scanner, then it may not be supported "
"by SANE. Check the SANE documentation for a "
"<a href=\"http://www.sane-project.org/sane-supported-devices.html\">list of supported devices</a>."
"<br>"),w);
    lab->setWordWrap(true);
    lab->setOpenExternalLinks(true);
    vl->addWidget(lab);

    vl->addSpacing(KDialog::spacingHint());

    lab = new QLabel(i18n("Scanner device name:"),w);
    vl->addWidget(lab);

    mDevEdit = new KLineEdit(w);
    connect(mDevEdit,SIGNAL(textChanged(const QString &)),SLOT(slotTextChanged()));
    vl->addWidget(mDevEdit);
    lab->setBuddy(mDevEdit);

    lab = new QLabel(i18n("Description:"),w);
    vl->addWidget(lab);

    mDescEdit = new KLineEdit(w);
    connect(mDescEdit,SIGNAL(textChanged(const QString &)),SLOT(slotTextChanged()));
    vl->addWidget(mDescEdit);
    lab->setBuddy(mDescEdit);

    slotTextChanged();
}


void AddDeviceDialog::slotTextChanged()
{
    enableButtonOk(!mDevEdit->text().trimmed().isEmpty() &&
                   !mDescEdit->text().trimmed().isEmpty());
}


QString AddDeviceDialog::getDevice() const
{
    return (mDevEdit->text());
}


QString AddDeviceDialog::getDescription() const
{
    return (mDescEdit->text());
}
