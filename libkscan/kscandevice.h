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

#ifndef KSCANDEVICE_H
#define KSCANDEVICE_H

#include "libkscanexport.h"

#include <qobject.h>
#include <qbytearray.h>
#include <qimage.h>
#include <qlist.h>
#include <qhash.h>
#include <qmap.h>

extern "C" {
#include <sane/sane.h>
}


class QWidget;
class QSocketNotifier;

class KScanOption;
class KScanOptSet;
class ImgScanInfo;


/**
 * @short Access and control a scanner.
 *
 * After constructing a @c KScanDevice, a scanner may be opened
 * via @c openDevice() by specifying its SANE backend name.  Having
 * done so, the scanner options supported by the scanner may be set
 * and read individually using a @c KScanOption.
 *
 * A scan or preview may be initiated, once the scan is finished the
 * scanned image data is made available via a signal.
 *
 * The scan device may be closed (using @c closeDevice()) and opened
 * again to change the scanner that is accessed.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class KSCAN_EXPORT KScanDevice : public QObject
{
    Q_OBJECT

public:

    /**
     * Scanning/control status.
     **/
    enum Status
    {
        Ok,
        NoDevice,
        ParamError,
        OpenDevice,
        ControlError,
        ScanError,
        EmptyPic,
        NoMemory,
        Reload,
        Cancelled,
        OptionNotActive,
        NotSupported
    };

    /**
     * Construct a new scanner device object.
     *
     * @param parent The parent object
     **/
    KScanDevice(QObject *parent);

    /**
     * Destructor.
     **/
    ~KScanDevice();

    /**
     * Open a scanner device.
     *
     * @param backend SANE name of the backend to open
     * @return The status of the operation
     *
     * @note This operation may prompt for SANE authentication if
     * it is required.
     **/
    KScanDevice::Status openDevice(const QByteArray &backend);

    /**
     * Close the scanner device and frees all related data.
     * After doing so, a new scanner may be opened.
     **/
    void closeDevice();

    /**
     * Get the device name of the current scanner backend.
     * It has the format, for example, "umax:/dev/sg1".
     *
     * @return The scanner backend name, or a null string if no scanner
     * is currently open.
     **/
    const QByteArray &scannerBackendName() const { return (mScannerName); }

    /**
     * Get a readable name/description of the current scanner
     * backend.  It may read, for example, "Mustek SP1200 Flatbed scanner".
     *
     * @return The scanner description.  If no scanner is currently open,
     * the (I18N'ed) string "No scanner connected" is returned.
     **/
    QString scannerDescription() const;

    /**
     * Check whether a scanner device is opened and connected: that is,
     * whether the @c openDevice() returned @c KScanDevice::Ok).
     **/
    bool isScannerConnected() const	{ return (mScannerInitialised); }

    /**
     * Get the SANE scanner handle of the current scanner.
     *
     * @return The scanner handle, or @c NULL if no scanner is
     * currently open.
     **/
    SANE_Handle scannerHandle() const	{ return (mScannerHandle); }

    /**
     * Acquires a preview from the current scanner device.
     * When the scan is complete, a result image will be sent by
     * @c sigNewPreview.
     *
     * @param forceGray If this is @c true, the preview is acquired
     * in greyscale regardless of any other settings.
     * @param dpi Resolution setting for the preview. If this is 0,
     * a suitable low resolution is selected.
     * @return The status of the operation
     **/
    KScanDevice::Status acquirePreview(bool forceGray = false, int dpi = 0);

    /**
     * Acquires a scan from the current scanner device.
     * When the scan is complete, a result image will be sent by
     * @c sigNewImage.
     *
     * @param filename File name to load, for virtual scanner test mode.
     * If this is a null or empty string, a real scan is performed.
     * @return The status of the operation
     **/
    KScanDevice::Status acquireScan(const QString &filename = QString::null);

    /**
     * Get the standard file name for saving the preview image for the
     * current scanner.
     *
     * @return Preview file name
     *
     * @note Saving the preview image this does not actually work unless a
     * subdirectory called @c previews exists within the calling application's
     * @c data resource directory.
     * @see loadPreviewImage
     * @see savePreviewImage
     * @see KStandardDirs
     **/
    const QString previewFile() const;

    /**
     * Load the last saved preview image for this device from the saved file.
     *
     * @return The preview image, or a null image if none could be loaded.
     *
     * @see previewFile
     * @see savePreviewImage
     **/
    QImage loadPreviewImage();

    /**
     * Saves a preview image for this device to the standard save file.
     *
     * @param image Preview image which is to be saved
     * @return @c true if the saving was successful
     *
     * @see previewFile
     * @see loadPreviewImage
     * @note For historical reasons, the image is saved in BMP format.
     **/
    bool savePreviewImage(const QImage &image) const;

    /**
     * Get the names of all of the parameters supported by the
     * current scanner device.
     *
     * @return A list of all known parameter names, in order of their SANE
     * index.
     * @see getCommonOptions
     * @see getAdvancedOptions
     **/
    QList<QByteArray> getAllOptions() const;

    /**
     * Get the common options of the device. An application will normally
     * display these options in a convenient and easy to access way.
     *
     * @return A list of the names of the common options.
     * @see getAllOptions
     * @see getAdvancedOptions
     **/
    QList<QByteArray> getCommonOptions() const;

    /**
     * Get the advanced options of the device. An application may hide
     * these options away in a less obvious place.
     *
     * @return A list of the names of the advanced options.
     * @see getAllOptions
     * @see getCommonOptions
     **/
    QList<QByteArray> getAdvancedOptions() const;

    /**
     * Retrieve the currently active parameters from the scanner.
     * This includes all of those that have an associated GUI element,
     * and also any others (e.g. the TL_X and 3 other scan area settings)
     * that have been @c apply()'ed but do not have a GUI.
     *
     * @param optSet An option set which will receive the options
     **/
    void getCurrentOptions(KScanOptSet *optSet) const;

    /**
     * Load a saved parameter set. All options that exist in the set
     * and which the current scanner supports will be @c set()
     * with the values from the @c KScanOptSet and @c apply()'ed.
     *
     * @param optSet An option set with the options to be loaded
     **/
    void loadOptionSet(const KScanOptSet *optSet);

    /**
     * Check whether the current scanner device supports an option.
     *
     * @param name The name of a scanner parameter
     * @return @c true if the option is known by this scanner
     **/
    bool optionExists(const QByteArray &name) const;

    /**
     * Resolve a backend-specific option name into a generally known
     * SANE option name.  See the source code for those names supported.
     *
     * @param name The backend-specific parameter name
     * @return The generally known SANE name, or the input @p name
     * unchanged if it has no known alias.
     **/
    QByteArray aliasName(const QByteArray &name) const;

    /**
     * Create a @c KScanOption and GUI widget suitable for the specified
     * scanner parameter.
     *
     * @param name Name of the SANE parameter.
     * @param parent Parent for widget.
     * @param descr Text description for the GUI label, if this is not
     * specified or empty then the parameter's SANE @c title is used.
     * @return A @c KScanOption for the parameter, or @c NULL if none
     * could be created.
     *
     * @see KScanOption::createWidget
     * @see KScanOption::widget
     * @see getExistingGuiElement
     **/
    KScanOption *getGuiElement(const QByteArray &name,
                               QWidget *parent,
                               const QString &descr = QString::null);

    /**
     * Find the @c KScanOption for an existing scanner parameter.
     *
     * @param name Name of the SANE parameter.
     * @return The @c KScanOption for the parameter, or @c NULL if the
     * parameter does not exist or it has not been created yet.
     **/
    KScanOption *getExistingGuiElement(const QByteArray &name) const;

    /**
     * Create or retrieve a @c KScanOption for the specified scanner
     * parameter.
     *
     * @param name Name of the SANE parameter.
     * @param create If the option does not yet exist, then create it if
     * this is @c true (the default).  If this is @c false, then do not
     * create the option if it does not already exist but return NULL
     * instead.
     * @return The existing or created option, or @c NULL if @p create
     * is @c false and the option does not currently exist.
     * @note This is the only means for a calling library or application
     * to create or obtain a @c KScanOption.
     * @note The returned pointer is valid until the scanner device is
     * closed or the @c KScanDevice object is destroyed.
     **/
    KScanOption *getOption(const QByteArray &name, bool create = true);

    /**
     * Set the enabled/disabled state of the GUI widget for an option.
     *
     * @param name Name of the SANE parameter.
     * @param state @c true to enable the widget, @c false to disable it.
     *
     * @note If the option is not software settable, it will be disabled
     * regardless of the @p state parameter.
     * @see KScanOption::isSoftwareSettable
     **/
    void guiSetEnabled(const QByteArray &name, bool state);

    /**
     * Get the maximum scan area size.  This can be used, for example, to
     * set the size of a preview widget.
     *
     * @return The width and height of the scan area, in SANE units
     * as returned by the backend.
     * @note Currently it is assumed that these units are always
     * millimetres, although according to the SANE documentation it is
     * possible that the unit may be pixels instead.
     **/
    QSize getMaxScanSize();

    /**
     * Get the current scan image format and bit depth.
     *
     * @param format An integer to receive the format, this is a
     * @c SANE_Frame value.
     * @param depth An integer to receive the bit depth (the number of bits
     * per sample).
     **/
    void getCurrentFormat(int *format, int *depth);

    /**
     * Read a configuration setting for the current scanner
     * from the global scanner configuration file.
     *
     * @param key Configuration key
     * @param def Default value
     * @return The configuration setting, or the @p def parameter if
     * no @p key is saved in the configuration.
     *
     * @see storeConfig
     **/
    QString getConfig(const QString &key, const QString &def = QString::null) const;

    /**
     * Save a configuration setting for the current scanner
     * to the global scanner configuration file.
     *
     * @param key Configuration key
     * @param val Value to store
     *
     * @see getConfig
     **/
    void storeConfig(const QString &key, const QString &val);

    /**
     * Retrieve or prompt for a username/password to authenticate access
     * to the scanner.  If there is a saved username/password pair for
     * the current scanner, these will be returned.  Otherwise,
     * the user is prompted to enter a username/password (via a
     * @c KPasswordDialog) and the entries are saved in the global
     * scanner configuration file.
     *
     * @param retuser String to receive the user name
     * @param retpass String to receive the password
     * @return @c true if a saved username/password was available or
     * the user entered them, @c false if no saved username/password
     * was available and the user cancelled the password dialogue.
     *
     * @note This will normally be called by @c KScanGlobal::authCallback()
     * when a SANE operation requires authentication.
     **/
    bool authenticate(QByteArray *retuser, QByteArray *retpass);

    /**
     * Get an error message for the last SANE operation, if it failed.
     *
     * @return the error message string
     **/
    QString lastSaneErrorMessage() const;

    /**
     * Get an error message for the specified scan result status.
     *
     * @return the error message string
     **/
    static QString statusMessage(KScanDevice::Status stat);

    /**
     * Find the SANE index of an option.
     *
     * @param name Name of the SANE parameter.
     * @return Its option index, or 0 if tyhe option is not known by the scanner.
     **/
    int getOptionIndex(const QByteArray &name) const;

    /**
     * Update the scanner with the value of the specified option (using
     * @c KScanOption::apply()), then reload and update the GUI widget
     * of all of the other options.  Their value or state may have
     * changed due to the change in this option.
     *
     * @param opt The option to @c apply() but to not @c reload().  If
     * this is @c NULL, this function is equivalent to @c reloadAll().
     *
     * @note This does the same as @c reloadAll(), but it does not
     * reload the scanner option given as the @p opt parameter. This is
     * useful to ensure that no recursion takes place while reloading
     * the options.
     * @see reloadAll
     **/
    void reloadAllBut(KScanOption *opt);

    /**
     * Reload all of the scanner options and update their GUI widgets.
     *
     * @see reloadAllBut
     **/
    void reloadAll();

