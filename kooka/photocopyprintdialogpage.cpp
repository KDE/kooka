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

#include "photocopyprintdialogpage.h"

#include <qstring.h>
#include <qmap.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <q3vgroupbox.h>
#include <q3hgroupbox.h>
#include <qlabel.h>

#include <QDebug>
#include <KLocalizedString>
#include <knuminput.h>
#include <kdialog.h>

extern "C" {
#include <sane/saneopts.h>
}

#include "kscanoption.h"

#include "kookaimage.h"

#define ID_SCREEN 0
#define ID_ORIG   1
#define ID_CUSTOM 2
#define ID_FIT_PAGE 3

PhotoCopyPrintDialogPage::PhotoCopyPrintDialogPage(KScanDevice *newScanDevice)
#ifndef KDE4
    : KPrintDialogPage()
#else
    : QWidget()
#endif
{
    //qDebug();

#ifndef KDE4
    setTitle(i18n("PhotoCopier"));
#endif

    sane_device = newScanDevice;

    QVBoxLayout *leftVBox = new QVBoxLayout(this);

    Q3HGroupBox *prtgroup1 = new Q3HGroupBox(i18n("Print options"), this);

    /* Allow for number of Copies */
    m_copies = new KIntNumInput(prtgroup1);
    m_copies->setLabel(i18n("Copies: "), Qt::AlignVCenter);
    m_copies->setValue(1);
    m_copies->setMinimum(1);
    // TODO: is this correct?  maximum number of copies 1?
    m_copies->setMaximum(1);

    QHBoxLayout *prtHBox = new QHBoxLayout(this);

    prtHBox->addWidget(prtgroup1);

//    QWidget *prtSpaceEater = new QWidget( prtgroup1 );
//    prtSpaceEater->setSizePolicy( QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ));
//    prtHBox->addWidget( prtSpaceEater );

    prtHBox->addStretch(1);

// Scanner options
    QString str = i18n("Scanner Settings: %1", sane_device->scannerDescription());
    Q3VGroupBox *scangroup1 = new Q3VGroupBox(str, this);

    str = i18n("Scan size:");
    QString strBR_X;
    QString strBR_Y;

    KScanOption *res = sane_device->getOption(SANE_NAME_SCAN_BR_X, true);
    if (res != NULL) {
        strBR_X = res->get();
    }

    res = sane_device->getOption(SANE_NAME_SCAN_BR_Y, true);
    if (res != NULL) {
        strBR_Y = res->get();
    }

    QString str1 = strBR_Y + "x" + strBR_X;
//if (strBR_X == "215" && strBR_Y == "295") str1 ="Letter";
//if (strBR_X == "215" && strBR_Y == "297") str1 ="A4";
//if (strBR_X == "151" && strBR_Y == "211") str1 ="A5";
//if (strBR_X == "107" && strBR_Y == "148") str1 ="A6";

    constructLabel(scangroup1, "Scan Size", &str1);
    constructLabel(scangroup1, "Scan Mode", SANE_NAME_SCAN_MODE);
    constructLabel(scangroup1, "Resolution", SANE_NAME_SCAN_RESOLUTION);
    constructLabel(scangroup1, "Brightness", SANE_NAME_BRIGHTNESS);
    constructLabel(scangroup1, "Contrast", SANE_NAME_CONTRAST);

    QHBoxLayout *scanHBox = new QHBoxLayout();
    scanHBox->addWidget(scangroup1);

    scanHBox->addStretch(1);

    leftVBox->addLayout(scanHBox);
    leftVBox->addLayout(prtHBox);

}

PhotoCopyPrintDialogPage::~PhotoCopyPrintDialogPage()
{
    //qDebug();
}

void PhotoCopyPrintDialogPage::setOptions(const QMap<QString, QString> &opts)
{
    //qDebug();
}

void PhotoCopyPrintDialogPage::getOptions(QMap<QString, QString> &opts, bool)
{
    //qDebug();
}

bool PhotoCopyPrintDialogPage::isValid(QString &msg)
{
    return (true);
}

QLabel *PhotoCopyPrintDialogPage::constructLabel(Q3VGroupBox *group, const char *strTitle, const QByteArray &strSaneOption)
{
    KScanOption *res = sane_device->getOption(strSaneOption, true);
    QString str = i18n(strTitle) + ": " + "\t" + (res != NULL ? res->get() : "?");
    QLabel *lbl = new QLabel(group);
    lbl->setText(str);
    return (lbl);
}

// The unusual 'const QString *' parameter is here so that this function can
// be overloaded with the previous.  If this were 'const QString &' then the
// overload would be ambiguous (because there is a converion from QByteArray
// to QString).

QLabel *PhotoCopyPrintDialogPage::constructLabel(Q3VGroupBox *group, const char *strTitle, const QString *strContents)
{
    QString str = i18n(strTitle) + ": " + "\t" + (*strContents);
    QLabel *lbl = new QLabel(group);
    lbl->setText(str);
    return (lbl);
}
