/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2020 Jonathan Marten <jjm@keelhaul.me.uk>		*
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

#include "scanparamspage.h"

#include <qgridlayout.h>
#include <qlabel.h>

#include "dialogbase.h"


ScanParamsPage::ScanParamsPage(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setObjectName(name);

    mLayout = new QGridLayout(this);
    mLayout->setSpacing(2*DialogBase::verticalSpacing());
    mLayout->setColumnStretch(2, 1);
    mLayout->setColumnMinimumWidth(1, 2*DialogBase::horizontalSpacing());

    mNextRow = 0;
    mPendingGroup = nullptr;
}


void ScanParamsPage::checkPendingGroup()
{
    if (mPendingGroup != nullptr) {            // separator to add first?
        QWidget *w = mPendingGroup;
        mPendingGroup = nullptr;               // avoid recursion!
        addRow(w);
    }
}


void ScanParamsPage::addRow(QWidget *wid)
{
    if (wid == nullptr) {
        return;    // must have one
    }

    checkPendingGroup();                // add separator if needed
    mLayout->addWidget(wid, mNextRow, 0, 1, -1);
    ++mNextRow;
}


void ScanParamsPage::addRow(QLabel *lab, QWidget *wid, QLabel *unit, Qt::Alignment align)
{
    if (wid == nullptr) {
        return;    // must have one
    }
    wid->setMaximumWidth(MAX_CONTROL_WIDTH);

    checkPendingGroup();                // add separator if needed

    if (lab != nullptr) {
        lab->setMaximumWidth(MAX_LABEL_WIDTH);
        lab->setMinimumWidth(MIN_LABEL_WIDTH);
        mLayout->addWidget(lab, mNextRow, 0, Qt::AlignLeft | align);
    }

    if (unit != nullptr) {
        mLayout->addWidget(wid, mNextRow, 2, align);
        mLayout->addWidget(unit, mNextRow, 3, Qt::AlignLeft | align);
    } else {
        mLayout->addWidget(wid, mNextRow, 2, 1, 2, align);
    }

    ++mNextRow;
}


void ScanParamsPage::addRow(const QString &lab, QWidget *wid, QLabel *unit, Qt::Alignment align)
{
    QLabel *l = nullptr;
    if (!lab.isEmpty())
    {
        l = new QLabel(lab, this);
        l->setBuddy(wid);
    }
    addRow(l, wid, unit, align);
}


void ScanParamsPage::clearRow()
{
    for (int c = 0; c<mLayout->columnCount(); ++c)
    {
        QLayoutItem *item = mLayout->itemAtPosition(mNextRow, c);
        if (item==nullptr) continue;
        QWidget *w = item->widget();
        if (w!=nullptr) w->deleteLater();
    }
}


bool ScanParamsPage::lastRow()
{
    addGroup(nullptr);					// hide last if present

    mLayout->addWidget(new QLabel(QString(), this), mNextRow, 0, 1, -1, Qt::AlignTop);
    mLayout->setRowStretch(mNextRow, 9);

    return (mNextRow > 0);
}


void ScanParamsPage::addGroup(QWidget *wid)
{
    if (mPendingGroup!=nullptr)
    {
        mPendingGroup->hide();				// don't need this after all
    }

    mPendingGroup = wid;
}
