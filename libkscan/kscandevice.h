/* This file is part of the KDE Project
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

#ifndef _KSCANDEV_H_
#define _KSCANDEV_H_

#include <qwidget.h>
#include <qasciidict.h>
#include <qsize.h>
#include <qobject.h>
#include <qstrlist.h>
#include <qsocketnotifier.h>


#include "kscanoption.h"
#include "kscanoptset.h"

#define DEFAULT_OPTIONSET "saveSet"
#define SCANNER_DB_FILE "scannerrc"

class ImgScanInfo;

extern "C" {
#include <sane/sane.h>
#include <sane/saneopts.h>
}

/**
 *  This is KScanDevice, a class for accessing the SANE scanning functions under KDE
 *
 *  @short KScanDevice
 *  @author Klaas Freitag
 *  @version 0.1alpha
 *
 */

typedef enum {
  SSTAT_SILENT, SSTAT_IN_PROGRESS, SSTAT_NEXT_FRAME, SSTAT_STOP_NOW, STTAT_STOP_ADF_FINISHED
} SCANSTATUS;


/**
 *  KScanStat
 *  shows the status of a KScan operation
 **/


class KScanDevice : public QObject
{
    Q_OBJECT

    /* Hmmm - No Q_PROPS ? */
public:
    /**
     *  KScanDevice - the KDE Scanner Device
     *  constructor. Does not much.
     *
     *   @see openDevice
     *
     */

    KScanDevice( QObject *parent = 0 );

    /**
     *  Destructor
     *
     *  releases internal allocated memory.
     */
    ~KScanDevice( );

    /**
     *  opens the device named backend.
     *   @return the state of the operation
     *   @param backend: the name of the backend to open
     */
    KScanStat openDevice( const QCString& backend );

    /**
     *  returns the names of all existing Scan Devices in the system.
     *  @param enable_net: allow net devices.
     *  @return a QStrList of available Scanners in the system
     *  @see KScanDevice
     */
    QStrList getDevices( ) const
        { return( scanner_avail ); }

    /**
     * returns the short, technical name of the currently attached backend.
     * It is in the form 'umax:/dev/sg1'.
     */
    QCString shortScannerName() const { return scanner_name; }

    /**
     *  returns a long, human readable name of the scanner, like
     * 'Mustek SP1200 Flatbed scanner'. The parameter name takes
     * the name of a backend, what means something like "/dev/scanner".
     * If the name of the backend is skipped, the selected backend is
     * returned.
     *
     * @param a QString with a backend string
     * @return a QString containing a human readable scanner name
     **/
    QString getScannerName( const QCString& name = 0 ) const;

    /*
     *  ========= Preview Functions ==========
     */

    /**
     *  QImage * acquirePreview( bool forceGray = 0, int dpi = 0 )
     *
     *  acquires a preview on the selected device. KScanDevice allocs
     *  memory for the qimage returned. This qimage remains valid
     *  until the next call to acquirePreview or until KScanDevice is
     *  destructed.
     *
     *   @return a pointer to a qimage or null on error
     *	@param  bool forceGray if true, acquire a preview in gray

     *	@param  int dpi        dpi setting. If 0, the dpi setting is
     *	        elected by the object, e.g. is the smallest possible
     *           resolution.

    */
    KScanStat acquirePreview( bool forceGray = 0, int dpi = 0 );

    /** acquires a qimage on the given settings.
     *  the Parameter image needs to point to a object qimage,
     *  which is filled by the SANE-Dev. After the function returns,
     *  the qimage is valid, if the returnvalue is OK.
     *  @param QImage *image - Pointer to a reserved image
     *  @return the state of the operation in KScanStat
     */
    KScanStat acquire( const QString& filename = QString::null );

    /**
     *  returns the default filename of the preview image of this scanner.
     *	@return default filename
     **/
    const QString previewFile();

    /**
     *  loads the last saved previewed image on this device from the default file
     *	@return a bitmap as QImage
     **/
    QImage loadPreviewImage();

    /**
     *  saves current previewed image from this device to the default file
     *  @param image	current preview image which should be saved
     *	@return true if the saving was sucessful
     **/
    bool   savePreviewImage( const QImage& image );


    /*
     *  ========= List Functions ==========
     */

    /**
     *  returns all existing options of the selected device.
     *  @see openDevice
     *	@return a QStrList with the names of all options
     **/
    QStrList getAllOptions();

    /**
     *  returns the common options of the device. A frontend should
     *  display the returned options in a convinient way and easy to
     *  use.
     *  @see getAllOptions
     *  @see getAdvancedOptions
     *  @return a QStrList with with names of the common options.
     */
    QStrList getCommonOptions();

    /**
     *  returns a list of advanced scan options. A frontend should
     *  display these options in a special dialog.
     *  @see getAllOptions
     *  @see getCommonOptions
     *  @return a QStrList with names of advanced scan options.
     */
    QStrList getAdvancedOptions();

    /**
     * retrieves a set of the current scan options and writes them
     * to the object pointed to by parameter optSet
     * @see KScanOptSet
     * @param optSet, a pointer to the option set object
     */
    void getCurrentOptions( KScanOptSet *optSet );

    /**
     * load scanner parameter sets. All options stored in @param optSet
     * are loaded into the scan device.
     */
    void loadOptionSet( KScanOptSet *optSet );

    /**
     *  applys the values in an scan-option object. The value to
     *  set must be set in the KScanOption object prior. However,
     *  it takes effect after being applied with this function.
     *  @see KScanOption
     *  @return the status of the operation
     *  @param  a scan-option object with the scanner option to set and
     *  an optional boolean parameter if it is a gamma table.
     **/

