/* This file is part of the KDE Project			-*- mode:c++; -*-
   Copyright (C) 2010 Jonathan Marten <jjm@keelhaul.me.uk>

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

#ifndef SCANPARAMS_P_H
#define SCANPARAMS_P_H

#include <qwidget.h>
#include <qlabel.h>
#include <qgridlayout.h>

#include <kdialog.h>


//  Limits for the scan option and label, to keep them to reasonable sizes
#define MAX_CONTROL_WIDTH		300
#define MAX_LABEL_WIDTH			150
#define MIN_LABEL_WIDTH			50


/**
 * This class represents a single options page (Basic, Other or Advanced)
 * within the scan parameter GUI.
 *
 * @author Jonathan Marten
 */

class ScanParamsPage : public QWidget
{
    Q_OBJECT

public:
    /**
     * Create a new options page.
     *
     * @param parent  The parent widget
     * @param name    Qt object name
     */
    ScanParamsPage(QWidget *parent, const char *name = NULL);

    /**
     * Destructor.
     */
    virtual ~ScanParamsPage();

    /**
     * Add a standard parameter row to the page.
     *
     * @param lab    Label for the parameter, normally from KScanOption::getlabel()
     *               or @c NULL for no label.
     * @param wid    Scan control widget, normally from KScanOption::widget().
     * @param unit   SANE unit label, normally from KScanOption::getUnit() or
     *               @c NULL for no unit label.
     * @param align  Special alignment flags for layout row.

     * @note The layout coloumns are used as follows:  the @p lab label goes in
     * column 0, left aligned.  Column 1 is a spacer.  If the @p unit label is
     * present then it goes in column 3 and the @p wid widget in column 2.  If
     * there is no @p unit label then the @p wid widget spans columns 2 and 3.
     */
    void addRow(QLabel *lab, QWidget *wid, QLabel *unit = NULL, Qt::Alignment align = 0);

    /**
     * Add a full width widget to the page.
     *
     * @param wid    The widget to add.
     */
    void addRow(QWidget *wid);

    /**
     * Finish off the layout by adding a stretch widget (so that all the created
     * parameters are aligned to the top), and check whether any widgets have
     * been added.
     *
     * @return @c true if any paramater widgets are present
     */
    bool lastRow();

private:
    QGridLayout *mLayout;
    int mNextRow;
};


ScanParamsPage::ScanParamsPage(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setObjectName(name);

    mLayout = new QGridLayout(this);
    mLayout->setSpacing(2*KDialog::spacingHint());
    mLayout->setColumnStretch(2, 1);
    mLayout->setColumnMinimumWidth(1, KDialog::marginHint());

    mNextRow = 0;
}


ScanParamsPage::~ScanParamsPage()
{
}


void ScanParamsPage::addRow(QWidget *wid)
{
    if (wid==NULL) return;				// must have one
    mLayout->addWidget(wid, mNextRow, 0, 1, -1);
    ++mNextRow;
}


void ScanParamsPage::addRow(QLabel *lab, QWidget *wid, QLabel *unit, Qt::Alignment align)
{
    if (wid==NULL) return;				// must have one
    wid->setMaximumWidth(MAX_CONTROL_WIDTH);

    if (lab!=NULL)
    {
        lab->setMaximumWidth(MAX_LABEL_WIDTH);
        lab->setMinimumWidth(MIN_LABEL_WIDTH);
        mLayout->addWidget(lab, mNextRow, 0, Qt::AlignLeft|align);
    }

    if (unit!=NULL)
    {
        mLayout->addWidget(wid, mNextRow, 2, align);
        mLayout->addWidget(unit, mNextRow, 3, Qt::AlignLeft|align);
    }
    else
    {
        mLayout->addWidget(wid, mNextRow, 2, 1, 2, align);
    }

    ++mNextRow;
}


bool ScanParamsPage::lastRow()
{
    mLayout->addWidget(new QLabel(QString::null, this), mNextRow, 0, 1, -1, Qt::AlignTop);
    mLayout->setRowStretch(mNextRow, 9);

    return (mNextRow>0);
}

#endif							// SCANPARAMS_P_H
