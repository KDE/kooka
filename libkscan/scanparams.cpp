/***************************************************************************
                          scanparams.cpp  -  description
                             -------------------
    begin                : Fri Dec 17 1999
    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <sane/saneopts.h>
#include <qcstring.h>
#include <qfile.h>
#include <qframe.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qimage.h>
#include <qtooltip.h>
#include <qmessagebox.h>
#include <qlayout.h>
#include <qdict.h>
#include <qfile.h>
#include <qprogressdialog.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kbuttonbox.h>
#include <kiconloader.h>
#include <kseparator.h>

#include "scanparams.h"
#include <scansourcedialog.h>
#include "massscandialog.h"
#include <gammadialog.h>
#include "kscanslider.h"




ScanParams::ScanParams( QWidget *parent, const char *name )
    : QVBox( parent, name ),
      progressDialog( 0L )
{
   /* first some initialisation and debug messages */
    sane_device = 0; virt_filename = 0;
    pb_edit_gtable = 0;
    cb_gray_preview = 0;
    pb_source_sel = 0;
    bg_virt_scan_mode = 0;
    xy_resolution_bind = 0;

    /* Preload icons */
    pixMiniFloppy = SmallIcon( "3floppy_unmount" );

    pixColor = SmallIcon( "palette_color" );
    pixGray  = SmallIcon( "palette_gray" );
    pixLineArt = SmallIcon( "palette_lineart" );
    pixHalftone = SmallIcon( "palette_halftone" );

    
}

bool ScanParams::connectDevice( KScanDevice *newScanDevice )
{
   setMargin( KDialog::marginHint() );
   setSpacing( KDialog::spacingHint() );
   /* Debug: dump common Options */

   if( ! newScanDevice )
   {
      kdDebug(29000) << "No scan device found !" << endl;
      sane_device = 0L;
      createNoScannerMsg();
      return( true );
   }
   sane_device = newScanDevice;

   QStrList strl = sane_device->getCommonOptions();
   QString emp;
   for ( emp=strl.first(); strl.current(); emp=strl.next() )
      kdDebug(29000) << "Common: " << strl.current() << endl;

   /* Start path for virual scanner */
   last_virt_scan_path = QDir::home();
   adf = ADF_OFF;

   /* Frame stuff for toplevel of scanparams - beautification */
   setFrameStyle( QFrame::Panel | QFrame::Raised );
   setLineWidth( 1 );


   /* initialise own widgets */
   cb_gray_preview = 0;

   /* A top layout box */
   // QVBoxLayout *top = new QVBoxLayout(this, 6);
   QString cap = i18n("<B>Scanner Settings</B><BR>");
   cap += sane_device->getScannerName();

   (void) new QLabel( cap, this );

   KSeparator* niceline = new KSeparator( KSeparator::HLine, this);

   /* Now create Widgets for the important scan settings */
   if( sane_device->optionExists( SANE_NAME_FILE ) )
   {
      /* Its a virtual scanner */
      scan_mode = ID_SANE_DEBUG;
      virtualScannerParams( );

   } else {
      scan_mode = ID_SCAN;
      scannerParams( );
   }
   /* Reload all options to care for inactive options */
   sane_device->slReloadAll();

   /* Create a Start-Scan-Button */
   KButtonBox *kbb = new KButtonBox( this );
   QPushButton* pb = kbb->addButton( i18n( "&Final Scan" ));
   connect( pb, SIGNAL(clicked()), this, SLOT(slStartScan()) );
   pb = kbb->addButton( i18n( "&Preview Scan" ));
   connect( pb, SIGNAL(clicked()), this, SLOT(slAcquirePreview()) );
   kbb->layout();


   /* Initialise the progress dialog */
   progressDialog = new QProgressDialog( i18n("Scanning in progress"),
					 i18n("Stop"), 1000, this,
					 "SCAN_PROGRESS", true, 0  );
   progressDialog->setAutoClose( true );
   progressDialog->setAutoReset( true );

   connect( sane_device, SIGNAL(sigScanProgress(int)),
	    progressDialog, SLOT(setProgress(int)));
	

   /* Connect the Progress Dialogs cancel-Button */
   connect( progressDialog, SIGNAL( cancelled() ), sane_device,
	    SLOT( slStopScanning() ) );

   return( true );
}

