/* This file is part of the KDE Project
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>

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

#include "imgscaledialog.h"

#include <qbuttongroup.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qlabel.h>

#include <klocale.h>
#include <QDebug>
#include <knumvalidator.h>
#include <klineedit.h>

/* ############################################################################## */

ImgScaleDialog::ImgScaleDialog(QWidget *parent, int curr_sel)
    : KDialog(parent)
{
    setObjectName("ImgScaleDialog");

    setButtons(KDialog::Ok | KDialog::Cancel);
    setCaption(i18n("Image Zoom"));
    setModal(true);
    showButtonSeparator(true);

    selected = curr_sel;

    QWidget *radios = new QWidget(this);
    setMainWidget(radios);

    QButtonGroup *radiosGroup = new QButtonGroup(radios);
    connect(radiosGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &ImgScaleDialog::slotSetSelValue);

    QVBoxLayout *radiosLayout = new QVBoxLayout(this);

    // left column: smaller Image
    QHBoxLayout *hbox = new QHBoxLayout();
    QVBoxLayout *vbox = new QVBoxLayout();

    QRadioButton *rb25 = new QRadioButton(i18n("25 %"));
    if (curr_sel == 25) {
        rb25->setChecked(true);
    }
    vbox->addWidget(rb25);
    radiosGroup->addButton(rb25, 0);

    QRadioButton *rb50 = new QRadioButton(i18n("50 %"));
    if (curr_sel == 50) {
        rb50->setChecked(true);
    }
    vbox->addWidget(rb50);
    radiosGroup->addButton(rb50, 1);

    QRadioButton *rb75 = new QRadioButton(i18n("75 %"));
    if (curr_sel == 75) {
        rb75->setChecked(true);
    }
    vbox->addWidget(rb75);
    radiosGroup->addButton(rb75, 2);

    QRadioButton *rb100 = new QRadioButton(i18n("&100 %"));
    if (curr_sel == 100) {
        rb100->setChecked(true);
    }
    vbox->addWidget(rb100);
    radiosGroup->addButton(rb100, 3);

    hbox->addLayout(vbox);

    // right column: bigger image:
    vbox = new QVBoxLayout();

    QRadioButton *rb150 = new QRadioButton(i18n("150 &%"));
    if (curr_sel == 150) {
        rb150->setChecked(true);
    }
    vbox->addWidget(rb150);
    radiosGroup->addButton(rb150, 4);

    QRadioButton *rb200 = new QRadioButton(i18n("200 %"));
    if (curr_sel == 200) {
        rb200->setChecked(true);
    }
    vbox->addWidget(rb200);
    radiosGroup->addButton(rb200, 5);

    QRadioButton *rb300 = new QRadioButton(i18n("300 %"));
    if (curr_sel == 300) {
        rb300->setChecked(true);
    }
    vbox->addWidget(rb300);
    radiosGroup->addButton(rb300, 6);

    QRadioButton *rb400 = new QRadioButton(i18n("400 %"));
    if (curr_sel == 400) {
        rb400->setChecked(true);
    }
    vbox->addWidget(rb400);
    radiosGroup->addButton(rb400, 7);

    hbox->addLayout(vbox);
    radiosLayout->addLayout(hbox);

    radiosLayout->addSpacing(KDialog::spacingHint());

    // Custom Scaler at the bottom:
    hbox = new QHBoxLayout();

    QRadioButton *rbCust = new QRadioButton(i18n("Custom:"));
    if (radiosGroup->checkedId() < 0) {
        rbCust->setChecked(true);
    }
    connect(rbCust, &QRadioButton::toggled, this, &ImgScaleDialog::slotEnableAndFocus);

    hbox->addWidget(rbCust);
    radiosGroup->addButton(rbCust, 8);

    leCust = new KLineEdit();
    QString sn;
    sn.setNum(curr_sel);
    leCust->setValidator(new KIntValidator(leCust));
    leCust->setText(sn);
    connect(leCust, &KLineEdit::textChanged, this, &ImgScaleDialog::slotCustomChanged);
    hbox->addWidget(leCust);
    hbox->setStretchFactor(leCust, 1);

    hbox->addWidget(new QLabel("%", this));

    radiosLayout->addLayout(hbox);
    radiosLayout->addStretch();

    radios->setLayout(radiosLayout);

    slotEnableAndFocus(rbCust->isChecked());
}

void ImgScaleDialog::slotCustomChanged(const QString &s)
{
    bool ok;
    int okval = s.toInt(&ok);
    if (ok && okval >= 5 && okval <= 1000) {
        selected = okval;
        enableButtonOk(true);
        emit customScaleChange(okval);
    } else {
        enableButtonOk(false);
    }
}

// This slot is called, when the user changes the Scale-Selection
// in the button group. The value val is the index of the active
// button which is translated to the Scale-Size in percent.
// If custom size is selected, the ScaleSize is read from the
// QLineedit.
//
void ImgScaleDialog::slotSetSelValue(int val)
{
    const int translator[] = { 25, 50, 75, 100, 150, 200, 300, 400, -1 };
    const int translator_size = sizeof(translator) / sizeof(int);
    int old_sel = selected;

    // Check if value is in Range
    if (val >= 0 && val < translator_size) {
        selected = translator[val];

        // Custom size selected
        if (selected == -1) {
            QString s = leCust->text();

            bool ok;
            int  okval = s.toInt(&ok);
            if (ok) {
                selected = okval;
                emit(customScaleChange(okval));
            } else {
                selected = old_sel;
            }
        } // Selection is not custom
    } else {
        //qDebug() << "Error: Invalid size selected!" << val;
    }
}

int ImgScaleDialog::getSelected() const
{
    return (selected);
}

void ImgScaleDialog::slotEnableAndFocus(bool b)
{
    leCust->setEnabled(b);
    if (b) {
        leCust->setFocus();
    }
}
