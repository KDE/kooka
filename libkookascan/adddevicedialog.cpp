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

#include "adddevicedialog.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qcombobox.h>

#include <klocalizedstring.h>
#include <klineedit.h>


AddDeviceDialog::AddDeviceDialog(QWidget *parent, const QString &caption)
    : DialogBase(parent)
{
    setObjectName("AddDeviceDialog");

    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    setWindowTitle(caption);

    QWidget *w = new QWidget(this);
    QVBoxLayout *vl = new QVBoxLayout(w);

    QLabel *lab = new QLabel(xi18nc("@info",
                                    "If your scanner has not been automatically detected, you can specify it here. "
                                    "The <interface>Scanner device name</interface> should be a backend name (with optional parameters) "
                                    "that is understood by SANE, see <link url=\"man:/sane\">sane(7)</link> or "
                                    "<link url=\"man:/sane-dll\">sane-dll(5)</link> for more information on available backends. "
                                    "The <interface>Type</interface> and <interface>Description</interface> can be used to identify the scanner later."
                                    "<nl/><nl/>"
                                    "For the information that needs to be entered here, try to locate the device using the "
                                    "<link url=\"man:/sane-find-scanner\">sane-find-scanner(1)</link> command. For a "
                                    "USB or networked HP scanner using <link url=\"http://hplip.sourceforge.net/\">HPLIP</link>, "
                                    "try using the <command>hp-probe</command> command to locate it, for example "
                                    "<icode>hp-probe&nbsp;-b&nbsp;usb</icode> or <icode>hp-probe&nbsp;-b&nbsp;net</icode>. "
                                    "If the scanner is found, then enter the device name displayed by these commands; note "
                                    "that if using HPLIP then <icode>hp:</icode> needs to be replaced by <icode>hpaio:</icode>."
                                    "<nl/><nl/>"
                                    "If these commands fail to locate your scanner, then it may not be supported "
                                    "by SANE. Check the SANE documentation for a "
                                    "<link url=\"http://www.sane-project.org/sane-supported-devices.html\">list of supported devices</link>."), w);
    lab->setWordWrap(true);
    lab->setOpenExternalLinks(true);
    vl->addWidget(lab);

    vl->addSpacing(verticalSpacing());
    vl->addStretch(1);

    lab = new QLabel(i18n("Scanner device name:"), w);
    vl->addWidget(lab);

    mDevEdit = new KLineEdit(w);
    connect(mDevEdit, &KLineEdit::textChanged, this, &AddDeviceDialog::slotTextChanged);
    vl->addWidget(mDevEdit);
    lab->setBuddy(mDevEdit);

    lab = new QLabel(i18n("Device type:"), w);
    vl->addWidget(lab);

    mTypeCombo = new QComboBox(w);
    vl->addWidget(mTypeCombo);
    lab->setBuddy(mTypeCombo);

    lab = new QLabel(i18n("Description:"), w);
    vl->addWidget(lab);

    mDescEdit = new KLineEdit(w);
    connect(mDescEdit, &KLineEdit::textChanged, this, &AddDeviceDialog::slotTextChanged);
    vl->addWidget(mDescEdit);
    lab->setBuddy(mDescEdit);

    w->setMinimumSize(QSize(450, 420));
    setMainWidget(w);

    // This list from http://www.sane-project.org/html/doc011.html#s4.2.8
    QStringList types;
    types << "scanner" << "film scanner" << "flatbed scanner"
          << "frame grabber" << "handheld scanner" << "multi-function peripheral"
          << "sheetfed scanner" << "still camera" << "video camera"
          << "virtual device";
    mTypeCombo->addItems(types);

    slotTextChanged();
}

void AddDeviceDialog::slotTextChanged()
{
    setButtonEnabled(QDialogButtonBox::Ok,
                     !mDevEdit->text().trimmed().isEmpty() &&
                     !mDescEdit->text().trimmed().isEmpty());
}

QByteArray AddDeviceDialog::getDevice() const
{
    return mDevEdit->text().toLocal8Bit();
}

QString AddDeviceDialog::getDescription() const
{
    return mDescEdit->text();
}

QByteArray AddDeviceDialog::getType() const
{
    return mTypeCombo->currentText().toLocal8Bit();
}
