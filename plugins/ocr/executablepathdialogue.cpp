/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2018 Jonathan Marten <jjm@keelhaul.me.uk>		*
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

#include "executablepathdialogue.h"

#include <qlabel.h>
#include <qboxlayout.h>

#include <klocalizedstring.h>
#include <kurlrequester.h>


ExecutablePathDialogue::ExecutablePathDialogue(QWidget *pnt)
    : DialogBase(pnt)
{
    setObjectName("ExecutablePathDialogue");
    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    setWindowTitle(i18n("Executable Path"));

    QWidget *page = new QWidget(this);
    setMainWidget(page);

    QVBoxLayout *lay = new QVBoxLayout(page);

    mLabel = new QLabel(i18n("Executable path:"), this);
    lay->addWidget(mLabel);

    mPathRequester = new KUrlRequester(QUrl("file:///usr/bin/"), this);
    mPathRequester->setPlaceholderText(i18n("Enter or select the path..."));
    mPathRequester->setAcceptMode(QFileDialog::AcceptOpen);
    mPathRequester->setMimeTypeFilters(QStringList() << "application/x-executable");
    mPathRequester->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    mPathRequester->setStartDir(QUrl("file:///usr/bin/"));
    connect(mPathRequester, &KUrlRequester::textChanged, this, &ExecutablePathDialogue::slotTextChanged);
    lay->addWidget(mPathRequester);
    mLabel->setBuddy(mPathRequester);

    lay->addStretch();
}


void ExecutablePathDialogue::setPath(const QString &exec)
{
    mPathRequester->setUrl(QUrl::fromLocalFile(exec));
}


void ExecutablePathDialogue::setLabel(const QString &text)
{
    mLabel->setText(text);
}


QString ExecutablePathDialogue::path() const
{
    return (mPathRequester->url().path());
}


void ExecutablePathDialogue::slotTextChanged(const QString &text)
{
    setButtonEnabled(QDialogButtonBox::Ok, !text.isEmpty());
}
