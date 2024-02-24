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

#include <qcombobox.h>
#include <qgridlayout.h>
#include <qdebug.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

#include <sane/saneopts.h>

#include <klocalizedstring.h>
#include <kpluralhandlingspinbox.h>

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
    gl->setContentsMargins(0, 0, 0, 0);
    int row = 0;

    // Row 0: Scan source, if the option is available
    const KScanOption *so = dev->getOption(SANE_NAME_SCAN_SOURCE, false);
    if (so!=nullptr && so->isValid())
    {
        QLabel *lab = new QLabel(i18n("Scan source:"), w);
        const Qt::Alignment labelAlign = static_cast<Qt::Alignment> (style()->styleHint(QStyle::SH_FormLayoutLabelAlignment, nullptr, this));
        gl->addWidget(lab, row, 0, 1, 2, labelAlign);

        // It is not possible to simply "adopt" the existing combo box
        // created by ScanParams, because that would remove it from the main
        // ScanParams widget and layout.  (So should that have said "kidnap"?)
        // Therefore we need to create a new combo box here.
        mSourceCombo = new QComboBox(w);
        mSourceCombo->setSizePolicy(QSizePolicy::MinimumExpanding, mSourceCombo->sizePolicy().verticalPolicy());
        mSourceCombo->setToolTip(i18nc("@info:tooltip", "Select the scan source"));

        // This assumes that the available sources do not change during
        // the lifetime of this dialogue.
        const QList<QByteArray> srcs = so->getList();
        for (const QByteArray &src : qAsConst(srcs)) mSourceCombo->addItem(src);

        connect(mSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MultiScanDialog::slotGuiChange);

        gl->addWidget(mSourceCombo, row, 2, 1, -1, Qt::AlignLeft);
        lab->setBuddy(mSourceCombo);
        mSourceCombo->setEnabled(so->isActive());
    }
    ++row;

    // Row 1: Spacing
    gl->setRowMinimumHeight(row, 2*DialogBase::verticalSpacing());
    ++row;

    // Row 2: Scan and ADF group
    QGroupBox *grp = new QGroupBox(i18n("Scan and ADF"), w);
    grp->setFlat(true);
    gl->addWidget(grp, row, 0, 1, -1);
    ++row;

    // Button group: Single/Multiple scan
    QButtonGroup *bg = new QButtonGroup(w);
    connect(bg, &QButtonGroup::idToggled, this, &MultiScanDialog::slotGuiChange);

    // Row 3: Single Scan radio button
    mScanSingleRadio = new QRadioButton(i18n("Single scan"), w);
    mScanSingleRadio->setToolTip(i18nc("@info:tooltip", "Perform a single scan from the selected source"));
    bg->addButton(mScanSingleRadio);
    gl->addWidget(mScanSingleRadio, row, 0, 1, -1, Qt::AlignLeft);
    ++row;

    // Row 4: Multiple Scan radio button
    mScanMultiRadio = new QRadioButton(i18n("Multiple scan"), w);
    mScanMultiRadio->setToolTip(i18nc("@info:tooltip", "Perform multiple scans from the selected source"));
    bg->addButton(mScanMultiRadio);
    gl->addWidget(mScanMultiRadio, row, 0, 1, -1, Qt::AlignLeft);
    ++row;

    // Button group: Multiple scan options
    bg = new QButtonGroup(w);
    connect(bg, &QButtonGroup::idToggled, this, &MultiScanDialog::slotGuiChange);

    // Row 5: Empty ADF radio button
    mMultiEmptyAdfRadio = new QRadioButton(i18n("Scan until ADF is empty"), w);
    mMultiEmptyAdfRadio->setToolTip(i18nc("@info:tooltip", "Keep scanning until the automatic document feeder is empty"));
    bg->addButton(mMultiEmptyAdfRadio);
    gl->addWidget(mMultiEmptyAdfRadio, row, 1, 1, -1, Qt::AlignLeft);
    ++row;

    // Row 6: Manual Scan radio button
    mMultiManualScanRadio = new QRadioButton(i18n("Wait until ready for next scan"), w);
    mMultiManualScanRadio->setToolTip(i18nc("@info:tooltip", "Wait for confirmation between scans"));
    bg->addButton(mMultiManualScanRadio);
    gl->addWidget(mMultiManualScanRadio, row, 1, 1, -1, Qt::AlignLeft);
    ++row;

    // Row 7: Time Delay radio button and delay setting
    mMultiDelayScanRadio = new QRadioButton(i18n("Scan after waiting for"), w);
    mMultiDelayScanRadio->setToolTip(i18nc("@info:tooltip", "Delay for a time between scans"));
    bg->addButton(mMultiDelayScanRadio);
    gl->addWidget(mMultiDelayScanRadio, row, 1, 1, 2, Qt::AlignLeft);

    mDelayTimeSpinbox = new KPluralHandlingSpinBox(w);
    mDelayTimeSpinbox->setRange(1, 60);
    mDelayTimeSpinbox->setSuffix(ki18ncp("Time unit", " second", " seconds"));
    mDelayTimeSpinbox->setToolTip(i18nc("@info:tooltip", "Set the delay time between scans"));
    gl->addWidget(mDelayTimeSpinbox, row, 3, Qt::AlignLeft);
    ++row;

    gl->setColumnStretch(3, 1);
    gl->setRowStretch(row, 1);

    setMainWidget(w);
}