    KScanStat apply( KScanOption *opt, bool=false );

    /**
     *  returns true if the option exists in that device.
     *  @param name: the name of a option from a returned option-List
     *  @return true, if the option exists
     */
    bool optionExists( const QCString& name );

    /**
     *  checks if the backend knows the option with the required name under
     *  a different name, like some backends do. If it does, the known name
     *  is returned, otherwise a null QCString.
     */

    QCString aliasName( const QCString& name );

    /**
     *  returns a Widget suitable for the selected Option and creates the
     *  KScanOption. It is internally connected to the scan device, every
     *  change to the widget is automaticly considered by the scan device.
     *  @param name: Name of the SANE Option
     *  @param parent: pointer to the parent widget
     *  @param desc: pointer to the text appearing as widget text
     *  @param tooltip: tooltip text. If zero, the SANE text will be used.
     **/
    KScanOption *getGuiElement( const QCString& name, QWidget *parent,
                                const QString& desc = QString::null,
                                const QString& tooltip = QString::null );

    /**
     *  returns the pointer to an already created Scanoption from the
     *  gui element list. Cares for option name aliases.
     */
    KScanOption *getExistingGuiElement( const QCString& name );

    /**
     *  sets an widget of the named option enabled/disabled
     **/
    void guiSetEnabled( const QCString& name, bool state );

    /**
     *  returns the maximum scan size. This is interesting e.g. for the
     *  preview widget etc.
     *  The unit of the return value is millimeter, if the sane backend
     *  returns millimeter. (TODO)
     **/
    QSize getMaxScanSize( void ) const;

    /**
     *  returns the information bit stored for the currently attached scanner
     *  identified by the key.
     */
    QString getConfig( const QString& key, const QString& def = QString() ) const;

    static bool        scanner_initialised;
    static SANE_Handle scanner_handle;
    static QAsciiDict<int>*  option_dic;
    static SANE_Device const **dev_list;
    static KScanOptSet *gammaTables;

public slots:
    void slOptChanged( KScanOption* );

    /**
     *  The setting of some options require a complete reload of all
     *  options, because they might have changed. In that case, slot
     *  slReloadAll() is called.
     **/
    void slReloadAll( );

    /**
     *  This slot does the same as ReloadAll, but it does not reload
     *  the scanner option given as parameter. This is useful to make
     *  sure that no recursion takes places during reload of options.
     **/
    void slReloadAllBut( KScanOption*);

    /**
     *   Notifies the scan device to stop the current scanning.
     *   This slot only makes sense, if a scan is in progress.
     *   Note that if the slot is called, it can take a few moments
     *   to stop the scanning
     */
    void slStopScanning( void );

    /**
     *  Image ready-slot in asynchronous scanning
     */
    void slScanFinished( KScanStat );

    /**
     * Save the current configuration parameter set. Only changed options are saved.
     **/
    void slSaveScanConfigSet( const QString&, const QString& );


    void slSetDirty( const QCString& name );

    /**
     * Closes the scan device and frees all related data, makes
     * the system ready to open a new, other scanner.
     */
    void slCloseDevice( );

    /**
     * stores the info bit in a config file for the currently connected
     * scanner. For this, the config file $KDEHOME/.kde/share/config/scannerrc
     * is opened, a group is created that identifies the scanner and the
     * device where it is connected. The information is stored into that group.
     */
    void slStoreConfig( const QString&, const QString& );

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
    void sigNewImage( QImage*, ImgScanInfo* );

    /**
     *  emitted if a new preview was acquired. The image has to be
     *  copied in the signal handler.
     */
    void sigNewPreview( QImage*, ImgScanInfo* );
    
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
    void sigScanProgress( int );

    /**
     *  emitted to show that a scan finished and provides a status how
     *  the scan finished.
     */
    void sigScanFinished( KScanStat );


    /**
     *  emitted to give all objects using this device to free all dependencies
     *  on the scan device, because this signal means, that the scan device is
     *  going to be closed. The can not be stopped. All receiving objects have
     *  to free their stuff using the device and pray. While handling the signal,
     *  the device is still open and valid.
     */
    void sigCloseDevice( );

private slots:
    /**
     *  Slot to acquire a whole image */
    void                doProcessABlock( void );


private:
    /**
     *  Function which applies all Options which need to be applied.
     *  See SANE-Documentation Table 4.5, description for SANE_CAP_SOFT_DETECT.
     *  The function sets the options which have SANE_CAP_AUTOMATIC set,
     *  to autmatic adjust.
     **/
    void                prepareScan( void );

    KScanStat           createNewImage( SANE_Parameters *p );

// not implemented
//   QWidget	      *entryField( QWidget *parent, const QString& text,
//                                    const QString& tooltip );
    KScanStat           find_options(); // help fct. to process options
    KScanStat           acquire_data( bool isPreview = false );
    QStrList 	      scanner_avail;  // list of names of all scan dev.
    QStrList	      option_list;    // list of names of all options
    QStrList            dirtyList;     // option changes

    inline  QString optionNotifyString(int) const;
    
    QPtrList<KScanOption>  gui_elements;
    QAsciiDict<SANE_Device>  scannerDevices;

    QSocketNotifier     *sn;

    SCANSTATUS          scanStatus;

    /* Data for the scan process */
    /* This could/should go to  a small help object */
    QCString             scanner_name;
    SANE_Byte           *data;
    QImage              *img;
    SANE_Parameters      sane_scan_param;
    long		      overall_bytes;
    int                 rest_bytes;
    int                 pixel_x, pixel_y;
    bool 		      scanningPreview;

    KScanOptSet         *storeOptions;

    class KScanDevicePrivate;
    KScanDevicePrivate *d;
};

#endif
