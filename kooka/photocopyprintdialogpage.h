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

#ifndef __PHOTOCOPYPRINTDIALOGPAGE_H__
#define __PHOTOCOPYPRINTDIALOGPAGE_H__

#include <qmap.h>
#include <qcheckbox.h>
#include <kdeprint/kprintdialogpage.h>
#include <scanparams.h>
#include "kookaimage.h"
#include <qvgroupbox.h>

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
class KIntNumInput;
class KookaImage;
class QVButtonGroup;
class QRadioButton;
class QCheckBox;
class KScanDevice;
class Previewer;

class PhotoCopyPrintDialogPage: public KPrintDialogPage
{
    Q_OBJECT
public:
    PhotoCopyPrintDialogPage( KScanDevice* );
    ~PhotoCopyPrintDialogPage();
    void setOptions(const QMap<QString,QString>& opts);
    void getOptions(QMap<QString,QString>& opts, bool include_def = false);
    bool isValid(QString& msg);

public slots:

private:
    QLabel* constructLabel(QVGroupBox*, char*, const QCString& );
    QLabel* constructLabel(QVGroupBox*, char*, QString* );

    KIntNumInput *m_copies;
    KScanDevice*  sane_device;

    KookaImage *m_image;
    QLabel     *m_screenRes;
    bool        m_ignoreSignal;
};

#endif
