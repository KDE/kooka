/***************************************************** -*- mode:c++; -*- ***
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
 ***************************************************************************/

#ifndef KOOKASCANPARAMS_H
#define KOOKASCANPARAMS_H

#include <kled.h>

#include "scanparams.h"

class QComboBox;
class KMessageWidget;
class ScanParamsPage;
class AbstractDestination;


class KookaScanParams : public ScanParams
{
    Q_OBJECT

public:
    explicit KookaScanParams(QWidget *parent);
    ~KookaScanParams() override = default;

    AbstractDestination *destinationPlugin() const	{ return (mDestinationPlugin); }
    void saveDestinationSettings();

protected:
    /**
     * Reimplemented for a custom message.
     *
     * @see ScanParams::messageScannerNotSelected
     */
    QWidget *messageScannerNotSelected() override;

    /**
     * Reimplemented for the scan destination controls.
     *
     * @see ScanParams::createScanDestinationGUI
     */
    void createScanDestinationGUI(ScanParamsPage *frame) override;

signals:
    void actionSelectScanner();
    void actionAddScanner();

protected slots:
    void slotLinkActivated(const QString &link);
    void slotDestinationSelected(int idx);
    void slotDeviceConnected(KScanDevice *dev);

    void slotScanBatchStart();
    void slotScanBatchEnd(bool ok);

private:
    void setLED(const QColor &col, KLed::State state = KLed::On);

private:
    KMessageWidget *mNoScannerMessage;
    QComboBox *mDestinationCombo;

    ScanParamsPage *mParamsPage;
    int mParamsRow;

    AbstractDestination *mDestinationPlugin;
};

#endif                          // KOOKASCANPARAMS_H
