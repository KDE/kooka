/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "gammadialog.h"

#include <qlabel.h>
#include <qgridlayout.h>
#include <qdialogbuttonbox.h>
#include <qpushbutton.h>

#include <klocalizedstring.h>
#include <kgammatable.h>

#include "kscancontrols.h"
#include "gammawidget.h"


GammaDialog::GammaDialog(const KGammaTable *table, QWidget *parent)
    : DialogBase(parent)
{
    setObjectName("GammaDialog");

    setModal(true);
    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply|QDialogButtonBox::Reset);
    setWindowTitle(i18n("Edit Gamma Table"));

    mTable = new KGammaTable(*table);			// take our own copy

    QWidget *page = new QWidget(this);
    QGridLayout *gl = new QGridLayout(page);

    // Sliders for brightness, contrast, gamma
    mSetBright = new KScanSlider(page, i18n("Brightness"));
    mSetBright->setRange(-50, 50);
    mSetBright->setValue(mTable->getBrightness());
    connect(mSetBright, QOverload<int>::of(&KScanSlider::settingChanged), mTable, &KGammaTable::setBrightness);
    QLabel *l = new QLabel(mSetBright->label(), page);
    l->setBuddy(mSetBright);
    gl->setRowMinimumHeight(0, verticalSpacing());
    gl->addWidget(l, 1, 0, Qt::AlignRight);
    gl->addWidget(mSetBright, 1, 1);

    mSetContrast = new KScanSlider(page, i18n("Contrast"));
    mSetContrast->setRange(-50, 50);
    mSetContrast->setValue(mTable->getContrast());
    connect(mSetContrast, QOverload<int>::of(&KScanSlider::settingChanged), mTable, &KGammaTable::setContrast);
    l = new QLabel(mSetContrast->label(), page);
    l->setBuddy(mSetContrast);
    gl->setRowMinimumHeight(2, verticalSpacing());
    gl->addWidget(l, 3, 0, Qt::AlignRight);
    gl->addWidget(mSetContrast, 3, 1);

    mSetGamma = new KScanSlider(page, i18n("Gamma"));
    mSetGamma->setRange(30, 300);
    mSetGamma->setValue(mTable->getGamma());
    connect(mSetGamma, QOverload<int>::of(&KScanSlider::settingChanged), mTable, &KGammaTable::setGamma);
    l = new QLabel(mSetGamma->label(), page);
    l->setBuddy(mSetGamma);
    gl->setRowMinimumHeight(4, verticalSpacing());
    gl->addWidget(l, 5, 0, Qt::AlignRight);
    gl->addWidget(mSetGamma, 5, 1);

    // Explanation label
    gl->setRowMinimumHeight(6, verticalSpacing());
    gl->setRowStretch(7, 1);

    l = new QLabel(i18n("This gamma table is passed to the scanner hardware."), page);
    l->setWordWrap(true);
    gl->addWidget(l, 8, 0, 1, 2, Qt::AlignLeft);

    // Gamma curve display
    mGtDisplay = new GammaWidget(mTable, page);
    mGtDisplay->resize(280, 280);
    gl->setColumnMinimumWidth(2, horizontalSpacing());
    gl->addWidget(mGtDisplay, 0, 3, -1, 1);
    gl->setColumnStretch(3, 1);
//    gl->setColumnStretch(1, 1);

    setMainWidget(page);
    connect(buttonBox()->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &GammaDialog::slotApply);
    connect(buttonBox()->button(QDialogButtonBox::Reset), &QAbstractButton::clicked, this, &GammaDialog::slotReset);
}


void GammaDialog::slotApply()
{
    emit gammaToApply(gammaTable());
}


void GammaDialog::slotReset()
{
    KGammaTable defaultGamma;				// construct with default values
    int b = defaultGamma.getBrightness();		// retrieve those values
    int c = defaultGamma.getContrast();
    int g = defaultGamma.getGamma();

    mSetBright->setValue(b);
    mSetContrast->setValue(c);
    mSetGamma->setValue(g);

    mTable->setAll(g, b, c);
}
