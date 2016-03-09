/************************************************************************
 *                                  *
 *  This file is part of libkscan, a KDE scanning library.      *
 *                                  *
 *  Copyright (C) 2013 Jonathan Marten <jjm@keelhaul.me.uk>     *
 *                                  *
 *  This library is free software; you can redistribute it and/or   *
 *  modify it under the terms of the GNU Library General Public     *
 *  License as published by the Free Software Foundation and appearing  *
 *  in the file COPYING included in the packaging of this file;     *
 *  either version 2 of the License, or (at your option) any later  *
 *  version.                                *
 *                                  *
 *  This program is distributed in the hope that it will be useful, *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of  *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   *
 *  GNU General Public License for more details.            *
 *                                  *
 *  You should have received a copy of the GNU General Public License   *
 *  along with this program;  see the file COPYING.  If not, write to   *
 *  the Free Software Foundation, Inc., 51 Franklin Street,     *
 *  Fifth Floor, Boston, MA 02110-1301, USA.                *
 *                                  *
 ************************************************************************/

#include "autoselectdialog.h"

#include <qcombobox.h>
#include <qformlayout.h>
#include <qlabel.h>
#include <qslider.h>

#include <KLocalizedString>
#include <QDebug>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "kscancontrols.h"
#include "kscandevice.h"
#include "scansettings.h"


// Combo box indexes
#define INDEX_BLACK		0
#define INDEX_WHITE		1

AutoSelectDialog::AutoSelectDialog(QWidget *parent)
    : QDialog(parent)
{
    //qDebug();
    setObjectName("AutoSelectDialog");

    setWindowTitle(i18nc("@title:window", "Autoselect Settings"));
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QFormLayout *fl = new QFormLayout;          // looks better with combo expanded
    fl->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    // slider for add/subtract margin
    const KConfigSkeletonItem *item = ScanSettings::self()->previewAutoselMarginItem();
    int defaultVal = KScanDevice::getDefault<int>(item);
    int maxVal = item->maxValue().toInt();
    int minVal = item->minValue().toInt();

    mMarginSlider = new KScanSlider(NULL, QString::null, minVal, maxVal, true, defaultVal);
    mMarginSlider->setValue(defaultVal);
    mMarginSlider->setToolTip(item->toolTip());
    connect(mMarginSlider, SIGNAL(settingChanged(int)), SLOT(slotControlChanged()));
    fl->addRow(item->label(), mMarginSlider);

//TODO PORT QT5     fl->addItem(new QSpacerItem(1, QDialog::marginHint()));

    // combobox to select whether black or white background
    item = ScanSettings::self()->previewAutoselBackgroundItem();
    mBackgroundCombo = new QComboBox;
    mBackgroundCombo->insertItem(INDEX_BLACK, i18n("Black"));
    mBackgroundCombo->insertItem(INDEX_WHITE, i18n("White"));
    mBackgroundCombo->setToolTip(item->toolTip());
    connect(mBackgroundCombo, SIGNAL(currentIndexChanged(int)), SLOT(slotControlChanged()));
    fl->addRow(item->label(), mBackgroundCombo);

    // slider for dust size - apparently not really much impact on the result
    item = ScanSettings::self()->previewAutoselDustsizeItem();
    defaultVal = KScanDevice::getDefault<int>(item);
    maxVal = item->maxValue().toInt();
    minVal = item->minValue().toInt();

    mDustsizeSlider = new KScanSlider(NULL, QString::null, minVal, maxVal, true, defaultVal);
    mDustsizeSlider->setValue(defaultVal);
    mDustsizeSlider->setToolTip(item->toolTip());
    connect(mDustsizeSlider, SIGNAL(settingChanged(int)), SLOT(slotControlChanged()));
    fl->addRow(item->label(), mDustsizeSlider);

    QWidget *w = new QWidget;
    w->setLayout(fl);
    mainLayout->addWidget(w);
    mainLayout->addWidget(mButtonBox);

    connect(mButtonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), SLOT(slotApplySettings()));
    connect(okButton, SIGNAL(clicked()), SLOT(slotApplySettings()));
    connect(okButton, SIGNAL(clicked()), SLOT(accept()));
    mButtonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

AutoSelectDialog::~AutoSelectDialog()
{
}

void AutoSelectDialog::slotControlChanged()
{
    mButtonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void AutoSelectDialog::slotApplySettings()
{
    const int margin = mMarginSlider->value();
    const bool bgIsWhite = (mBackgroundCombo->currentIndex()==INDEX_WHITE);
    const int dustsize = mDustsizeSlider->value();
    emit settingsChanged(margin, bgIsWhite, dustsize);
    mButtonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void AutoSelectDialog::setSettings(int margin, bool bgIsWhite, int dustsize)
{
    mMarginSlider->setValue(margin);
    mBackgroundCombo->setCurrentIndex(bgIsWhite ? INDEX_WHITE : INDEX_BLACK);
    mDustsizeSlider->setValue(dustsize);
}
