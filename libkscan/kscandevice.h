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

#include <qsize.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qbytearray.h>
#include <qimage.h>
#include <qlist.h>
#include <qhash.h>

#include "kscanoption.h"

extern "C" {
#include <sane/sane.h>
#include <sane/saneopts.h>
}


class QWidget;
class QSocketNotifier;

class KScanOptSet;
class ImgScanInfo;


/**
 *  This is KScanDevice, a class for accessing the SANE scanning functions under KDE
 *
 *  @short KScanDevice
 *  @author Klaas Freitag
 *  @version 0.1alpha
 *
 */

class KSCAN_EXPORT KScanDevice : public QObject
{
    Q_OBJECT

public:

    /**
     * Scanning/control status
     */
    enum Status
    {
        Ok,
        NoDevice,
        ParamError,
        OpenDevice,
        ControlError,
        EmptyPic,
        NoMemory,
        Reload,
        Cancelled,
        OptionNotActive
    };

    /**
     *  KScanDevice - the KDE Scanner Device
     *  constructor. Does not much.
     *
     *   @see openDevice
     *
     */
    explicit KScanDevice(QObject *parent = NULL);

    /**
     *  Destructor
     *
     *  releases internal allocated memory.
     */
    ~KScanDevice();

    /**
     *  Opens the device named @p backend.
     *   @return the state of the operation
     *   @param backend the name of the backend to open
     */
    KScanDevice::Status openDevice(const QByteArray &backend);

    /**
     *  Get an error message for the last SANE operation.
     *   @return the error message string
     */
    QString lastSaneErrorMessage() const;

    /**
     * Returns the short, technical name of the currently attached backend.
     * It is in the form 'umax:/dev/sg1'.
     */
    const QByteArray &scannerBackendName() const { return (mScannerName); }

    /**
     * Returns a long, human readable name of the currently attached
     * scanner backend, such as "Mustek SP1200 Flatbed scanner".
     **/
    QString scannerDescription() const;

    /**
     * Checks whether a scanner device is set and connected (i.e.
     * whether the previous openDevice() returned KScanDevice::Ok).
     **/
    bool isScannerConnected() const { return (mScannerInitialised); }

    /*
     *  ========= Preview/Scan Functions ==========
     */

    /**
     *  Acquires a preview on the selected device.  Allocates
     *  mScanImage as the QImage returned.  This image remains valid
     *  until the next call to acquirePreview() or acquire(), or until
     *  the KScanDevice is destructed.
     *
     *	@param forceGray if @c true, acquire a preview in gray
     *	@param dpi       DPI setting. If 0, the DPI setting is
     *	                 selected by the object, e.g. the lowest possible
     *                   resolution.
     *  @return the status of the operation
     */
    KScanDevice::Status acquirePreview(bool forceGray = false, int dpi = 0);

    /**
     *  Acquires a scan.  mScanImage is allocated as for acquirePreview().
     *
     *  @param filename File name for virtual scanner testing
     *  @return the status of the operation
     */
    KScanDevice::Status acquire(const QString &filename = QString::null);

    /**
     *  returns the default filename of the preview image of this scanner.
     *	@return default filename
     **/
    const QString previewFile();

    /**
     *  Loads the last saved previewed image on this device from the default file
     *	@return a bitmap as QImage
     **/
    QImage loadPreviewImage();

    /**
     *  Saves current previewed image from this device to the default file
     *  @param image current preview image which should be saved
     *	@return @c true if the saving was sucessful
     **/
    bool savePreviewImage(const QImage &image);


    /*
     *  ========= List Functions ==========
     */

    /**
     *  Returns all existing options of the selected device.
     *  @see openDevice
     *	@return a list with the names of all options
     **/
    QList<QByteArray> getAllOptions();

    /**
     *  Returns the common options of the device. A frontend should
     *  display the returned options in a convinient way and easy to
     *  use.
     *  @see getAllOptions
     *  @see getAdvancedOptions
     *  @return a list with with names of the common options
     */
    QList<QByteArray> getCommonOptions();

    /**
     *  Returns a list of advanced scan options. A frontend should
     *  display these options in a special dialog.
     *  @see getAllOptions
     *  @see getCommonOptions
     *  @return a list with names of advanced scan options
     */
    QList<QByteArray> getAdvancedOptions();