ScanParams::~ScanParams()
{
    delete progressDialog;
}

void ScanParams::scannerParams( void ) // QVBoxLayout *top )
{
   KScanOption *so = 0;

   /* Its a real scanner */
   /* Mode setting */
   QHBox *hb = new QHBox(this);

   so = sane_device->getGuiElement( SANE_NAME_SCAN_MODE, hb );
   if ( so )
   {
      /* Pretty-Pixmap */

      KScanCombo *cb = (KScanCombo*) so->widget();


      cb->slSetIcon( pixLineArt, i18n("Lineart") );
      cb->slSetIcon( pixLineArt, i18n("Binary" ));
      cb->slSetIcon( pixGray, i18n("Gray") );
      cb->slSetIcon( pixColor, i18n("Color") );
      cb->slSetIcon( pixHalftone, i18n("Halftone") );

      hb->setMargin( KDialog::marginHint() );
      hb->setSpacing( KDialog::spacingHint());

      connect( so, SIGNAL(guiChange(KScanOption*)),
	       this, SLOT(slReloadAllGui( KScanOption* )));
   }

   /* Add a button for Source-Selection */
   if( sane_device->optionExists( SANE_NAME_SCAN_SOURCE ))
   {
      KScanOption source( SANE_NAME_SCAN_SOURCE );

      pb_source_sel = new QPushButton( i18n("Source..."), hb );
      connect( pb_source_sel, SIGNAL(clicked()), this, SLOT(slSourceSelect()));

#if 0 /* Testing !! TODO: remove */
      if( ! source.active() ) {
	 pb_source_sel->setEnabled( false );
      }
#endif
   }

   /* Resolution Setting -> X-Resolution Setting */
   so = sane_device->getGuiElement( SANE_NAME_SCAN_X_RESOLUTION, this,
				    i18n("Resolution (dpi):").local8Bit() );
   if ( so )
   {
      int x_y_res = 100;

      so->set( x_y_res );
      sane_device->apply( so );
      so->slRedrawWidget( so );

      connect( so, SIGNAL(guiChange(KScanOption*)),
	       this, SLOT(slReloadAllGui( KScanOption* )));

      xy_resolution_bind =
	 sane_device->getGuiElement(SANE_NAME_RESOLUTION_BIND, this,
				    i18n( "Bind X- and Y-Resolution" ).local8Bit() );
      if( xy_resolution_bind  )
      {
	 xy_resolution_bind->set( 0 );
	 sane_device->apply( xy_resolution_bind );

	 xy_resolution_bind->slRedrawWidget( xy_resolution_bind );
	 /* Connect to Gui-change-Slot */
	 connect( xy_resolution_bind, SIGNAL(guiChange(KScanOption*)),
		  this, SLOT(slReloadAllGui( KScanOption* )));
      }

      /* Resolution Setting -> Y-Resolution Setting */
      so = sane_device->getGuiElement( SANE_NAME_SCAN_Y_RESOLUTION, this );
      if ( so )
      {
	 so->set( x_y_res );
	 sane_device->apply( so );
	 so->slRedrawWidget( so );
      }
   }
   else
   {
      /* If the SCAN_X_RES does not exists, perhaps just SCAN_RES does */
      so = sane_device->getGuiElement( SANE_NAME_SCAN_RESOLUTION, this,
				       i18n("Resolution (dpi):") );
      if( so )
      {
	 so->set( 100 );
	 sane_device->apply( so );
      }
      else
      {
	 kdDebug(29000) << "SERIOUS: No Resolution setting possible !" << endl;
      }
   }

   /* Insert another beautification line */
   KSeparator *niceline = new KSeparator( KSeparator::HLine, this);

   /* Speed-Setting - show only if active */
   KScanOption kso_speed( SANE_NAME_SCAN_SPEED );
   if( kso_speed.valid() && kso_speed.softwareSetable() && kso_speed.active())
   {
      so = sane_device->getGuiElement( SANE_NAME_SCAN_SPEED, this );
   }

   /* Threshold-Setting */
   so = sane_device->getGuiElement( SANE_NAME_THRESHOLD, this );
   if( so )
   {
      so->set(50);
   }

   /* Brightness-Setting */
   so = sane_device->getGuiElement( SANE_NAME_BRIGHTNESS, this );

   /* Contrast-Setting */
   so = sane_device->getGuiElement( SANE_NAME_CONTRAST, this );
   /* Custom Gamma */

   /* Sharpness */
   so = sane_device->getGuiElement( "sharpness", this );
   /* The gamma table can be used - add  a button for editing */
   if( sane_device->optionExists( SANE_NAME_CUSTOM_GAMMA ) )
   {
      QHBox *hb1 = new QHBox(this);

      so = sane_device->getGuiElement( SANE_NAME_CUSTOM_GAMMA, hb1);

      if( so )
      {
	 int isOn;
	 so->get( &isOn );
	
	 kdDebug(29000) << "Custom Gamma Table is <" << (isOn ? "on" : "off") << ">" << endl;
	
	 /* Connect a signal to refresh activity of the gamma tables */
	 connect( so, SIGNAL(guiChange(KScanOption*)),
	       this, SLOT(slReloadAllGui( KScanOption* )));

	 pb_edit_gtable = new QPushButton( i18n("Edit..."), hb1 );
	 CHECK_PTR(pb_edit_gtable);
	 pb_edit_gtable->setEnabled( isOn );
	
	 connect( pb_edit_gtable, SIGNAL( clicked () ),
		  this, SLOT( slEditCustGamma () ) );

	 /* This connection cares for enabling/disabling the edit-Button */
	 connect( so, SIGNAL(guiChange(KScanOption*)),
		  this, SLOT(slOptionNotify(KScanOption*)));
      }
   }

   /* my Epson Perfection backends offer a list of user defined gamma values */
   
   /* Insert another beautification line */
   KSeparator *niceline1 = new KSeparator( KSeparator::HLine, this);

   so = sane_device->getGuiElement( SANE_NAME_NEGATIVE, this  );

   /* PREVIEW-Switch */
   kdDebug(29000) << "Try to get Gray-Preview" << endl;
   if( sane_device->optionExists( SANE_NAME_GRAY_PREVIEW ))
   {
     so = sane_device->getGuiElement( SANE_NAME_GRAY_PREVIEW, this );
     cb_gray_preview = (QCheckBox*) so->widget();
     QToolTip::add( cb_gray_preview, i18n("Acquire a gray preview even in color mode (faster)") );
   }
   // top->addStretch( 1 );
}


