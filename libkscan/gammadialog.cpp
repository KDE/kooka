/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>

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

#include "gammadialog.h"

#include <qlabel.h>
#include <qgridlayout.h>

#include <klocale.h>
#include <QDebug>
#include <kgammatable.h>

#include "kscancontrols.h"
#include "gammawidget.h"

GammaDialog::GammaDialog(const KGammaTable *table, QWidget *parent)
    : KDialog(parent)
{
    setObjectName("GammaDialog");

    setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply | KDialog::Reset);
    setCaption(i18n("Edit Gamma Table"));
    setModal(true);
    showButtonSeparator(true);

    mTable = new KGammaTable(*table);           // take our own copy

    QWidget *page = new QWidget(this);
    setMainWidget(page);
    QGridLayout *gl = new QGridLayout(page);

    // Sliders for brightness, contrast, gamma
    mSetBright = new KScanSlider(page, i18n("Brightness"), -50, 50);
    mSetBright->setValue(mTable->getBrightness());
    connect(mSetBright, SIGNAL(settingChanged(int)), mTable, SLOT(setBrightness(int)));
    QLabel *l = new QLabel(mSetBright->label(), page);
    l->setBuddy(mSetBright);
    gl->setRowMinimumHeight(0, KDialog::marginHint());
    gl->addWidget(l, 1, 0, Qt::AlignRight);
    gl->addWidget(mSetBright, 1, 1);

    mSetContrast = new KScanSlider(page, i18n("Contrast"), -50, 50);
    mSetContrast->setValue(mTable->getContrast());
    connect(mSetContrast, SIGNAL(settingChanged(int)), mTable, SLOT(setContrast(int)));
    l = new QLabel(mSetContrast->label(), page);
    l->setBuddy(mSetContrast);
    gl->setRowMinimumHeight(2, KDialog::marginHint());
    gl->addWidget(l, 3, 0, Qt::AlignRight);
    gl->addWidget(mSetContrast, 3, 1);

    mSetGamma = new KScanSlider(page, i18n("Gamma"), 30, 300);
    mSetGamma->setValue(mTable->getGamma());
    connect(mSetGamma, SIGNAL(settingChanged(int)), mTable, SLOT(setGamma(int)));
    l = new QLabel(mSetGamma->label(), page);
    l->setBuddy(mSetGamma);
    gl->setRowMinimumHeight(4, KDialog::marginHint());
    gl->addWidget(l, 5, 0, Qt::AlignRight);
    gl->addWidget(mSetGamma, 5, 1);

    // Explanation label
    gl->setRowMinimumHeight(6, KDialog::marginHint());
    gl->setRowStretch(7, 1);

    l = new QLabel(i18n("This gamma table is passed to the scanner hardware."), page);
    l->setWordWrap(true);
    gl->addWidget(l, 8, 0, 1, 2, Qt::AlignLeft);

    // Gamma curve display
    mGtDisplay = new GammaWidget(mTable, page);
    mGtDisplay->resize(280, 280);
    gl->setColumnMinimumWidth(2, KDialog::marginHint());
    gl->addWidget(mGtDisplay, 0, 3, 9, 1);
    gl->setColumnStretch(3, 1);

    connect(this, SIGNAL(okClicked()), SLOT(slotApply()));
    connect(this, SIGNAL(applyClicked()), SLOT(slotApply()));
    connect(this, SIGNAL(resetClicked()), SLOT(slotReset()));
}

void GammaDialog::slotApply()
{
    emit gammaToApply(gammaTable());
}

void GammaDialog::slotReset()
{
    KGammaTable defaultGamma;               // construct with default values
    int b = defaultGamma.getBrightness();       // retrieve those values
    int c = defaultGamma.getContrast();
    int g = defaultGamma.getGamma();

    mSetBright->setValue(b);
    mSetContrast->setValue(c);
    mSetGamma->setValue(g);

    mTable->setAll(g, b, c);
}
