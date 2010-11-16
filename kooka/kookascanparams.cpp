/***************************************************************************
 *                                                                         *
 *  This file is part of Kooka, a KDE scanning/OCR application.            *
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *                                                                         *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *                                                                         *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 *  Author:  Jonathan Marten <jjm AT keelhaul DOT me DOT uk>               *
 *                                                                         *
 ***************************************************************************/

#include "kookascanparams.h"
#include "kookascanparams.moc"

#include <qlabel.h>

#include <klocale.h>


KookaScanParams::KookaScanParams(QWidget *parent)
    : ScanParams(parent)
{
    mNoScannerMessage = NULL;
}


QWidget *KookaScanParams::messageScannerNotSelected()
{
    if (mNoScannerMessage==NULL)
    {
        mNoScannerMessage = new QLabel(
            i18n("<qt>"
                 "<b>Gallery Mode - No scanner selected</b>"
                 "<p>"
                 "In this mode you can browse, manipulate and OCR images already in the gallery."
                 "<p>"
                 "<a href=\"a:1\">Select a scanner device</a> "
                 "to perform scanning, or "
                 "<a href=\"a:2\">add a device</a> "
                 "if a scanner is not automatically detected."));

        mNoScannerMessage->setWordWrap(true);
        connect(mNoScannerMessage, SIGNAL(linkActivated(const QString &)),
                SLOT(slotLinkActivated(const QString &)));
    }

    return (mNoScannerMessage);
}


void KookaScanParams::slotLinkActivated(const QString &link)
{
    if (link=="a:1") emit actionSelectScanner();
    else if (link=="a:2") emit actionAddScanner();
}
