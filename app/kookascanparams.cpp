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

#include <qicon.h>

#include <kmessagewidget.h>
#include <klocalizedstring.h>


KookaScanParams::KookaScanParams(QWidget *parent)
    : ScanParams(parent),
      mNoScannerMessage(nullptr)
{
}

QWidget *KookaScanParams::messageScannerNotSelected()
{
    if (mNoScannerMessage==nullptr)
    {
        mNoScannerMessage = new KMessageWidget(
            xi18nc("@info",
                   "<emphasis strong=\"1\">Gallery Mode - No scanner selected</emphasis>"
                   "<nl/><nl/>"
                   "In this mode you can browse, manipulate and OCR images already in the gallery."
                   "<nl/><nl/>"
                   "<link url=\"a:1\">Select a scanner device</link> "
                   "to perform scanning, or "
                   "<link url=\"a:2\">add a device</link> "
                   "if a scanner is not automatically detected."));

        mNoScannerMessage->setMessageType(KMessageWidget::Information);
        mNoScannerMessage->setIcon(QIcon::fromTheme("dialog-information"));
        mNoScannerMessage->setWordWrap(true);
        mNoScannerMessage->setCloseButtonVisible(false);
        connect(mNoScannerMessage, &KMessageWidget::linkActivated, this, &KookaScanParams::slotLinkActivated);
    }

    return (mNoScannerMessage);
}

void KookaScanParams::slotLinkActivated(const QString &link)
{
    if (link == QLatin1String("a:1")) {
        emit actionSelectScanner();
    } else if (link == QLatin1String("a:2")) {
        emit actionAddScanner();
    }
}
