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

#include "kscandevice.h"
#include "scansourcedialog.h"

#include <qframe.h>

#include <kicon.h>

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

class ScanSizeSelector;


// TODO: into class
typedef enum { ID_SANE_DEBUG, ID_QT_IMGIO, ID_SCAN } ScanMode;


class KSCAN_EXPORT ScanParams : public QFrame
{
    Q_OBJECT

public:
    ScanParams( QWidget *parent);
    ~ScanParams();

    bool connectDevice(KScanDevice *newScanDevice, bool galleryMode = false);

    KLed *operationLED() { return m_led; }

public slots:
    /**
     * sets the scan area to the default, which is the whole area.
     */
    void slotMaximalScanSize();

    /**
     * starts acquireing a preview image.
     * This ends up in a preview-signal of the scan-device object
     */
    void slotAcquirePreview();
    void slotStartScan();

    /**
     * connect this slot to KScanOptions Signal optionChanged to be informed
     * on a options change.
     */
    void slotOptionNotify(const KScanOption *so);

protected slots:
    /**
     * connected to the button which opens the source selection dialog
     */
    void slotSourceSelect();

    /**
     *  Slot to call if the virtual scanner mode is changed
     */
    void slotVirtScanModeSelect( int id );

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
     *  internal slot called when the slider for x resolution changes.
     *  In the slot, the signal scanResolutionChanged will be emitted, which
     *  is visible outside the scanparam-object to notify that the resolutions
     *  changed.
     *
     *  That is e.g. useful for size calculations
     */
    void slotNewXResolution(KScanOption *so);

    /**
     *  the same slot as @see slNewXResolution but for y resolution changes.
     */
    void slotNewYResolution(KScanOption *so);

    void slotNewScanMode();

    void slotScanSizeSelected(const QRect &rect);
    void slotNewPreviewRect(const QRect &rect);

signals:
    /**
     *  emitted if the resolution to scan changes. This signal may be connected
     *  to slots calculating the size of the image size etc.
     *
     *  As parameters the resolutions in x- and y-direction are coming.
     */
    void scanResolutionChanged(int xres,int yres);
    void scanModeChanged(int bytes_per_pix);

    void newCustomScanSize(const QRect &rect);

private:
    KScanStat prepareScan(QString *vfp);
    KScanStat performADFScan();
    void startProgress();

    void createNoScannerMsg(bool galleryMode);
    void initialise(KScanOption *opt);
    void initStartupArea();
    void setEditCustomGammaTableState();
    QScrollArea *createScannerParams();

    void applyRect(const QRect &rect);

    KScanDevice *sane_device;
    KScanOption *virt_filename;

    QCheckBox *cb_gray_preview;
    QPushButton *pb_edit_gtable;
    QProgressDialog *progressDialog;

    AdfBehaviour adf;
    ScanMode scan_mode;
    ScanSizeSelector *area_sel;

    KScanOption *xy_resolution_bind;
    KScanOption *source_sel;

    KScanOptSet *startupOptset;

    QPixmap pixLineArt, pixGray, pixColor, pixHalftone;
    KLed *m_led;
    bool m_firstGTEdit;

    KIcon mIconColor;
    KIcon mIconGray;
    KIcon mIconLineart;
    KIcon mIconHalftone;
};

#endif							// SCANPARAMS_H
