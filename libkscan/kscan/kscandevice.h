/** HEADER **/

#ifndef _KSCANDEV_H_
#define _KSCANDEV_H_

#include <qwidget.h>
#include <qdict.h>
#include <qwidget.h>
#include <qobject.h>
#include <qstrlist.h>
#include <qsocketnotifier.h>

extern "C" {
#include <sane.h>
#include <saneopts.h>
}


typedef enum { KSCAN_OK, KSCAN_ERROR, KSCAN_ERR_NO_DEVICE,
	       KSCAN_ERR_BLOCKED, KSCAN_ERR_NO_DOC, KSCAN_ERR_PARAM,
               KSCAN_ERR_OPEN_DEV, KSCAN_ERR_CONTROL, KSCAN_ERR_EMPTY_PIC,
               KSCAN_ERR_MEMORY, KSCAN_ERR_SCAN, KSCAN_UNSUPPORTED,
               KSCAN_RELOAD, KSCAN_CANCELLED }
KScanStat;

typedef enum { INVALID_TYPE, BOOL, SINGLE_VAL, RANGE, GAMMA_TABLE, STR_LIST, STRING }
KScan_Type;


class KGammaTable;


/**
 *  This is KScanDevice, a class for accessing the SANE scanning functions
 *  under Linux.
 *
 *  @short KScanDevice
 *  @author Klaas Freitag
 *  @version 0.1alpha
 *
 */

/**
 *  KScan_Option
 *
 *  is a help class for accessing the scanner options.
 **/

typedef enum {
  SSTAT_SILENT, SSTAT_IN_PROGRESS, SSTAT_NEXT_FRAME, SSTAT_STOP_NOW, STTAT_STOP_ADF_FINISHED
} SCANSTATUS;

class KScanOption:public QObject
{
  Q_OBJECT
public:
  KScanOption( const char *new_name );
  KScanOption( const KScanOption& so );
  ~KScanOption();

  bool initialised( void ){ return( ! buffer_untouched );};
  bool valid( void );
  bool autoSetable( void );
  bool active( void );
  bool softwareSetable();
  KScan_Type type( void );

  bool set( int val );
  bool set( double val );
  bool set( int *p_val, int size );
  bool set( QString );
  bool set( bool b ){ if( b ) return(set( (int)(1==1) )); else return( set( (int) (1==0) )); }
  bool set( KGammaTable  *gt );
  bool set( const char* );
	
  bool get( int* );
  bool get( KGammaTable* );
  const QString get( void );

  QWidget *createWidget( QWidget *parent, const char *w_desc=0,
			 const char *tooltip=0  );

  /* Operators */
  const KScanOption& operator= (const KScanOption& so );

  // Possible Values
  QStrList   getList();
  bool       getRange( double*, double*, double* );

  QString    getName() { return( name ); }
  void *     getBuffer(){ return( buffer ); }
  QWidget   *widget( ) { return( internal_widget ); }
  /**
   *  returns the type of the selected option.
   *  This might be SINGLE_VAL, VAL_LIST, STR_LIST, GAMMA_TABLE,
   *  RANGE or BOOL
   *
   *  You may use the information returned to decide, in which way
   *  the option is to set.
   *
   *  A SINGLE_VAL is returned in case the value is represented by a
   *  single number, e.g. the resoltion.
   *
   *  A VAL_LIST is returned in case the value needs to be set by
   *  a list of numbers. You need to check the size to find out,
   *  how many numbers you have to
   *  @param name: the name of a option from a returned option-List
   *  return a option type.
   **/
  KScan_Type typeToSet( const char* name );

  /**
   *  returns a string describing the unit of given the option.
   *  @return the unit description, e.g. mm
   *  @param name: the name of a option from a returned option-List
   **/
  QString unitOf( const char *name );

public slots:
void       slRedrawWidget( KScanOption *so );
  /**
   *	 that slot makes the option to reload the buffer from the scanner.
   */
  void       slReload( void );

protected slots:
/**
 *  this slot is called if an option has a gui element (not all have)
 *  and if the value of the widget is changed.
 *  This is an internal slot.
 **/
void		  slWidgetChange( void );
  void		  slWidgetChange( const char* );
  void		  slWidgetChange( int );
	
  signals:
  /**
   *  Signal emitted if a option changed for different reasons.
   *  The signal should be connected by outside objects.
   **/
  void      optionChanged( KScanOption*);
  /**
   *  Signal emitted if the option is set by a call to the set()
   *  member of the option. In the slot needs to be checked, if
   *  a widget exists, it needs to be set to the new value.
   *  This is a more internal signal
   **/
  void      optionSet( void );

  /**
   *  Signal called when user changes a gui - element
   */
  void      guiChange( KScanOption* );

private:
  void       initOption( const char *new_name );