void MultiScanDialog::setOptions(const MultiScanOptions &opts)
{
    mOptions = opts;

    if (mSourceCombo!=nullptr) mSourceCombo->setCurrentText(mOptions.source());

    const MultiScanOptions::Flags f = mOptions.flags();
    const bool multi = (f & MultiScanOptions::MultiScan);

    mScanSingleRadio->setChecked(!multi);
    mScanMultiRadio->setChecked(multi);

    mMultiEmptyAdfRadio->setChecked(true);		// unless one of the next two set
    mMultiManualScanRadio->setChecked(f & MultiScanOptions::ManualWait);
    mMultiDelayScanRadio->setChecked(f & MultiScanOptions::DelayWait);

    mDelayTimeSpinbox->setValue(mOptions.delay());

    slotGuiChange();
}


const MultiScanOptions &MultiScanDialog::options()
{
    if (mSourceCombo!=nullptr) mOptions.setSource(mSourceCombo->currentText().toLatin1());

    MultiScanOptions::Flags f = mOptions.flags();
    f &= ~MultiScanOptions::MultiScan;
    if (mScanMultiRadio->isChecked()) f |= MultiScanOptions::MultiScan;

    f &= ~(MultiScanOptions::ManualWait|MultiScanOptions::DelayWait);
    if (mMultiManualScanRadio->isChecked()) f |= MultiScanOptions::ManualWait;
    else if (mMultiDelayScanRadio->isChecked()) f |= MultiScanOptions::DelayWait;

    mOptions.setFlags(f);
    mOptions.setDelay(mDelayTimeSpinbox->value());

    return (mOptions);
}


void MultiScanDialog::slotGuiChange()
{
    const MultiScanOptions::Flags f = mOptions.flags();
    const bool multi = mScanMultiRadio->isChecked();
    const bool adf = (mSourceCombo!=nullptr) ? KScanDevice::matchesAdf(mSourceCombo->currentText().toLatin1()) : false;

    mMultiEmptyAdfRadio->setEnabled(adf && multi && (f & MultiScanOptions::AdfAvailable));
    mMultiManualScanRadio->setEnabled(multi);
    mMultiDelayScanRadio->setEnabled(multi);

    if (!mMultiEmptyAdfRadio->isEnabled() && mMultiEmptyAdfRadio->isChecked()) mMultiManualScanRadio->setChecked(true);

    mDelayTimeSpinbox->setEnabled(multi && mMultiDelayScanRadio->isChecked());
}
