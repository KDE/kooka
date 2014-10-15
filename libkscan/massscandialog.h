/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>

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

#ifndef MASSSCANDIALOG_H
#define MASSSCANDIALOG_H

#include "libkscanexport.h"

#include <kdialog.h>

class QProgressBar;
class QLabel;

/**
  *@author Klaas Freitag
  */

class KSCAN_EXPORT MassScanDialog : public KDialog
{
    Q_OBJECT

public:
    MassScanDialog(QWidget *parent);
    ~MassScanDialog();

public slots:

    void slotStartScan();
    void slotStopScan();
    void slotFinished();

    // TODO: needs to be a slot?
    void setPageProgress(int p);

private:
    QString     scanopts;
    QLabel      *l_scanopts;

    QString     tofolder;
    QLabel      *l_tofolder;

    QString     progress;
    QLabel      *l_progress;

    QProgressBar *progressbar;
};

#endif                          // MASSSCANDIALOG_H