  QWidget    *entryField ( QWidget *parent, const char *text );
  QWidget    *sliderWidg( QWidget *parent, const char *text );
  QWidget    *comboBox   ( QWidget *parent, const char *text );
	
  const      SANE_Option_Descriptor *desc;
  QString    name;
  void       *buffer;
  QWidget    *internal_widget;
  bool       buffer_untouched;
  size_t     buffer_size;

  /* For gamma-Tables remeber gamma, brightness, contrast */
  int        gamma, brightness, contrast;
};

/**
 *  KScanStat
 *  shows the status of a KScan operation
 **/


class KScanDevice : public QObject
{
  Q_OBJECT
public:
  /**
   *  KScanDevice - the KDE Scanner Device
   *
   *  @param backend Name of the backend to open initially.
   *
   *  the parameter backend may be zero. In that case, no backend is opend
   *  until a call to openDevice happens.
   *
   *  If backend is given, the device will be opened and the options will
   *  be read.
   *	@see openDevice
   *
   */

  KScanDevice( QObject *parent = 0 );

  /**
   *  Destructor
   *
   *  releases internal allocated memory.
   */
  ~KScanDevice( ){ };

  /**
   *  opens the device named backend.
   *   @return the state of the operation
   *	@param backend: the name of the backend to open
   */
  KScanStat openDevice( const char* backend );

  /**
   *  returns the names of all existing Scan Devices in the system.
   *  @param enable_net: allow net devices.
   *  @return a QStrList of available Scanners in the system
   *  @see KScanDevice
   */
  QStrList getDevices( )
  { return( scanner_avail ); }

  QString getScannerName( void );
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
  KScanStat acquire( QString filename = 0);

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
   *  applys the values in an scan-option object. The value to
   *  set must be set in the KScanOption object prior. However,
   *  it takes effect after being applied with this function.
   *  @see KScanOption
   *  @return the status of the operation
   *  @param  a scan-option object with the value to set.
   **/

  KScanStat apply( KScanOption *opt );

  /**
   *  returns true if the option exists in that device.
   *  @param name: the name of a option from a returned option-List
   *  @return true, if the option exists
   */
  bool optionExists( const char* name );

  /**
   *  returns a Widget suitable for the selected Option and creates the
   *  KScanOption. It is internally connected to the scan device, every
   *  change to the widget is automaticly considered by the scan device.
   *  @param name: Name of the SANE Option
   *  @param parent: pointer to the parent widget
   *  @param desc: pointer to the text appearing as widget text
   *  @param tooltip: tooltip text. If zero, the SANE text will be used.
   **/
  KScanOption *getGuiElement( const char *name, QWidget *parent,
			      const char *desc =0, const char *tooltip=0 );

  /**
   *  sets an widget of the named option enabled/disable
   **/
  bool guiSetEnabled( QString name, bool state );
	
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
   *  the scanner option given as parameter. This is usefull to make
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
  void sigNewImage( QImage* );
	
  /**
   *  emitted if a new preview was acquired. The image has to be
   *  copied in the signal handler.
   */
  void sigNewPreview( QImage*);

  /**
   *  emitted to tell the application the progress of the scanning
   *  of one page. The final value is always 1000. The signal is
   *  emitted for zero to give the change to initialize the progress
   *  dialog.
   **/
  void sigScanProgress( int );	


  void sigScanFinished( KScanStat );

private slots:
  /**
   *  Slot to acquire a whole image */
  void                doProcessABlock( void );

  /**
   *  Image ready-slot in asynchronous scanning */
  void                slScanFinished( KScanStat );

private:
  /**
   *  Function which applies all Options which need to be applied.
   *  See SANE-Documentation Table 4.5, description for SANE_CAP_SOFT_DETECT.
   *  The function sets the options which have SANE_CAP_AUTOMATIC set,
   *  to autmatic adjust.
   **/
  void                prepareScan( void );

  KScanStat           createNewImage( SANE_Parameters *p );


  QWidget	      *entryField( QWidget *parent, const char *text,
                                   const char *tooltip );
  KScanStat           find_options(); // help fct. to process options
  KScanStat           acquire_data( bool isPreview = false );
  QStrList 	      scanner_avail;  // list of names of all scan dev.
  QStrList	      option_list;    // list of names of all options

  QList<KScanOption>  gui_elements;
  QDict<KScanOption>  gui_elem_names;

  QDict<SANE_Device>  scannerDevices;

  QSocketNotifier     *sn;

  SCANSTATUS          scanStatus;

  /* Data for the scan process */
  /* This could/should go to  a small help object */
  QString             scanner_name;
  SANE_Byte           *data;
  QImage              *img;
  SANE_Parameters      sane_scan_param;
  long		      overall_bytes;
  int                 rest_bytes;
  int                 pixel_x, pixel_y;
  bool 		      scanningPreview;
};

#endif
