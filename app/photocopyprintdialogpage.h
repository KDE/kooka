/* This file is part of the KDE Project
   Copyright (C) 2008 Alex Kempshall <mcmurchy1917-kooka@yahoo.co.uk>

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

#ifndef PHOTOCOPYPRINTDIALOGPAGE_H
#define PHOTOCOPYPRINTDIALOGPAGE_H

#include <qmap.h>
#include <qcheckbox.h>

#ifndef KDE4
#include <kdeprint/kprintdialogpage.h>
#endif

#include "scanparams.h"

#include "kookaimage.h"

#define OPT_SCALING  "kde-kooka-scaling"
#define OPT_SCAN_RES "kde-kooka-scanres"
#define OPT_SCREEN_RES "kde-kooka-screenres"
#define OPT_WIDTH    "kde-kooka-width"
#define OPT_HEIGHT   "kde-kooka-height"
#define OPT_PSGEN_DRAFT  "kde-kooka-psdraft"
#define OPT_RATIO    "kde-kooka-ratio"
#define OPT_FITPAGE  "kde-kooka-fitpage"

class QWidget;
class QString;
class QLabel;
class Q3VGroupBox;

class KIntNumInput;

class ScanImage;
class KScanDevice;

#ifndef KDE4
class PhotoCopyPrintDialogPage: public KPrintDialogPage
#else
class PhotoCopyPrintDialogPage: public QWidget
#endif
{
    Q_OBJECT

public:
    explicit PhotoCopyPrintDialogPage(KScanDevice *newScanDevice);
    virtual ~PhotoCopyPrintDialogPage();

    void setOptions(const QMap<QString, QString> &opts);
    void getOptions(QMap<QString, QString> &opts, bool include_def = false);
    bool isValid(QString &msg);

private:
    QLabel *constructLabel(Q3VGroupBox *group, const char *strTitle, const QByteArray &strSaneOption);
    QLabel *constructLabel(Q3VGroupBox *group, const char *strTitle, const QString *strContents);

    KIntNumInput *m_copies;
    KScanDevice  *sane_device;

    ScanImage  *m_image;
    QLabel     *m_screenRes;
    bool        m_ignoreSignal;
};

#endif                          // PHOTOCOPYPRINTDIALOGPAGE_H