public slots:
    /**
     * Request the scan device to stop the scan currently in progress.
     * The scan may continue until the current data block has been
     * read and processed.
     **/
    void slotStopScanning();

signals:
    /**
     * Emitted to indicate that a scan is about to start.
     * Depending on the scanner, there may be a delay (for example,
     * while the lamp warms up) between this signal and the
     * @c sigAcquireStart.
     * @see sigAcquireStart
     **/
    void sigScanStart();

    /**
     * Emitted to indicate that a scan is starting to acquire data.
     * The first @c sigScanProgress will be emitted with progress value 0
     * immediately after this signal.
     * @see sigScanStart
     **/
    void sigAcquireStart();

    /**
     * Emitted to indicate the scanning progress. The first value sent
     * is always 0 and the final value is always 100. If the scan results
     * in multiple frames (for a 3-pass scanner) the sequence will
     * repeat.
     *
     * @param progress The scan progress as a percentage
     **/
    void sigScanProgress(int progress);

    /**
     * Emitted when a new scan image has been acquired.
     * The receiver should take a deep copy of the image, as it will
     * be destroyed after this signal has been delivered.
     *
     * @param img The acquired scan image
     * @param info Additional information for the image
     * @see sigNewPreview
     **/
    void sigNewImage(const QImage *img, const ImgScanInfo *info);

    /**
     * Emitted when a new preview image has been acquired.
     * The receiver should take a deep copy of the image, as it will
     * be destroyed after this signal has been delivered.
     *
     * @param img The acquired preview image
     * @param info Additional information for the image
     * @see sigNewImage
     **/
    void sigNewPreview(const QImage *img, const ImgScanInfo *info);
    
    /**
     * Emitted to indicate that a scan or preview has finished.
     * This signal is always emitted, even if the scan failed with
     * an error or was cancelled, and before the @c sigNewImage or
     * the @c sigNewPreview.
     *
     * @param stat Status of the scan
     **/
    void sigScanFinished(KScanDevice::Status stat);

    /**
     * Emitted when the device is about to be closed by @c closeDevice().
     * This gives any callers using this device a chance to give up any
     * dependencies on it.  While this signal is being emitted, the
     * scanner device is still open and valid.
     **/
    void sigCloseDevice();