void ScanParams::createNoScannerMsg( void )
{
   /* Mode setting */
   QString cap;
   cap = i18n( "<B>Problem: No Scanner was found</B><P>Your system does not provide a SANE <I>(Scanner Access Now Easy)</I> installation, which is required by the KDE scan support.<P>Please install and configure SANE correctly on your system.<P>Visit the SANE homepage under http://wwww.mostang.com/sane to find out more about SANE installation and configuration. " );
   
   (void) new QLabel( cap, this );
   
}

/* This slot will be called if something changes with the option given as a param.
 * This is usefull if the parameter - Gui has widgets in his own space, which depend
 * on widget controlled by the KScanOption.
 */
void ScanParams::slOptionNotify( KScanOption *kso )
{
   if( !kso || !kso->valid()) return;

   kdDebug(29000) << "slOptionNotify called !" << endl;
   if( kso->getName() == SANE_NAME_CUSTOM_GAMMA )
   {
      int state;
      kso->get( &state );

      if( pb_edit_gtable )
      {
	 pb_edit_gtable->setEnabled( state );
      }
   }
}


void ScanParams::slSourceSelect( void )
{
   kdDebug(29000) << "Open Window for source selection !" << endl;
   KScanOption so( SANE_NAME_SCAN_SOURCE );
   ADF_BEHAVE adf = ADF_OFF;

   const QCString& currSource = so.get();
   kdDebug(29000) << "Current Source is <" << currSource << ">" << endl;
   QStrList sources;

   if( so.valid() )
   {
      sources = so.getList();
#undef CHEAT_FOR_DEBUGGING
#ifdef CHEAT_FOR_DEBUGGING
      if( sources.find( "Automatic Document Feeder" ) == -1)
          sources.append( "Automatic Document Feeder" );
#endif

      ScanSourceDialog d( this, sources, adf );
      d.slSetSource( currSource );

      if( d.exec() == QDialog::Accepted  )
      {
#ifdef __GNUC__
#warning and here is the rest
#endif
	 QString sel_source = d.getText();
	 adf = d.getAdfBehave();

	 /* set the selected Document source, the behavior is stored in a membervar */
	 so.set( sel_source.latin1() ); // FIX in ScanSourceDialog, then here
	 sane_device->apply( &so );

	 kdDebug(29000) << "Dialog finished OK: " << sel_source << ", " << adf << endl;

      }
   }
}

