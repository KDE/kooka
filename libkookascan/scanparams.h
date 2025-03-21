/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#ifndef SCANPARAMS_H
#define SCANPARAMS_H

#include "kookascan_export.h"

#include <qwidget.h>

#include "kscandevice.h"
//#include "scansourcedialog.h"

class QProgressDialog;
class QPushButton;
class QLabel;
class QTabWidget;

class KScanOption;
class KGammaTable;
class KLed;
class KMessageWidget;
class KLocalizedString;

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
 * The scanner name/description and the operation LED appear above the
 * scanner settings, and the "Preview" and "Scan" action buttons below.
 *
 * This class controls the scanner operation (via its @c KScanDevice) and
 * the popup scanning progress dialogue.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class KOOKASCAN_EXPORT ScanParams : public QWidget
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
    explicit ScanParams(QWidget *parent);

    /**
     * Destructor.
     **/
    virtual ~ScanParams();

    /**
     * Specify a scanner device and create the GUI for its settings.
     *
     * @param newScanDevice The scan device to use, or @c nullptr if no
     * scanner device is to be used.
     * @param galleryMode If this is @c true, then a @c nullptr value for
     * the @p newScanDevice will be allowed and a message will be
     * displayed in place of the scanner settings.  This message will
     * be as created by @c messageScannerNotSelected().  If this
     * parameter is @c false and the @p newScanDevice parameter is
     * @c nullptr, the message as created by @c messageScannerProblem()
     * will be displayed.
     * @return @c true in all cases.
     **/
    bool connectDevice(KScanDevice *newScanDevice, bool galleryMode = false);

    /**
     * Get the operation "LED" widget.
     *
     * @return The widget, or @c nullptr if it has not been created yet.
     * Do not delete this, it is owned by the @c ScanParams object.
     **/
    KLed *operationLED() const;

    /**
     * Set the scan destination to be displayed in the progress dialogue.
     *
     * @param dest Destination to be shown
     **/
    void setScanDestination(const KLocalizedString &dest);

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

    /**
     * A new scan area has been drawn on or auto-selected by the previewer.
     *
     * @param rect The new scan area.
     **/
    void slotNewPreviewRect(const QRect &rect);

protected:
    /**
     * Create a widget (for example, a QLabel showing an appropriate message)
     * which is displayed if no scanner is selected.  This happens if
     * the @c newScanDevice parameter to @c connectDevice() is @c nullptr and
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
     * parameter to @c connectDevice() is @c nullptr but the @c galleryMode
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

    /**
     * Create a GUI to display the application's "Scan Destination"
     * selector and further controls as appropriate.
     *
     * @param frame the page to add the widgets to
     *
     * @note Any widgets created should be children of the @p frame page,
     * so they will be deleted along with the @c ScanParams object.
     * @see @c ScanParamsPage
     **/
    virtual void createScanDestinationGUI(ScanParamsPage *frame)	{ }

    /**
     * Check whether a scan device is present and configured.
     *
     * @return @c true if there is a scan device.
     **/
    bool hasScanDevice() const					{ return (mSaneDevice!=nullptr); }

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

    enum ScanMode {                 // order fixed by GUI buttons
        SaneDebugMode = 0,
        VirtualScannerMode = 1,
        NormalMode = 2
    };

    KScanDevice::Status prepareScan(QString *vfp);
    KScanDevice::Status performADFScan();

    void createNoScannerMsg(bool galleryMode);
    void initStartupArea(bool dontRestore);
    void setEditCustomGammaTableState();

    QWidget *createScannerParams();
    ScanParamsPage *createTab(QTabWidget *tw, const QString &title, const char *name = nullptr);

    QRect applyRect(const QRect &rect);
    void setMaximalScanSize();

    bool getGammaTableFrom(const QByteArray &opt, KGammaTable *gt);
    bool setGammaTableTo(const QByteArray &opt, const KGammaTable *gt);

    KScanDevice *mSaneDevice;
    KScanOption *mVirtualFile;

    QPushButton *mGammaEditButt;
    QProgressDialog *mProgressDialog;
    KLed *mLed;

//    AdfBehaviour adf;
    ScanParams::ScanMode mScanMode;
    ScanSizeSelector *mAreaSelect;

    KScanOption *mResolutionBind;
    KScanOption *mSourceSelect;

    KMessageWidget *mProblemMessage;
    KMessageWidget *mNoScannerMessage;
};

#endif                          // SCANPARAMS_H