    /**
     * retrieves a set of the current scan options and writes them
     * to the object pointed to by parameter optSet
     * @see KScanOptSet
     * @param optSet the option set object
     */
    void getCurrentOptions(KScanOptSet *optSet);

    /**
     * load scanner parameter sets. All options stored in @param optSet
     * are loaded into the scan device.
     */
    void loadOptionSet(KScanOptSet *optSet);

    /**
     *  applys the values in an scan-option object. The value to
     *  set must be set in the KScanOption object prior. However,
     *  it takes effect after being applied with this function.
     *  @see KScanOption
     *  @return the status of the operation
     *  @param  a scan-option object with the scanner option to set and
     *  an optional boolean parameter if it is a gamma table.
     **/

    KScanDevice::Status apply(KScanOption *opt, bool isGammaTable = false);

    /**
     *  returns true if the option exists in that device.
     *  @param name: the name of a option from a returned option-List
     *  @return true, if the option exists
     */
    bool optionExists(const QByteArray &name);

    /**
     *  checks if the backend knows the option with the required name under
     *  a different name, like some backends do. If it does, the known name
     *  is returned, otherwise a null QByteArray.
     */
    QByteArray aliasName(const QByteArray &name);

    /**
     *  returns a Widget suitable for the selected Option and creates the
     *  KScanOption. It is internally connected to the scan device, every
     *  change to the widget is automaticly considered by the scan device.
     *  @param name: Name of the SANE Option
     *  @param parent: pointer to the parent widget
     *  @param desc: pointer to the text appearing as widget text
     *  @param tooltip: tooltip text. If zero, the SANE text will be used.
     **/
    KScanOption *getGuiElement(const QByteArray &name,
                               QWidget *parent,
                               const QString &desc = QString::null,
                               const QString &tooltip = QString::null);

    /**
     *  returns the pointer to an already created Scanoption from the
     *  gui element list. Cares for option name aliases.
     */
    KScanOption *getExistingGuiElement(const QByteArray &name);

    /**
     *  sets an widget of the named option enabled/disabled
     **/
    void guiSetEnabled(const QByteArray& name, bool state);

    /**
     *  returns the maximum scan size. This is interesting e.g. for the
     *  preview widget etc.
     *  The unit of the return value is millimeter, if the sane backend
     *  returns millimeter. (TODO)
     **/
    QSize getMaxScanSize() const;

    /**
     **/
    void getCurrentFormat(int *format, int *depth);

    /**
     *  returns the information bit stored for the currently attached scanner
     *  identified by the key.
     */
    QString getConfig(const QString &key, const QString &def = QString::null) const;

    /**
     * stores the info bit in a config file for the currently connected
     * scanner. For this, the config file $KDEHOME/.kde/share/config/scannerrc
     * is opened, a group is created that identifies the scanner and the
     * device where it is connected. The information is stored into that group.
     */
    void storeConfig(const QString &key, const QString &val);

    /**
     * Retrieve or prompt for a username/password to authenticate the scanner.
     */
    bool authenticate(QByteArray *retuser, QByteArray *retpass);

    /*
     * Return an error message for the specified scan result status.
     */
    static QString errorMessage(KScanDevice::Status stat);




// TODO: public global data!
// Used by KScanOption, so either (a) every KScanOption object needs to link back
// to its parent KScanDevice, or (b) KScanDevice needs to be a singleton class.
    typedef QHash<QByteArray, int> OptionDict;
    static OptionDict *option_dic;
    static KScanOptSet *gammaTables;
    static SANE_Handle gScannerHandle;



public slots:
    /**
     **/
    void slotOptChanged(KScanOption *opt);

    /**
     *  The setting of some options require a complete reload of all
     *  options, because they might have changed. In that case, slot
     *  slReloadAll() is called.
     **/
    void slotReloadAll();

    /**
     *  This slot does the same as ReloadAll, but it does not reload
     *  the scanner option given as parameter. This is useful to make
     *  sure that no recursion takes places during reload of options.
     **/
    void slotReloadAllBut(KScanOption *opt);

    /**
     *   Notifies the scan device to stop the current scanning.
     *   This slot only makes sense, if a scan is in progress.
     *   Note that if the slot is called, it can take a few moments
     *   to stop the scanning
     */
    void slotStopScanning();