/** slFileSelect
 *  this slot allows to select a file or directory for virtuell Scanning.
 *  If a dir is selected, the virt. Scanner crawls for all readable
 *  images, if a single file is selected, the virt Scanner just reads
 *  one file.
 *  If SANE Debug Mode is selected, only pnm can be loaded.
 *  on Qt mode, all available Qt-Formats are ok.
 */
void ScanParams::slFileSelect( void )
{
   kdDebug(29000) << "File Selector" << endl;
   QString filter;
   QCString prefix = "\n*.";

   if( scan_mode == ID_QT_IMGIO )
   {
      QStrList filterList = QImage::inputFormats();
      filter = i18n( "*.*|All Files (*.*)");
      for( QCString fi_item = filterList.first(); !fi_item.isEmpty();
	   fi_item = filterList.next() )
      {
	
	 filter.append( QString::fromLatin1( prefix + fi_item.lower()) );
      }
   }
   else
   {
      filter.append( i18n( "*.pnm|PNM image files (*.pnm)") );
   }



   KFileDialog fd(last_virt_scan_path.path(), filter, this, "FileDialog",true);
   fd.setCaption( i18n("Select the inputfile") );
   /* Read the filename and remind it */
   QString fileName;
   if ( fd.exec() == QDialog::Accepted ) {
      fileName = fd.selectedFile();
      QFileInfo ppath( fileName );
      last_virt_scan_path = QDir(ppath.dirPath(true));
   } else {
      return;
   }

   if ( !fileName.isNull() && virt_filename ) { // got a file name
      kdDebug(29000) << "Got fileName: " << fileName << endl;
      // write Value to SANEOption, it updates itself.
      virt_filename->set( QFile::encodeName( fileName ) );
   }
}

/** Slot which is called if the user switches in the gui between
 *  the SANE-Debug-Mode and the qt image reading
 */
void ScanParams::slVirtScanModeSelect( int id )
{
   if( id == 0 ) {
      scan_mode = ID_SANE_DEBUG; /* , ID_QT_IMGIO */
      sane_device->guiSetEnabled( "three-pass", true );
      sane_device->guiSetEnabled( "grayify", true );
      sane_device->guiSetEnabled( SANE_NAME_CONTRAST, true );
      sane_device->guiSetEnabled( SANE_NAME_BRIGHTNESS, true );

      /* Check if the entered filename to delete */
      if( virt_filename ) {
	 QString vf = virt_filename->get();
	 kdDebug(29000) << "Found File in Filename-Option: " << vf << endl;

	 QFileInfo fi( vf );
	 if( fi.extension() != QString::fromLatin1("pnm") )
	    virt_filename->set(QCString(""));
      }
   } else {
      scan_mode = ID_QT_IMGIO;
      sane_device->guiSetEnabled( "three-pass", false );
      sane_device->guiSetEnabled( "grayify", false );
      sane_device->guiSetEnabled( SANE_NAME_CONTRAST, false );
      sane_device->guiSetEnabled( SANE_NAME_BRIGHTNESS, false );
   }
}


