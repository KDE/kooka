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


class QScrollArea;
class QProgressDialog;
class QPushButton;
class QCheckBox;
class QLabel;

class KScanOption;
class KGammaTable;
class KScanOptSet;
class KLed;
class KTabWidget;

class ScanParamsPage;
class ScanSizeSelector;


/**
 * @short Scanner settings GUI.
 *
 * The most well-known and important scanner options are placed in a
 * "Basic" tab.  These are settings such as the scan mode and resolution,
 * area and document source.
 *
 * Other scanner options are placed either in an "Other" or "Advanced" tab,
 * depending on how they are classified by the SANE backend.
 *
 * The scanner name/desciption and the operation LED appear above the
 * scanner settings, and the "Preview" and "Scan" action buttons below.
 *
 * This class controls the scanner operation (via its @c KScanDevice) and
 * the popup scanning progress dialogue.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class KSCAN_EXPORT ScanParams : public QWidget
{
    Q_OBJECT

public:
    /**
     * Create the scanner options container widget.  The GUI for the
     * scanner settings is not actually created until the scanner device is
     * specified with @c connectDevice().
     *
     * @param parent The parent widget
     **/
    ScanParams(QWidget *parent);

    /**
     * Destructor.
     **/
    virtual ~ScanParams();

    /**
     * Specify a scanner device and create the GUI for its settings.
     *
     * @param newScanDevice The scan device to use, or @c NULL if no
     * scanner device is to be used.
     * @param galleryMode If this is @c true, then a @c NULL value for
     * the @p newScanDevice will be allowed and a message will be
     * displayed in place of the scanner settings.  This message will
     * be as created by @c messageScannerNotSelected().  If this
     * parameter is @c false and the @p newScanDevice parameter is
     * @c NULL, the message as created by @c messageScannerProblem()
     * will be displayed.
     * @return @c true in all cases.
     **/
    bool connectDevice(KScanDevice *newScanDevice, bool galleryMode = false);

    /**
     * Get the operation "LED" widget.
     *
     * @return The widget, or @c NULL if it has not been created yet.
     * Do not delete this, it is owned by the @c ScanParams object.
     **/
    KLed *operationLED() const		{ return (mLed); }

public slots:
    /**
     * Start to acquire a scan preview.  This is internally connected
     * and called when the "Preview" button is clicked.
     *
     * @see slotStartScan
     **/
    void slotAcquirePreview();

    /**
     * Start to acquire a scan.  This is internally connected and called
     * when the "Start Scan" button is clicked.
     *
     * @see slotAcquirePreview
     **/
    void slotStartScan();

protected:
    /**
     * Create a widget (for example, a QLabel showing an appropriate message)
     * which is displayed if no scanner is selected.  This happens if
     * the @c newScanDevice parameter to @c connectDevice() is @c NULL and
     * the @c galleryMode parameter is @c true.
     *
     * @return the created widget
     *
     * @note The returned widget is added to the GUI, so it will be deleted
     * when the @c ScanParams object is deleted.
     *
     * @see messageScannerProblem
     **/
    virtual QWidget *messageScannerNotSelected();

    /**
     * Create a widget (for example, a QLabel showing an appropriate message)
     * which is displayed if a scanner is selected but there is an error
     * opening or connecting to it.  This happens if the @c newScanDevice
     * parameter to @c connectDevice() is @c NULL but the @c galleryMode
     * parameter is @c false (the default).
     *
     * @return the created widget
     *
     * @note The returned widget is added to the GUI, so it will be deleted
     * when the @c ScanParams object is deleted.
     *
     * @see messageScannerNotSelected
     **/
    virtual QWidget *messageScannerProblem();

protected slots:
    /**
     * Open the source selection dialogue.  Internally connected
     * to the GUI button, if one is present.
     **/
    void slotSourceSelect();

    /**
     * Change the virtual scan mode.  Internally connected to the
     * GUI radio button group, if this setting is present.
     *
     * @param but The activated button index within the group.
     **/
    void slotVirtScanModeSelect(int but);

    /**
     * Open the gamma table edit dialogue.  Internally connected
     * to the GUI button, if one is present.
     **/
    void slotEditCustGamma();

    /**
     * The setting of one of the scanner controls has changed.
     * Inform the scanner device and check whether any other options
     * are affected.
     *
     * @param so The option that has changed.
     **/
    void slotOptionChanged(KScanOption *so);

    /**
     * The gamma table has been edited via the gamma table edit dialogue.
     * Send the new gamma table to the scanner.
     *
     * @param gt The new gamma table.
     **/
    void slotApplyGamma(const KGammaTable *gt);

    /**
     * The setting for the X or Y, or both, resolutions has changed.
     * Calculate the new resolution and emit the @c scanResolutionChanged()
     * signal.
     *
     * @param so The option that has changed.
     *
     * @see scanResolutionChanged
     **/
    void slotNewResolution(KScanOption *so);

    /**
     * The setting for the scan mode (colour, greyscale etc.) has changed.
     * Calculate the new bit depth and emit the @c scanModeChanged()
     * signal.
     *
     * @see scanModeChanged
     **/
    void slotNewScanMode();

    /**
     * A new preset scan size or orientation has been chosen by the user.
     * Calculate the new scan area and emit the @c newCustomScanSize()
     * signal.
     *
     * @param rect The new scan area, or a null rectangle (@c QRect())
     * for the default maximum scan area.
     *
     * @see newCustomScanSize
     **/
    void slotScanSizeSelected(const QRect &rect);

    /**
     * A new scan area has been drawn on or auto-selected by the previewer.
     *
     * @param rect The new scan area.
     **/
    void slotNewPreviewRect(const QRect &rect);

signals:
    /**
     * Indicates that the resolution setting has changed.
     *
     * @param xres The new X resolution.
     * @param yres The new Y resolution.
     *
     * @see slotNewResolution
     **/
    void scanResolutionChanged(int xres, int yres);

    /**
     * Indicates that the scan mode setting has changed.
     *
     * @param bytes_per_pix The new pixel depth.  This is the number
     * of bytes per pixel, or @c 0 for a black/white (1 bit per pixel)
     * image.
     *
     * @see slotNewScanMode
     **/
    void scanModeChanged(int bytes_per_pix);

    /**
     * The scan area or its size has changed, either by the user selecting
     * a preset scan area or by drawing an area on the previewer, or by
     * the previewer auto-selection.
     *
     * @param rect The new scan area.
     * @see slotScanSizeSelected
     **/
    void newCustomScanSize(const QRect &rect);

private slots:
    void slotScanProgress(int value);

private:

    enum ScanMode					// order fixed by GUI buttons
    {
        SaneDebugMode = 0,
        VirtualScannerMode = 1,
        NormalMode = 2
    };

    KScanDevice::Status prepareScan(QString *vfp);
    KScanDevice::Status performADFScan();

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
    KLed *mLed;

    AdfBehaviour adf;
    ScanParams::ScanMode mScanMode;
    ScanSizeSelector *area_sel;

    KScanOption *mResolutionBind;
    KScanOption *mSourceSelect;

    KScanOptSet *mStartupOptions;

    QPixmap pixLineArt, pixGray, pixColor, pixHalftone;
    bool mFirstGTEdit;

    QIcon mIconColor;
    QIcon mIconGray;
    QIcon mIconLineart;
    QIcon mIconHalftone;

    QLabel *mProblemMessage;
    QLabel *mNoScannerMessage;
};

#endif							// SCANPARAMS_H
