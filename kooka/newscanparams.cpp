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
#include <klocale.h>
#include <kdebug.h>
#include <kvbox.h>

NewScanParams::NewScanParams(QWidget *parent,
                             const QString &name, const QString &desc, bool renaming)
    : KDialog(parent)
{
    setObjectName("NewScanParams");

    setModal(true);
    setButtons(KDialog::Ok | KDialog::Cancel);
    showButtonSeparator(true);

    KVBox *vb = new KVBox(this);
    vb->setMargin(KDialog::marginHint());
    vb->setSpacing(KDialog::spacingHint());
    setMainWidget(vb);

    if (renaming) {
        setCaption(i18n("Edit Scan Parameters"));
        new QLabel(i18n("Change the name and/or description of the scan parameter set."), vb);
    } else {
        setCaption(i18n("Save Scan Parameters"));
        new QLabel(i18n("Enter a name and description for the new scan parameter set."), vb);
    }
    new QLabel("", vb);

    QLabel *l = new QLabel(i18n("Set name:"), vb);
    mNameEdit = new QLineEdit(name, vb);
    connect(mNameEdit, SIGNAL(textChanged(const QString &)), SLOT(slotTextChanged()));
    l->setBuddy(mNameEdit);

    l = new QLabel(i18n("Description:"), vb);
    mDescEdit = new QLineEdit(desc, vb);
    connect(mDescEdit, SIGNAL(textChanged(const QString &)), SLOT(slotTextChanged()));
    l->setBuddy(mDescEdit);

    slotTextChanged();
    mNameEdit->setFocus();
}

void NewScanParams::slotTextChanged()
{
    bool ok = !mNameEdit->text().trimmed().isEmpty() &&
              !mDescEdit->text().trimmed().isEmpty();
    enableButtonOk(ok);
}

QString NewScanParams::getName() const
{
    return (mNameEdit->text());
}

QString NewScanParams::getDescription() const
{
    return (mDescEdit->text());
}