    /**
     *  Image ready-slot in asynchronous scanning
     */
    void slotScanFinished(KScanDevice::Status status);

    /**
     * Save the current configuration parameter set. Only changed options are saved.
     **/
// TODO: parameters
    void slotSaveScanConfigSet(const QString&, const QString& );

    /**
     **/
    void slotSetDirty(const QByteArray &name);

    /**
     * Closes the scan device and frees all related data, makes
     * the system ready to open a new, other scanner.
     */
    void slotCloseDevice();

signals:
    /**
     *  emitted, if an option change was applied, which made other
     *  options changed. The application may connect a slot to this
     *  signal to see, if other options became active/inactive.
     **/
    void sigOptionsChanged();

    /**
     *  emitted, if an option change was applied, which made the
     *  scan parameters change. The parameters need to be reread
     *  by sane_get_parameters().
     **/
    void sigScanParamsChanged();

    /**
     *  emitted if a new image was acquired. The image has to be
     *  copied in the signal handler.
     */
    void sigNewImage(const QImage *img, const ImgScanInfo *info);

    /**
     *  emitted if a new preview was acquired. The image has to be
     *  copied in the signal handler.
     */
    void sigNewPreview(const QImage *img, const ImgScanInfo *info);
    
    /**
     * emitted to tell the application the start of scanning. That is
     * before the enquiry of the scanner starts. Between this signal
     * and @see sigScanProgress(0) can be some time consumed by a scanner
     * warming up etc.
     */
    void sigScanStart();

    /**
     * signal to indicate the start of acquireing data. It is equal
     * to @see sigScanProgress emitted with value 0 and thus it is sent
     * after sigScanStart.
     * The time between sigScanStart and sigAcquireStart differs very much
     * from scanner to scanner.
     */
    void sigAcquireStart();

    /**
     *  emitted to tell the application the progress of the scanning
     *  of one page. The final value is always 1000. The signal is
     *  emitted for zero to give the change to initialize the progress
     *  dialog.
     **/
// TODO: parameters
    void sigScanProgress( int );

    /**
     *  emitted to show that a scan finished and provides a status how
     *  the scan finished.
     */
// TODO: parameters
    void sigScanFinished( KScanDevice::Status );


    /**
     *  emitted to give all objects using this device to free all dependencies
     *  on the scan device, because this signal means, that the scan device is
     *  going to be closed. The can not be stopped. All receiving objects have
     *  to free their stuff using the device and pray. While handling the signal,
     *  the device is still open and valid.
     */
    void sigCloseDevice();


private slots:
    /**
     *  Slot to acquire a whole image
     */
    void doProcessABlock();


private:
    /**
     *  Function which applies all Options which need to be applied.
     *  See SANE-Documentation Table 4.5, description for SANE_CAP_SOFT_DETECT.
     *  The function sets the options which have SANE_CAP_AUTOMATIC set,
     *  to automatic adjust.
     **/
    void                prepareScan();

    KScanDevice::Status           createNewImage(const SANE_Parameters *p);

    KScanDevice::Status           find_options(); // help fct. to process options
    KScanDevice::Status           acquire_data(bool isPreview = false);

    /**
     * Clear any saved authentication for this scanner, to ensure that the
     * user is prompted next time.
     */
    void clearSavedAuth();


    enum ScanningState					// only used by KScanDevice
    {
        ScanIdle,
        ScanInProgress,
        ScanNextFrame,
        ScanStopNow,
        ScanStopAdfFinished
    };


    QList<QByteArray>	      option_list;    // list of names of all options
    QList<QByteArray>            dirtyList;     // option changes

    QList<KScanOption *>  gui_elements;

    QSocketNotifier *mSocketNotifier;

    SANE_Status mSaneStatus;
    KScanDevice::ScanningState mScanningState;

    /* Data for the scan process */
    QByteArray             mScannerName;
    SANE_Byte           *mScanBuf;
    QImage              *mScanImage;
    SANE_Parameters      sane_scan_param;
    long		      overall_bytes;
    int                 rest_bytes;
    int                 pixel_x, pixel_y;
    bool 		      scanningPreview;

    KScanOptSet         *storeOptions;

    bool mScannerInitialised;

    class KScanDevicePrivate;
    KScanDevicePrivate *d;
};


#endif							// KSCANDEVICE_H