/**
 *   virtualScannerParams builds the GUI for the virtual scanner,
 *   which allows the user to read images from the harddisk.
 */
void ScanParams::virtualScannerParams( void )
{
#if 0
   KScanOption *so = 0;
   QWidget     *w = 0;

   /* Selection if virt. Scanner or SANE Debug */
   bg_virt_scan_mode = new QButtonGroup( 2, Qt::Horizontal,
					 this, "GroupBoxVirtScanner" );
   connect( bg_virt_scan_mode, SIGNAL(clicked(int)),
	    this, SLOT( slVirtScanModeSelect(int)));

   QRadioButton *rb1 = new QRadioButton( i18n("SANE debug (pnm only)"),
					 bg_virt_scan_mode, "VirtScanSANEDebug" );



   QRadioButton *rb2 = new QRadioButton( i18n("virt. Scan (all Qt modes)"),
					 bg_virt_scan_mode, "VirtScanQtModes" );

   rb1->setChecked( true );
   rb2->setChecked( false );

   if( scan_mode == ID_QT_IMGIO )
   {
      rb2->setChecked( true );
      rb1->setChecked( false );
   }

   top->addWidget( bg_virt_scan_mode, 1 );

   /* GUI-Element Filename */
   virt_filename = sane_device->getGuiElement( SANE_NAME_FILE, this );
   if( virt_filename )
   {
      QHBoxLayout *hb = new QHBoxLayout();
      top->addLayout( hb );
      w = virt_filename->widget();
      w->setMinimumHeight( (w->sizeHint()).height());
      connect( w, SIGNAL(returnPressed()), this,
	       SLOT( slCheckGlob()));

      hb->addWidget( w, 12 );

      QPushButton *pb_file_sel = new QPushButton( this );
      pb_file_sel->setPixmap(miniFloppyPixmap);
      //hb->addStretch( 1 );
      hb->addWidget( pb_file_sel, 1 );
      connect( pb_file_sel, SIGNAL(clicked()), this, SLOT(slFileSelect()));

   }

   /* GUI-Element Brightness */
   so = sane_device->getGuiElement( SANE_NAME_BRIGHTNESS, this );
   if( so )
   {
      top->addWidget( so->widget(), 1 );
      so->set( 0 );
      sane_device->apply( so );
   }

   /* GUI-Element Contrast */
   so = sane_device->getGuiElement( SANE_NAME_CONTRAST, this );
   if ( so )
   {
      top->addWidget( so->widget(), 1 );
      so->set( 0 );
      sane_device->apply( so );
   }


   /* GUI-Element grayify on startup */
   so = sane_device->getGuiElement( "grayify", this, i18n("convert the image to gray on loading"), 0 );
   if ( so )
   {
      top->addWidget( so->widget(), 1 );
      so->set( true );
      sane_device->apply( so );
   }

   /* GUI-Element three pass simulation */
   so = sane_device->getGuiElement( "three-pass", this, i18n("Simulate three pass acquirering"), 0 );
   if ( so )
   {
      top->addWidget( so->widget(), 1 );
      so->set( false );
      sane_device->apply( so );
   }
#endif
}