private slots:
    void doProcessABlock();
    void slotScanFinished(KScanDevice::Status status);

private:
    KScanDevice::Status findOptions();
    void showOptions();

    KScanDevice::Status createNewImage(const SANE_Parameters *p);

    KScanDevice::Status acquireData(bool isPreview = false);

    /**
     * Clear any saved authentication for this scanner, to ensure that the
     * user is prompted again next time.
     **/
    void clearSavedAuth();

    /**
     * Save the current option parameter set.
     * Only active and GUI options are saved.
     **/
    void saveStartupConfig();

    /**
     * Scanning progress state.
     **/
    enum ScanningState					// only used by KScanDevice
    {
        ScanIdle,
        ScanStarting,
        ScanInProgress,
        ScanNextFrame,
        ScanStopNow,
        ScanStopAdfFinished
    };

    typedef QHash<QByteArray,KScanOption *> OptionHash;
    typedef QMap<int,QByteArray> IndexMap;

    OptionHash mCreatedOptions;				// option name -> KScanOption
    IndexMap mKnownOptions;				// SANE index -> option name

    KScanOptSet *mSavedOptions;

    QByteArray mScannerName;
    bool mScannerInitialised;
    SANE_Handle mScannerHandle;

    KScanDevice::ScanningState mScanningState;
    SANE_Status mSaneStatus;

    SANE_Byte *mScanBuf;
    QImage *mScanImage;
    SANE_Parameters mSaneParameters;
    long mBytesRead;
    long mBlocksRead;
    int mBytesUsed;
    int mPixelX, mPixelY;
    bool mScanningPreview;
    QSocketNotifier *mSocketNotifier;

    int mCurrScanResolutionX;
    int mCurrScanResolutionY;
};


#endif							// KSCANDEVICE_H
