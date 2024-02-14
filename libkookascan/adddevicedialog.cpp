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
#include <qformlayout.h>
#include <qcombobox.h>
#include <qdesktopservices.h>
#include <qurl.h>

#include <klocalizedstring.h>
#include <klineedit.h>

#include "clickabletooltip.h"
#include "scandevices.h"


AddDeviceDialog::AddDeviceDialog(QWidget *parent, const QString &caption)
    : DialogBase(parent)
{
    setObjectName("AddDeviceDialog");

    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    setWindowTitle(caption);

    QWidget *w = new QWidget(this);
    QFormLayout *fl = new QFormLayout(w);

    QLabel *lab = new QLabel(xi18nc("@info",
                                    "If your scanner has not been automatically detected, you can specify it here. "
                                    "The <interface>Scanner device</interface> is the SANE backend device name. "
                                    "The <interface>Manufacturer</interface>, <interface>Type</interface> and "
                                    "<interface>Description</interface> can be used to identify the scanner later."
                                    "<nl/><nl/>"
                                    "More information on "
                                    "<link url=\"?1\">SANE backends</link>, "
                                    "<link url=\"?2\">locating devices</link>, "
                                    "<link url=\"?3\">HP scanners</link>, "
                                    "<link url=\"?4\">device not found</link>."), w);
    lab->setWordWrap(true);
    lab->setOpenExternalLinks(false);
    connect(lab, &QLabel::linkActivated, this, &AddDeviceDialog::slotLinkActivated);
    fl->addRow(lab);

    fl->addRow(new QLabel(w));				// separator row

    mDevEdit = new KLineEdit(w);
    connect(mDevEdit, &KLineEdit::textChanged, this, &AddDeviceDialog::slotTextChanged);
    fl->addRow(i18n("Scanner device:"), mDevEdit);

    mManufacturerEdit = new KLineEdit(w);
    mManufacturerEdit->setPlaceholderText(i18nc("Value used for manufacturer if none entered", "User specified"));
    connect(mManufacturerEdit, &KLineEdit::textChanged, this, &AddDeviceDialog::slotTextChanged);
    fl->addRow(i18n("Manufacturer:"), mManufacturerEdit);

    mTypeCombo = new QComboBox(w);
    mTypeCombo->setSizePolicy(QSizePolicy::MinimumExpanding, mTypeCombo->sizePolicy().verticalPolicy());
    fl->addRow(i18n("Device type:"), mTypeCombo);

    mDescEdit = new KLineEdit(w);
    connect(mDescEdit, &KLineEdit::textChanged, this, &AddDeviceDialog::slotTextChanged);
    fl->addRow(i18n("Description:"), mDescEdit);

    w->setMinimumSize(QSize(540, 300));
    setMainWidget(w);

    // This list from http://www.sane-project.org/html/doc011.html#s4.2.8
    QList<const char *> types;
    types << "scanner" << "film scanner" << "flatbed scanner"
          << "frame grabber" << "handheld scanner" << "multi-function peripheral"
          << "sheetfed scanner" << "still camera" << "video camera"
          << "virtual device";
    for (const char *type : qAsConst(types))
    {
        QString iconName = ScanDevices::self()->typeIconName(type);
        if (iconName.isEmpty()) iconName = "scanner";
        mTypeCombo->addItem(QIcon::fromTheme(iconName), type);
    }

    slotTextChanged();
}


void AddDeviceDialog::slotLinkActivated(const QString &link)
{
    if (!link.startsWith('?'))				// not an internal help link
    {
        QDesktopServices::openUrl(QUrl(link));
        return;
    }

    // XML entity 8209 is a non-breaking hyphen (no equivalent in HTML)
    QString msg;

    if (link=="?1")					// help on SANE backends
    {
        msg = xi18nc("@info",
                     "The <interface>Scanner device</interface> should be a backend name (with optional "
                     "parameters) that is understood by SANE. See <link url=\"man:/sane\">sane(7)</link> or "
                     "<link url=\"man:/sane-dll\">sane&#8209;dll(5)</link> for more information on available "
                     "backends and the format of device names.");
    }
    else if (link=="?2")				// help on locating devices
    {
        msg = xi18nc("@info",
                     "To find the information that needs to be entered here, try to locate the device using the "
                     "<link url=\"man:/sane-find-scanner\">sane&#8209;find&#8209;scanner(1)</link> command for SCSI, "
                     "USB or parallel port scanners, or the <link url=\"man:/scanimage\">scanimage(1)</link> "
                     "command with the <icode>&#8209;L</icode> option for network scanners. "
                     "If the scanner is found, then enter the device name displayed by these commands.");
    }
    else if (link=="?3")				// help on HP network devices
    {
        msg = xi18nc("@info",
                     "For a USB or networked HP scanner using <link url=\"http://hplip.sourceforge.net/\">HPLIP</link>, "
                     "try using the <command>hp&#8209;probe</command> command to locate it, for example "
                     "<icode>hp&#8209;probe&nbsp;&#8209;b&nbsp;usb</icode> or <icode>hp&#8209;probe&nbsp;&#8209;b&nbsp;net</icode>. "
                     "Note that if the scanner is found by HPLIP, then the device name <icode>hp:</icode> "
                     "that it displays needs to be replaced by <icode>hpaio:</icode> for SANE.");
    }
    else if (link=="?4")				// help if scanner not found
    {
        msg = xi18nc("@info",
                     "If these commands fail to locate your scanner, then it may not be supported "
                     "by SANE. Check the SANE documentation for a "
                     "<link url=\"http://www.sane-project.org/sane-supported-devices.html\">list of supported devices</link>.");
    }

    if (msg.isEmpty()) return;				// help link not recognised

    QLabel *l = ClickableToolTip::showText(QCursor::pos(), msg);
    l->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    l->setOpenExternalLinks(true);
}


void AddDeviceDialog::slotTextChanged()
{
    setButtonEnabled(QDialogButtonBox::Ok,
                     !mDevEdit->text().trimmed().isEmpty() &&
                     !mDescEdit->text().trimmed().isEmpty());
}

QByteArray AddDeviceDialog::getDevice() const
{
    return (mDevEdit->text().toLocal8Bit());
}

QString AddDeviceDialog::getDescription() const
{
    return (mDescEdit->text());
}

QString AddDeviceDialog::getManufacturer() const
{
    return (mManufacturerEdit->text());
}

QByteArray AddDeviceDialog::getType() const
{
    return (mTypeCombo->currentText().toLocal8Bit());
}