/* This slot is called if the user changes the
void ScanParams::slCheckGlob( void )
{

}
*/
/* Slot called to start scanning */
void ScanParams::slStartScan( void )
{
   KScanStat stat = KSCAN_OK;

   kdDebug(29000) << "Called start-scan-Slot!" << endl;
   QString q;

   if( scan_mode == ID_SANE_DEBUG || scan_mode == ID_QT_IMGIO )
   {
      if( virt_filename )
	 q = virt_filename->get();
      if( q.isEmpty() )
      {
	 QMessageBox::information( this, i18n("KSANE"),
				   i18n("The filename for virtual scanning is not set.\n"
					"Please set the filename first.") );
	 stat = KSCAN_ERR_PARAM;
      }
   }
   /* if it is a virt Scanner in SANE Debug or real scanning is on */
   if( stat == KSCAN_OK )
   {
      if( (scan_mode == ID_SANE_DEBUG || scan_mode == ID_SCAN) )
      {
	 if( adf == ADF_OFF )
	 {
	    /* Progress-Dialog */
	    progressDialog->setProgress(0);
	    if( progressDialog->isHidden())
	       progressDialog->show();
	    kdDebug(29000) << "* slStartScan: Start to acquire an image!" << endl;
	    stat = sane_device->acquire( );

	 } else {
	    kdDebug(29000) << "Not yet implemented :-/" << endl;
	
	    // stat = performADFScan();
	 }
      } else {
	 kdDebug(29000) << "Reading dir by Qt-internal imagereading file " << q << endl;
	 sane_device->acquire( q );
      }
   }
}

/* Slot called to start the Custom Gamma Table Edit dialog */

void ScanParams::slEditCustGamma( void )
{
    kdDebug(29000) << "Called EditCustGamma ;)" << endl;
    KGammaTable old_gt;

    KScanOption redGt( SANE_NAME_GAMMA_VECTOR_R );
    if( redGt.active()) kdDebug(29000) << "RED-GT is active now !" << endl;

    KScanOption blueGt( SANE_NAME_GAMMA_VECTOR_B );
    if( blueGt.active()) kdDebug(29000) << "blue-GT is active now !" << endl;

    KScanOption greenGt( SANE_NAME_GAMMA_VECTOR_G );
    if( greenGt.active()) kdDebug(29000) << "green-GT is active now !" << endl;

    KScanOption grayGt( SANE_NAME_GAMMA_VECTOR );
    if( grayGt.active()) kdDebug(29000) << "Gray-GT is active now !" << endl;
    grayGt.get( &old_gt );

    kdDebug(29000) << "Old gamma table: " << old_gt.getGamma() << ", " << old_gt.getBrightness() << ", " << old_gt.getContrast() << endl;

    GammaDialog gdiag( this );
    connect( &gdiag, SIGNAL( gammaToApply(GammaDialog&) ),
	     this, SLOT( slApplyGamma(GammaDialog&) ) );

    gdiag.setGt( old_gt );

    if( gdiag.exec() == QDialog::Accepted  )
    {
       slApplyGamma( gdiag );
    }

}


void ScanParams::slApplyGamma( GammaDialog& gdiag )
{
   KGammaTable *gt = gdiag.getGt();
   if( ! gt ) return;
   KGammaTable old_gt;

   KScanOption grayGt( SANE_NAME_GAMMA_VECTOR );
   grayGt.get( &old_gt );

   /* Now find out, which gamma-Tables are active. */
   if( grayGt.active() )
   {
      grayGt.set( gt );
      sane_device->apply( &grayGt, true );
   }

   KScanOption rGt( SANE_NAME_GAMMA_VECTOR_R );
   KScanOption gGt( SANE_NAME_GAMMA_VECTOR_G );
   KScanOption bGt( SANE_NAME_GAMMA_VECTOR_B );

   if( rGt.active() ) {
      rGt.set( gt );
      sane_device->apply( &rGt, true );
   }

   if( gGt.active() ) {
      gGt.set( gt );
      sane_device->apply( &gGt, true );
   }

   if( bGt.active() ) {
      bGt.set( gt );
      sane_device->apply( &bGt, true );
   }
   kdDebug(29000) << "Fine !" << endl;

}

/* Slot calls if a widget changes. Things to do:
 * - Apply the option and reload all if the option affects all
 */

void ScanParams::slReloadAllGui( KScanOption* t)
{
    if( !t || ! sane_device ) return;
    kdDebug(29000) << "This is slReloadAllGui for widget <" << t->getName() << ">" << endl;
    /* Need to reload all _except_ the one which was actually changed */

    sane_device->slReloadAllBut( t );

    /* Custom Gamma */
    KScanOption kso( SANE_NAME_CUSTOM_GAMMA );

    if( pb_edit_gtable )
    {
        int state;
        kso.get( &state );
	pb_edit_gtable->setEnabled( state );
    }
}


