/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2013-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "autoselectdialog.h"

#include <qcombobox.h>
#include <qformlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <klocalizedstring.h>

#include "kscancontrols.h"
#include "kscandevice.h"
#include "scansettings.h"


// Combo box indexes
#define INDEX_BLACK		0
#define INDEX_WHITE		1


AutoSelectDialog::AutoSelectDialog(QWidget *parent)
    : DialogBase(parent)
{
    setObjectName("AutoSelectDialog");

    setModal(true);
    setWindowTitle(i18nc("@title:window", "Autoselect Settings"));
    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply);

    QWidget *w = new QWidget(this);
    QFormLayout *fl = new QFormLayout(w);		// looks better with combo expanded
    fl->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    // slider for add/subtract margin
    const KConfigSkeletonItem *item = ScanSettings::self()->previewAutoselMarginItem();
    Q_ASSERT(item!=nullptr);
    int defaultVal = KScanDevice::getDefault<int>(item);
    int maxVal = item->maxValue().toInt();
    int minVal = item->minValue().toInt();

    mMarginSlider = new KScanSlider(nullptr, QString(), true);
    mMarginSlider->setRange(minVal, maxVal, -1, defaultVal);
    mMarginSlider->setToolTip(item->toolTip());
    connect(mMarginSlider, QOverload<int>::of(&KScanSlider::settingChanged), this, &AutoSelectDialog::slotControlChanged);
    fl->addRow(item->label(), mMarginSlider);

    fl->addItem(new QSpacerItem(1, verticalSpacing()));

    // combobox to select whether black or white background
    item = ScanSettings::self()->previewAutoselBackgroundItem();
    Q_ASSERT(item!=nullptr);
    mBackgroundCombo = new QComboBox;
    mBackgroundCombo->insertItem(INDEX_BLACK, i18n("Black"));
    mBackgroundCombo->insertItem(INDEX_WHITE, i18n("White"));
    mBackgroundCombo->setToolTip(item->toolTip());
    connect(mBackgroundCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AutoSelectDialog::slotControlChanged);
    fl->addRow(item->label(), mBackgroundCombo);

    // slider for dust size - apparently not really much impact on the result
    item = ScanSettings::self()->previewAutoselDustsizeItem();
    Q_ASSERT(item!=nullptr);
    defaultVal = KScanDevice::getDefault<int>(item);
    maxVal = item->maxValue().toInt();
    minVal = item->minValue().toInt();

    mDustsizeSlider = new KScanSlider(nullptr, QString(), true);
    mDustsizeSlider->setRange(minVal, maxVal, -1, defaultVal);
    mDustsizeSlider->setToolTip(item->toolTip());
    connect(mDustsizeSlider, QOverload<int>::of(&KScanSlider::settingChanged), this, &AutoSelectDialog::slotControlChanged);
    fl->addRow(item->label(), mDustsizeSlider);

    setMainWidget(w);

    connect(buttonBox()->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &AutoSelectDialog::slotApplySettings);
    connect(buttonBox()->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this, &AutoSelectDialog::slotApplySettings);
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void AutoSelectDialog::slotControlChanged()
{
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void AutoSelectDialog::slotApplySettings()
{
    const int margin = mMarginSlider->value();
    const bool bgIsWhite = (mBackgroundCombo->currentIndex()==INDEX_WHITE);
    const int dustsize = mDustsizeSlider->value();
    emit settingsChanged(margin, bgIsWhite, dustsize);
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void AutoSelectDialog::setSettings(int margin, bool bgIsWhite, int dustsize)
{
    mMarginSlider->setValue(margin);
    mBackgroundCombo->setCurrentIndex(bgIsWhite ? INDEX_WHITE : INDEX_BLACK);
    mDustsizeSlider->setValue(dustsize);
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
}
