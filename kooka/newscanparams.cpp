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

#include "newscanparams.h"

#include <qlabel.h>
#include <qlineedit.h>

#include <klineedit.h>
#include <KLocalizedString>
#include <QDebug>
#include <kvbox.h>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

NewScanParams::NewScanParams(QWidget *parent,
                             const QString &name, const QString &desc, bool renaming)
    : QDialog(parent)
{
    setObjectName("NewScanParams");

    setModal(true);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewScanParams::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &NewScanParams::reject);

    KVBox *vb = new KVBox(this);
//TODO PORT QT5     vb->setMargin(QDialog::marginHint());
//TODO PORT QT5     vb->setSpacing(QDialog::spacingHint());
    mainLayout->addWidget(vb);
    mainLayout->addWidget(buttonBox);

    if (renaming) {
        setWindowTitle(i18n("Edit Scan Parameters"));
        new QLabel(i18n("Change the name and/or description of the scan parameter set."), vb);
    } else {
        setWindowTitle(i18n("Save Scan Parameters"));
        new QLabel(i18n("Enter a name and description for the new scan parameter set."), vb);
    }
    new QLabel("", vb);

    QLabel *l = new QLabel(i18n("Set name:"), vb);
    mainLayout->addWidget(l);
    mNameEdit = new QLineEdit(name, vb);
    mainLayout->addWidget(mNameEdit);
    connect(mNameEdit, &QLineEdit::textChanged, this, &NewScanParams::slotTextChanged);
    l->setBuddy(mNameEdit);

    l = new QLabel(i18n("Description:"), vb);
    mainLayout->addWidget(l);
    mDescEdit = new QLineEdit(desc, vb);
    mainLayout->addWidget(mDescEdit);
    connect(mDescEdit, &QLineEdit::textChanged, this, &NewScanParams::slotTextChanged);
    l->setBuddy(mDescEdit);

    slotTextChanged();
    mNameEdit->setFocus();
}

void NewScanParams::slotTextChanged()
{
    bool ok = !mNameEdit->text().trimmed().isEmpty() &&
              !mDescEdit->text().trimmed().isEmpty();
    mOkButton->setEnabled(ok);
}

QString NewScanParams::getName() const
{
    return (mNameEdit->text());
}

QString NewScanParams::getDescription() const
{
    return (mDescEdit->text());
}
