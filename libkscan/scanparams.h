/* This file is part of the KDE Project			-*- mode:c++; -*-
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>

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

#ifndef SCANPARAMS_H
#define SCANPARAMS_H

#include "libkscanexport.h"

#include <qwidget.h>
#include <qicon.h>

#include "kscandevice.h"
#include "scansourcedialog.h"

/**
  *@author Klaas Freitag
  */

class QScrollArea;
class QProgressDialog;
class QPushButton;
class QCheckBox;

class KScanOption;
class KGammaTable;
class KScanOptSet;
class KLed;
class KTabWidget;

class ScanParamsPage;
class ScanSizeSelector;


class KSCAN_EXPORT ScanParams : public QWidget
{
    Q_OBJECT

public:
    enum ScanMode
    {
        SaneDebugMode = 0,				// order fixed by GUI buttons
        VirtualScannerMode = 1,
        NormalMode = 2
    };

    ScanParams(QWidget *parent);
    ~ScanParams();

    bool connectDevice(KScanDevice *newScanDevice, bool galleryMode = false);

    KLed *operationLED() const { return (mLed); }

public slots:
    void slotAcquirePreview();
    void slotStartScan();

protected slots:
    /**
     * connected to the button which opens the source selection dialog
     */
    void slotSourceSelect();

    /**
     *  Slot to call if the virtual scanner mode is changed
     */
    void slotVirtScanModeSelect(int but);

    /**
     *  Slot for result on an edit-Custom Gamma Table request.
     *  Starts a dialog.
     */
    void slotEditCustGamma();

    /**
     *  Slot called if a Gui-Option changed due to user action, eg. the
     *  user selects another entry in a List.
     *  Action to do is to apply the new value and check, if it affects others.
     */
    void slotReloadAllGui(KScanOption *so);

    /**
     *  Slot called when the Edit Custom Gamma-Dialog has a new gamma table
     *  to apply. This is an internal slot.
     */
    void slotApplyGamma(const KGammaTable *gt);

    /**
     *  Internal slot called when the setting for X or Y resolution changes.
     *  The signal scanResolutionChanged() will be emitted, which notifies
     *  that the resolutions have changed.  Useful e.g. for size calculations.
     */
    void slotNewResolution(KScanOption *so);

    void slotNewScanMode();

    void slotScanSizeSelected(const QRect &rect);
    void slotNewPreviewRect(const QRect &rect);

signals:
    /**
     *  Emitted if the resolution to scan or the scan mode/depth changes.
     *  These signals may be used to recalculate the image size, etc.
     */
    void scanResolutionChanged(int xres, int yres);
    void scanModeChanged(int bytes_per_pix);

    void newCustomScanSize(const QRect &rect);

private:
    KScanDevice::Status prepareScan(QString *vfp);
    KScanDevice::Status performADFScan();
    void startProgress();

    void createNoScannerMsg(bool galleryMode);
    void initialise(KScanOption *opt);
    void initStartupArea();
    void setEditCustomGammaTableState();

    QWidget *createScannerParams();
    ScanParamsPage *createTab(KTabWidget *tw, const QString &title, const char *name = NULL);

    void applyRect(const QRect &rect);
    void setMaximalScanSize();

    KScanDevice *mSaneDevice;
    KScanOption *mVirtualFile;

    QPushButton *mGammaEditButt;
    QProgressDialog *mProgressDialog;

    AdfBehaviour adf;
    ScanParams::ScanMode mScanMode;
    ScanSizeSelector *area_sel;

    KScanOption *mResolutionBind;
    KScanOption *mSourceSelect;

    KScanOptSet *startupOptset;

    QPixmap pixLineArt, pixGray, pixColor, pixHalftone;
    KLed *mLed;
    bool mFirstGTEdit;

    QIcon mIconColor;
    QIcon mIconGray;
    QIcon mIconLineart;
    QIcon mIconHalftone;
};

#endif							// SCANPARAMS_H