/* Slot called to start acquirering a preview */
void ScanParams::slAcquirePreview( void )
{
    kdDebug(29000) << "Called acquirePreview-Slot!" << endl;
    bool gray_preview = false;
    if( cb_gray_preview )
        gray_preview  = cb_gray_preview->isChecked();


    /* Maximal preview size */
    slMaximalScanSize();

    if( ! sane_device ) kdDebug(29000) << "Aeetsch: sane_device is 0 !" << endl;
    CHECK_PTR( sane_device );
    KScanStat stat = sane_device->acquirePreview( gray_preview );

    if( stat != KSCAN_OK )
    {
	kdDebug(29000) << "Error in scanning !" << endl;
    }
}

/* Custom Scan size setting.
 * The custom scan size is given in the QRect parameter. It must contain values
 * from 0..1000 which are interpreted as tenth of percent of the overall dimension.
 */
void ScanParams::slCustomScanSize( QRect sel)
{
   kdDebug(29000) << "Custom-Size: " << sel.x() << ", " << sel.y() << " - " << sel.width() << "x" << sel.height() << endl;

   KScanOption tl_x( SANE_NAME_SCAN_TL_X );
   KScanOption tl_y( SANE_NAME_SCAN_TL_Y );
   KScanOption br_x( SANE_NAME_SCAN_BR_X );
   KScanOption br_y( SANE_NAME_SCAN_BR_Y );

   double min1=0.0, max1=0.0, min2=0.0, max2=0.0, dummy1=0.0, dummy2=0.0;
   tl_x.getRange( &min1, &max1, &dummy1 );
   br_x.getRange( &min2, &max2, &dummy2 );

   /* overall width */
   double range = max2-min1;
   double w = min1 + double(range * (double(sel.x()) / 1000.0) );
   tl_x.set( w );
   w = min1 + double(range * double(sel.x() + sel.width())/1000.0);
   br_x.set( w );


   kdDebug(29000) << "set tl_x: " << min1 + double(range * (double(sel.x()) / 1000.0) ) << endl;
   kdDebug(29000) << "set br_x: " << min1 + double(range * (double(sel.x() + sel.width())/1000.0)) << endl;

   /** Y-Value setting */
   tl_y.getRange( &min1, &max1, &dummy1 );
   br_y.getRange(&min2, &max2, &dummy2 );

   /* overall width */
   range = max2-min1;
   w = min1 + range * double(sel.y()) / 1000.0;
   tl_y.set( w );
   w = min1 + range * double(sel.y() + sel.height())/1000.0;
   br_y.set( w );


   kdDebug(29000) << "set tl_y: " << min1 + double(range * (double(sel.y()) / 1000.0) ) << endl;
   kdDebug(29000) << "set br_y: " << min1 + double(range * (double(sel.y() + sel.height())/1000.0)) << endl;


   sane_device->apply( &tl_x );
   sane_device->apply( &tl_y );
   sane_device->apply( &br_x );
   sane_device->apply( &br_y );
}


	/**
	 * sets the scan area to the default, which is the whole area.
	 */
void ScanParams::slMaximalScanSize( void )
{
   kdDebug(29000) << "Setting to default" << endl;
   slCustomScanSize(QRect( 0,0,1000,1000));
}

#if 0
QSize  ScanParams::sizeHint( )
{
   QPushButton pb_example( i18n("Preview Scan"), this );
   // int w = pb_example.width();

   /** Hmm - not really clean  TODO ! */
   return( QSize( 360, 360 ) );

   // return( QSize( 2.2*w, 2.2*w) );

}
#endif
KScanStat ScanParams::performADFScan( void )
{
   KScanStat stat = KSCAN_OK;
   bool 		 scan_on = true;
	
   MassScanDialog *msd = new MassScanDialog( this );
   msd->show();

   /* The scan source should be set to ADF by the SourceSelect-Dialog */

   while( scan_on )
   {
      scan_on = false;
   }
   return( stat );
}
#include "scanparams.moc"
