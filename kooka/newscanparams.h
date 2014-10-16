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

#ifndef NEWSCANPARAMS_H
#define NEWSCANPARAMS_H

#include <QDialog>

class QLineEdit;
class QPushButton;
/**
 *  A dialogue to allow the user to enter a name and description for
 *  a set of saved scan parameters.
 */

class NewScanParams : public QDialog
{
    Q_OBJECT

public:
    NewScanParams(QWidget *parent, const QString &name, const QString &desc, bool renaming);

    QString getName() const;
    QString getDescription() const;

protected slots:
    void slotTextChanged();

private:
    QLineEdit *mNameEdit;
    QLineEdit *mDescEdit;
    QPushButton *mOkButton;
};

#endif                          // NEWSCANPARAMS_H
