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

#ifndef SCANPARAMS_H
#define SCANPARAMS_H

#include "kscandevice.h"
#include "scansourcedialog.h"

#include <qvbox.h>
#include <qhbox.h>

#include <qdir.h>
#include <qpixmap.h>

/**
  *@author Klaas Freitag
  */

class GammaDialog;
class KScanOptSet;
class QScrollView;
class KLed;
class QProgressDialog;
class QPushButton;
class QCheckBox;
class QButtonGroup;

typedef enum { ID_SANE_DEBUG, ID_QT_IMGIO, ID_SCAN } ScanMode;

class ScanParams : public QVBox
{
   Q_OBJECT
public:
   ScanParams( QWidget *parent, const char *name = 0);
   ~ScanParams();
#if 0
   QSize sizeHint( );
#endif
   bool connectDevice( KScanDevice* );

   KLed *operationLED() { return m_led; }

public slots:
/**
 * In this slot, a custom scan window can be set, e.g. through a preview
 * image with a area selector. The QRect-param needs to contain values
 * between 0 and 1000, which are interpreted as tenth of percent of the
 * whole image dimensions.
 **/
void slCustomScanSize( QRect );

   /**
    * sets the scan area to the default, which is the whole area.
    */
   void slMaximalScanSize( void );

   /**
    * starts acquireing a preview image.
    * This ends up in a preview-signal of the scan-device object
    */
   void slAcquirePreview( void );
   void slStartScan( void );

   /**
    * connect this slot to KScanOptions Signal optionChanged to be informed
    * on a options change.
    */
   void slOptionNotify( KScanOption *kso );

protected slots:
/**
 * connected to the button which opens the source selection dialog
 */
void slSourceSelect( void );
   /**
    * allows to select a file or directory for the virtuell scanner
    */
   void slFileSelect	( void );

   /**
    *  Slot to call if the virtual scanner mode is changed
    */
   void slVirtScanModeSelect( int id );

   /**
    *  Slot for result on an edit-Custom Gamma Table request.
    *  Starts a dialog.
    */
   void slEditCustGamma( void );

   /**
    *  Slot called if a Gui-Option changed due to user action, eg. the
    *  user selects another entry in a List.
    *  Action to do is to apply the new value and check, if it affects others.
    */
   void slReloadAllGui( KScanOption* );

   /**
    *  Slot called when the Edit Custom Gamma-Dialog has a new gamma table
    *  to apply. This is an internal slot.
    */
   void slApplyGamma( KGammaTable* );

   /**
    *  internal slot called when the slider for x resolution changes.
    *  In the slot, the signal scanResolutionChanged will be emitted, which
    *  is visible outside the scanparam-object to notify that the resolutions
    *  changed.
    *
    *  That is e.g. useful for size calculations
    */
   void slNewXResolution( KScanOption* );

   /**
    *  the same slot as @see slNewXResolution but for y resolution changes.
    */
   void slNewYResolution( KScanOption* );


   signals:

   /**
    *  emitted if the resolution to scan changes. This signal may be connected
    *  to slots calculating the size of the image size etc.
    *
    *  As parameters the resolutions in x- and y-direction are coming.
    */
   void scanResolutionChanged( int, int );

private:


   QScrollView*  scannerParams( );
   void          virtualScannerParams( void );
   void          createNoScannerMsg( void );
   void          initialise( KScanOption* );
   void          setEditCustomGammaTableState();

   KScanStat     performADFScan( void );

   KScanDevice      *sane_device;
   KScanOption   *virt_filename;
   QCheckBox     *cb_gray_preview;
   QPushButton   *pb_edit_gtable;
   QPushButton   *pb_source_sel;
   ADF_BEHAVE	  adf;
   QButtonGroup  *bg_virt_scan_mode;
   ScanMode  	  scan_mode;
   QDir          last_virt_scan_path;

   KScanOption   *xy_resolution_bind;

   KScanOptSet   *startupOptset;

   QProgressDialog *progressDialog;

   QPixmap       pixLineArt, pixGray, pixColor, pixHalftone, pixMiniFloppy;
   KLed          *m_led;
   bool          m_firstGTEdit;

   class ScanParamsPrivate;
   ScanParamsPrivate *d;
};

#endif
