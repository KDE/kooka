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

#include "newscanparams.h"

#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>

#include <klocalizedstring.h>


NewScanParams::NewScanParams(QWidget *parent,
                             const QString &name, const QString &desc, bool renaming)
    : DialogBase(parent)
{
    setObjectName("NewScanParams");

    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    QWidget *vb = new QWidget(this);
    QVBoxLayout *vbVBoxLayout = new QVBoxLayout(vb);

    QLabel *l;
    if (renaming)
    {
        setWindowTitle(i18n("Edit Scan Parameters"));
        l = new QLabel(i18n("Change the name and/or description of the scan parameter set."), vb);
    }
    else
    {
        setWindowTitle(i18n("Save Scan Parameters"));
        l = new QLabel(i18n("Enter a name and description for the new scan parameter set."), vb);
    }
    vbVBoxLayout->addWidget(l);

    l = new QLabel("", vb);
    vbVBoxLayout->addWidget(l);

    l = new QLabel(i18n("Set name:"), vb);
    vbVBoxLayout->addWidget(l);
    mNameEdit = new QLineEdit(name, vb);
    vbVBoxLayout->addWidget(mNameEdit);
    connect(mNameEdit, &QLineEdit::textChanged, this, &NewScanParams::slotTextChanged);
    l->setBuddy(mNameEdit);

    l = new QLabel(i18n("Description:"), vb);
    vbVBoxLayout->addWidget(l);
    mDescEdit = new QLineEdit(desc, vb);
    vbVBoxLayout->addWidget(mDescEdit);
    connect(mDescEdit, &QLineEdit::textChanged, this, &NewScanParams::slotTextChanged);
    l->setBuddy(mDescEdit);

    setMainWidget(vb);
    slotTextChanged();
    mNameEdit->setFocus();
}

void NewScanParams::slotTextChanged()
{
    setButtonEnabled(QDialogButtonBox::Ok, !mNameEdit->text().trimmed().isEmpty() &&
                                           !mDescEdit->text().trimmed().isEmpty());
}

QString NewScanParams::getName() const
{
    return (mNameEdit->text());
}

QString NewScanParams::getDescription() const
{
    return (mDescEdit->text());
}
