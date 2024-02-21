/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2024  Jonathan Marten <jjm@keelhaul.me.uk>		*
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

#include "multiscandialog.h"

// #include <qbuttongroup.h>
// #include <qlayout.h>
// #include <qradiobutton.h>
// #include <qlabel.h>
// #include <qlineedit.h>
// #include <qvalidator.h>
#include <qcombobox.h>
#include <qgridlayout.h>
#include <qdebug.h>
#include <qlabel.h>

#include <sane/saneopts.h>

#include <klocalizedstring.h>

#include "kscandevice.h"
#include "kscanoption.h"


MultiScanDialog::MultiScanDialog(KScanDevice *dev, QWidget *pnt)
    : DialogBase(pnt)
{
    setObjectName("MultiScanDialog");

    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    setWindowTitle(i18n("ADF & Multiple Scan Options"));
    setModal(true);

    mSourceCombo = nullptr;				// not created yet

    QWidget *w = new QWidget(this);
    QGridLayout *gl = new QGridLayout(w);

    // Row 0: Scan source, if the option is available
    const KScanOption *so = dev->getOption(SANE_NAME_SCAN_SOURCE, false);
    if (so!=nullptr && so->isValid())
    {
        QLabel *lab = new QLabel(i18n("Scan source:"), w);
        // TODO: get QFormLayout alignment from style
        gl->addWidget(lab, 0, 0, Qt::AlignRight);

        // It is not possible to simply "adopt" the existing combo box
        // created by ScanParams, because that would remove it from the main
        // ScanParams widget and layout.  (So should that have said "kidnap"?)
        // Therefore we need to create a new combo box here.
        mSourceCombo = new QComboBox(w);
        mSourceCombo->setSizePolicy(QSizePolicy::MinimumExpanding, mSourceCombo->sizePolicy().verticalPolicy());

        // This assumes that the available sources do not change during
        // the lifetime of this dialogue.
        const QList<QByteArray> srcs = so->getList();
        for (const QByteArray &src : qAsConst(srcs)) mSourceCombo->addItem(src);

        connect(mSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MultiScanDialog::slotGuiChange);

        gl->addWidget(mSourceCombo, 0, 1, 1, -1, Qt::AlignLeft);
        lab->setBuddy(mSourceCombo);
        mSourceCombo->setEnabled(so->isActive());
    }

    gl->setColumnStretch(1, 1);
    gl->setRowStretch(9, 1);

    setMainWidget(w);
}


void MultiScanDialog::setOptions(const MultiScanOptions &opts)
{
    mOptions = opts;
    updateGui();
}


const MultiScanOptions &MultiScanDialog::options() const
{
    return (mOptions);
}


void MultiScanDialog::slotGuiChange()
{
    if (mSourceCombo!=nullptr) mOptions.setSource(mSourceCombo->currentText().toLatin1());




}


void MultiScanDialog::updateGui()
{
    if (mSourceCombo!=nullptr) mSourceCombo->setCurrentText(mOptions.source());





}
