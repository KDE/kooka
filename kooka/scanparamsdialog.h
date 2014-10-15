/* This file is part of the KDE Project             -*- mode:c++ -*-
   Copyright (C) 2008 Jonathan Marten <jjm@keelhaul.me.uk>

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

#ifndef SCANPARAMSDIALOG_H
#define SCANPARAMSDIALOG_H

#include <kdialog.h>

#include "libkscan/kscanoptset.h"

class QLabel;
class QListWidget;
class QListWidgetItem;

class KScanDevice;

/**
 *  A dialogue to allow the user to enter a name and description for
 *  a set of saved scan parameters.
 */

class ScanParamsDialog : public KDialog
{
    Q_OBJECT

public:
    ScanParamsDialog(QWidget *parent, KScanDevice *scandev);

protected slots:
    void slotSelectionChanged(QListWidgetItem *item);
    void slotLoad();
    void slotSave();
    void slotDelete();
    void slotEdit();
    void slotLoadAndClose(QListWidgetItem *item);

private:
    void populateList();

    QLabel *descLabel;
    QListWidget *paramsList;
    QPushButton *buttonLoad;
    QPushButton *buttonSave;
    QPushButton *buttonDelete;
    QPushButton *buttonEdit;

    KScanOptSet::StringMap sets;
    KScanDevice *sane;
};

#endif                          // SCANPARAMSDIALOG_H
