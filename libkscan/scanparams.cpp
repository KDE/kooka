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

#include <qpushbutton.h>
#include <qimage.h>
#include <qtooltip.h>
#include <qprogressdialog.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qfileinfo.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kbuttonbox.h>
#include <kiconloader.h>
#include <kled.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kactivelabel.h>
#include <kmimetype.h>

#include <sane/saneopts.h>

#include "scansourcedialog.h"
#include "massscandialog.h"
#include "gammadialog.h"
#include "kscancontrols.h"

#include "scanparams.h"
#include "scanparams.moc"


// SANE testing options

#define TESTING_OPTIONS

#ifndef SANE_NAME_TEST_PICTURE
#define SANE_NAME_TEST_PICTURE		"test-picture"
#define SANE_TITLE_TEST_PICTURE		SANE_I18N("Test picture")
#define SANE_DESC_TEST_PICTURE		SANE_I18N("Select the test picture")
#endif

#ifndef SANE_NAME_THREE_PASS
#define SANE_NAME_THREE_PASS		"three-pass"
#define SANE_TITLE_THREE_PASS		SANE_I18N("3-pass mode")
#define SANE_DESC_THREE_PASS		SANE_I18N("Select whether to use 3-pass mode")
#endif

#ifndef SANE_NAME_HAND_SCANNER
#define SANE_NAME_HAND_SCANNER		"hand-scanner"
#define SANE_TITLE_HAND_SCANNER		SANE_I18N("Hand scanner")
#define SANE_DESC_HAND_SCANNER		SANE_I18N("Select whether this is a hand scanner")
#endif

#ifndef SANE_NAME_GRAYIFY
#define SANE_NAME_GRAYIFY		"grayify"
#define SANE_TITLE_GRAYIFY		SANE_I18N("Grayify")
#define SANE_DESC_GRAYIFY		SANE_I18N("Load the image as grayscale")
#endif




ScanParams::ScanParams( QWidget *parent, const char *name )
   : QFrame( parent, name )
{
    sane_device = NULL;
    virt_filename = NULL;
    pb_edit_gtable = NULL;
    cb_gray_preview = NULL;
    xy_resolution_bind = NULL;
    progressDialog = NULL;
    source_sel = NULL;

    m_firstGTEdit = true;

    /* Preload icons */
    pixColor = SmallIcon( "palette_color" );
    pixGray  = SmallIcon( "palette_gray" );
    pixLineArt = SmallIcon( "palette_lineart" );
    pixHalftone = SmallIcon( "palette_halftone" );

    /* intialise the default last save warnings */
    startupOptset = NULL;
}


bool ScanParams::connectDevice( KScanDevice *newScanDevice,bool galleryMode )
{
   /* Frame stuff for toplevel of scanparams - beautification */
    setFrameStyle(QFrame::Panel|QFrame::Raised);
    setLineWidth(1);

    QGridLayout *lay = new QGridLayout(this,6,2,KDialog::marginHint(),
                                       KDialog::spacingHint() );
    lay->setColStretch(0,9);

    if (newScanDevice==NULL)				// no scanner device
    {
        kdDebug(29000) << k_funcinfo << "No scan device, gallery=" << galleryMode << endl;
        sane_device = NULL;
        createNoScannerMsg(galleryMode);
        return (true);
    }

    sane_device = newScanDevice;
    ///* Debug: dump common Options */
    //QStrList strl = sane_device->getCommonOptions();
    //QString emp;
    //for ( emp=strl.first(); strl.current(); emp=strl.next() )
    //   kdDebug(29000) << "Common: " << strl.current() << endl;

    scan_mode = ID_SCAN;
    adf = ADF_OFF;

    QLabel *lab = new QLabel(i18n("<qt><b>Scanner&nbsp;Settings</b>"),this);
    lay->addWidget(lab,0,0,Qt::AlignLeft);

    m_led = new KLed(this);
    m_led->setState( KLed::Off );
    m_led->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ));
    lay->addWidget(m_led,0,1,Qt::AlignRight);

    lab = new QLabel(sane_device->getScannerName(),this);
    lay->addMultiCellWidget(lab,1,1,0,1,Qt::AlignLeft);

    KSeparator *sep = new KSeparator(KSeparator::HLine,this);
    lay->addMultiCellWidget(sep,2,2,0,1);

    /* load the startup scanoptions */
    startupOptset = new KScanOptSet( DEFAULT_OPTIONSET );
    Q_CHECK_PTR( startupOptset );

    if( !startupOptset->load( "Startup" ) )
    {
        kdDebug(29000) << "Could not load Startup-Options" << endl;
        delete startupOptset;
        startupOptset = NULL;
    }

    /* Now create Widgets for the important scan settings */
    QScrollView *sv = scannerParams();
    lay->addMultiCellWidget(sv,3,3,0,1);
    lay->setRowStretch(3,9);

    /* Reload all options to care for inactive options */
    sane_device->slReloadAll();

    sep = new KSeparator(KSeparator::HLine,this);
    lay->addMultiCellWidget(sep,4,4,0,1);

    /* Create the Scan Buttons */
    QPushButton* pb = new QPushButton(i18n("Preview"),this);
    pb->setMinimumWidth(100);
    connect( pb, SIGNAL(clicked()), this, SLOT(slAcquirePreview()) );
    lay->addWidget(pb,5,0,Qt::AlignLeft);

    pb = new QPushButton(i18n("Start Scan"),this);
    pb->setMinimumWidth(100);
    connect( pb, SIGNAL(clicked()), this, SLOT(slStartScan()) );
    lay->addWidget(pb,5,1,Qt::AlignRight);

    /* Initialise the progress dialog */
    progressDialog = new QProgressDialog( i18n("Scanning in progress"),
                                          i18n("Stop"), 100, 0L,
                                          "SCAN_PROGRESS", true, 0  );
    progressDialog->setAutoClose( true );
    progressDialog->setAutoReset( true );
    progressDialog->setCaption(i18n("Scanning"));

    connect( sane_device, SIGNAL(sigScanProgress(int)),
             progressDialog, SLOT(setProgress(int)));

    /* Connect the Progress Dialogs cancel-Button */
    connect( progressDialog, SIGNAL( cancelled() ), sane_device,
             SLOT( slStopScanning() ) );

    return (true);
}


ScanParams::~ScanParams()
{
    kdDebug(29000) << k_funcinfo << endl;

    delete startupOptset;  startupOptset = NULL;
    delete progressDialog; progressDialog = NULL;
}



void ScanParams::initialise(KScanOption *so)
{
    if (so==NULL) return;
    if (startupOptset==NULL) return;

    QCString name = so->getName();
    if (!name.isEmpty())
    {
        QCString val = startupOptset->getValue(name);
        kdDebug(29000) << "Initialising <" << name << "> with value <" << val << ">" << endl;
        so->set(val);
        sane_device->apply(so);
    }
}




QScrollView *ScanParams::scannerParams()
{
    KScanOption *so;

    QScrollView *sv = new QScrollView(this);
    sv->setHScrollBarMode( QScrollView::AlwaysOff );
    sv->setResizePolicy( QScrollView::AutoOneFit );

    QWidget *frame = new QWidget(sv->viewport());

    frame->setBackgroundColor(sv->backgroundColor());

    QGridLayout *lay = new QGridLayout(frame,1,4,KDialog::marginHint(),2*KDialog::spacingHint());
    lay->setColStretch(2,1);
    lay->setColSpacing(1,KDialog::marginHint());

    int row = 0;


    virt_filename = sane_device->getGuiElement( SANE_NAME_FILE, frame,
                                                SANE_TITLE_FILE,
                                                SANE_DESC_FILE );
    if (virt_filename!=NULL)
    {
        initialise( virt_filename );
        connect( virt_filename, SIGNAL(guiChange(KScanOption*)),
                 SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(virt_filename->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(virt_filename->widget(),row,row,2,3);
        ++row;

        /* Selection for either virtual scanner or SANE debug */
        QButtonGroup *vbg = new QButtonGroup( 2, Qt::Vertical, i18n("Testing Mode"),frame);
        connect(vbg,SIGNAL(clicked(int)),SLOT(slVirtScanModeSelect(int)));

        QRadioButton *rb1 = new QRadioButton(i18n("SANE Debug (from PNM image)"),vbg);
        QRadioButton *rb2 = new QRadioButton(i18n("Virtual Scanner (any image format)"),vbg);

        if (scan_mode==ID_SCAN) scan_mode = ID_SANE_DEBUG;
        rb1->setChecked(scan_mode==ID_SANE_DEBUG);
        rb2->setChecked(scan_mode==ID_QT_IMGIO);

        lay->addMultiCellWidget(vbg,row,row,2,3);
        ++row;

        lay->addMultiCellWidget(new KSeparator(KSeparator::HLine,frame),row,row,0,3);
        ++row;
    }

    /* Mode setting */
    so = sane_device->getGuiElement( SANE_NAME_SCAN_MODE, frame,
                                     SANE_TITLE_SCAN_MODE,
                                     SANE_DESC_SCAN_MODE );
    if (so!=NULL)
    {
        KScanCombo *cb = (KScanCombo*) so->widget();

        // Having loaded the 'sane-backends' message catalogue, these strings
        // are now translatable.
        cb->slSetIcon( pixLineArt, i18n("Line art") );
        cb->slSetIcon( pixLineArt, i18n("Lineart") );
        cb->slSetIcon( pixLineArt, i18n("Binary") );
        cb->slSetIcon( pixGray, i18n("Gray") );
        cb->slSetIcon( pixGray, i18n("Grey") );
        cb->slSetIcon( pixColor, i18n("Color") );
        cb->slSetIcon( pixColor, i18n("Colour") );
        cb->slSetIcon( pixHalftone, i18n("Halftone") );

        initialise( so );
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(cb,row,row,2,3);
        ++row;
    }

    /* Resolution Setting -> try X-Resolution Setting */
    so = sane_device->getGuiElement( SANE_NAME_SCAN_X_RESOLUTION, frame,
                                     i18n("Resolution"),
                                     SANE_DESC_SCAN_X_RESOLUTION );
    if (so!=NULL)
    {
        initialise( so );
        int x_y_res;
        so->get( &x_y_res );
        so->slRedrawWidget( so );

        /* connect to slot that passes the resolution to the previewer */
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 this, SLOT( slNewXResolution(KScanOption*)));
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addWidget(so->widget(),row,2);
        lay->addWidget(new QLabel(i18n("dpi"),frame),row,3,Qt::AlignLeft);
							// SANE resolution always in DPI
        ++row;

        xy_resolution_bind = sane_device->getGuiElement(SANE_NAME_RESOLUTION_BIND, frame,
                                                        SANE_TITLE_RESOLUTION_BIND,
                                                        SANE_DESC_RESOLUTION_BIND );
        if (xy_resolution_bind!=NULL)
        {
            initialise( xy_resolution_bind );
            xy_resolution_bind->slRedrawWidget( xy_resolution_bind );

            /* Connect to Gui-change-Slot */
            connect( xy_resolution_bind, SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slReloadAllGui( KScanOption* )));

            lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
            lay->addMultiCellWidget(so->widget(),row,row,2,3);
            ++row;
        }

        /* Resolution Setting -> Y-Resolution Setting */
        so = sane_device->getGuiElement( SANE_NAME_SCAN_Y_RESOLUTION, frame,
                                         SANE_TITLE_SCAN_Y_RESOLUTION,
                                         SANE_DESC_SCAN_Y_RESOLUTION );
        int y_res = x_y_res;
        if (so!=NULL)
        {
            initialise( so );
            if( so->active() )
                so->get( &y_res );
            so->slRedrawWidget( so );

            lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
            lay->addWidget(so->widget(),row,2);
            lay->addWidget(new QLabel(i18n("dpi"),frame),row,3,Qt::AlignLeft);
            ++row;
        }

        emit scanResolutionChanged( x_y_res, y_res );
    }
    else
    {
        /* If the SCAN_X_RES does not exists, perhaps just SCAN_RES does */
        so = sane_device->getGuiElement( SANE_NAME_SCAN_RESOLUTION, frame,
                                         SANE_TITLE_SCAN_Y_RESOLUTION,
                                         SANE_DESC_SCAN_X_RESOLUTION );
        if (so!=NULL)
        {
            initialise( so );
            so->slRedrawWidget( so );

            /* connect to slot that passes the resolution to the previewer */
            connect( so, SIGNAL(guiChange(KScanOption*)),
                     this, SLOT( slNewXResolution(KScanOption*)));
            connect( so, SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slReloadAllGui( KScanOption* )));

            lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
            lay->addWidget(so->widget(),row,2);
            lay->addWidget(new QLabel(i18n("dpi"),frame),row,3,Qt::AlignLeft);
            ++row;
        }
        else
        {
            kdDebug(29000) << "SERIOUS: No Resolution setting possible!" << endl;
        }
    }

    /* Insert another beautification line */
    lay->addMultiCellWidget(new KSeparator(KSeparator::HLine,frame),row,row,0,3);
    ++row;

    /* Add a button for Source-Selection */
    
    source_sel = sane_device->getGuiElement( SANE_NAME_SCAN_SOURCE, frame,
                                             SANE_TITLE_SCAN_SOURCE,
                                             SANE_DESC_SCAN_SOURCE );
    if (source_sel!=NULL)
    {
        initialise( source_sel );
        connect( source_sel, SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(source_sel->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(source_sel->widget(),row,row,2,3);
        ++row;

        //  Will need to enable the "Advanced" dialogue later, because that
        //  contains other ADF options.  They are not implemented at the moment
        //  but they may be some day...
        //QPushButton *pb = new QPushButton( i18n("Source && ADF Options..."), frame);
        //connect(pb, SIGNAL(clicked()), SLOT(slSourceSelect()));
        //lay->addMultiCellWidget(pb,row,row,2,3,Qt::AlignRight);
        //++row;
    }

    /* Halftoning */
    so = sane_device->getGuiElement( SANE_NAME_HALFTONE, frame,
                                     SANE_TITLE_HALFTONE,
                                     SANE_DESC_HALFTONE );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    so = sane_device->getGuiElement( SANE_NAME_HALFTONE_DIMENSION, frame,
                                     SANE_TITLE_HALFTONE_DIMENSION,
                                     SANE_DESC_HALFTONE_DIMENSION );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    so = sane_device->getGuiElement( SANE_NAME_HALFTONE_PATTERN, frame,
                                     SANE_TITLE_HALFTONE_PATTERN,
                                     SANE_DESC_HALFTONE_PATTERN );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    // SANE "test" options
    so = sane_device->getGuiElement( SANE_NAME_TEST_PICTURE, frame,
                                     SANE_TITLE_TEST_PICTURE,
                                     SANE_DESC_TEST_PICTURE );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    kdDebug(29000) << "Bit-Depth exists" << endl;
    so = sane_device->getGuiElement( SANE_NAME_BIT_DEPTH, frame,
                                     SANE_TITLE_BIT_DEPTH,
                                     SANE_DESC_BIT_DEPTH );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

#ifdef TESTING_OPTIONS
    so = sane_device->getGuiElement( SANE_NAME_THREE_PASS, frame,
                                     SANE_TITLE_THREE_PASS,
                                     SANE_DESC_THREE_PASS );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    so = sane_device->getGuiElement( SANE_NAME_HAND_SCANNER, frame,
                                     SANE_TITLE_HAND_SCANNER,
                                     SANE_DESC_HAND_SCANNER );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    so = sane_device->getGuiElement( SANE_NAME_GRAYIFY, frame,
                                     SANE_TITLE_GRAYIFY,
                                     SANE_DESC_GRAYIFY );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slReloadAllGui( KScanOption* )));

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }
#endif

    /* Speed-Setting - show only if active */
    KScanOption kso_speed( SANE_NAME_SCAN_SPEED );
    if( kso_speed.valid() && kso_speed.softwareSetable() && kso_speed.active())
    {
        so = sane_device->getGuiElement( SANE_NAME_SCAN_SPEED, frame,
                                         SANE_TITLE_SCAN_SPEED,
                                         SANE_DESC_SCAN_SPEED );
        initialise( so );

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    /* Threshold-Setting */
    so = sane_device->getGuiElement( SANE_NAME_THRESHOLD, frame,
                                     SANE_TITLE_THRESHOLD,
                                     SANE_DESC_THRESHOLD);
    if (so!=NULL)
    {
        initialise( so );
        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    /* Brightness-Setting */
    so = sane_device->getGuiElement( SANE_NAME_BRIGHTNESS, frame,
                                     SANE_TITLE_BRIGHTNESS,
                                     SANE_DESC_BRIGHTNESS);
    if (so!=NULL)
    {
        initialise( so );
        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    /* Contrast-Setting */
    so = sane_device->getGuiElement( SANE_NAME_CONTRAST, frame,
                                     SANE_TITLE_CONTRAST,
                                     SANE_DESC_CONTRAST );
    if (so!=NULL)
    {
        initialise( so );
        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    /* Sharpness */
    so = sane_device->getGuiElement( "sharpness", frame );
    if (so!=NULL)
    {
        initialise( so );
        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }


    /* The gamma table can be used - add a button for editing */
    if (sane_device->optionExists( SANE_NAME_CUSTOM_GAMMA ) )
    {
        QHBox *hb1 = new QHBox(frame);
        so = sane_device->getGuiElement( SANE_NAME_CUSTOM_GAMMA, hb1,
                                         SANE_TITLE_CUSTOM_GAMMA,
                                         SANE_DESC_CUSTOM_GAMMA );
        if (so!=NULL)
        {
            initialise( so );
            connect( so,   SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slReloadAllGui( KScanOption* )));

            /* This connection cares for enabling/disabling the edit-Button */
            connect( so,   SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slOptionNotify(KScanOption*)));
        }

        /* Connect a signal to refresh activity of the gamma tables */
        (void) new QWidget( hb1 ); /* dummy widget to eat space */

        pb_edit_gtable = new QPushButton( i18n("Edit..."), hb1 );

        connect( pb_edit_gtable, SIGNAL( clicked () ),
                 this, SLOT( slEditCustGamma () ) );
        setEditCustomGammaTableState();

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(hb1,row,row,2,3);
        ++row;
    }
    /* my Epson Perfection backends offer a list of user defined gamma values */

    so = sane_device->getGuiElement( SANE_NAME_NEGATIVE, frame,
                                     SANE_TITLE_NEGATIVE,
                                     SANE_DESC_NEGATIVE );
    if (so!=NULL)
    {
        initialise( so );
        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
    }

    /* Preview-Switch */
    so = sane_device->getGuiElement( SANE_NAME_GRAY_PREVIEW, frame,
                                     SANE_TITLE_GRAY_PREVIEW,
                                     SANE_DESC_GRAY_PREVIEW );
    if (so!=NULL)
    {
        initialise( so );
        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addMultiCellWidget(so->widget(),row,row,2,3);
        ++row;
        cb_gray_preview = (QCheckBox*) so->widget();
        QToolTip::add( cb_gray_preview, i18n("Acquire a gray preview even in color mode (faster)") );
    }

    lay->addMultiCellWidget(new QLabel("",frame),row,row,0,3);
    lay->setRowStretch(row,1);				// dummy row for stretch

    frame->setMinimumWidth(frame->sizeHint().width());
    sv->setMinimumWidth(frame->minimumWidth());
    sv->addChild(frame);

    return (sv);
}




void ScanParams::createNoScannerMsg(bool galleryMode)
{
    QString msg;
    if (galleryMode)
    {
        msg = i18n("<qt>\
<b>Gallery Mode: No scanner selected</b>\
<p>\
In this mode you can browse, manipulate and OCR images already in the gallery.\
<p>\
Select a scanner device \
(use the menu option <i>Settings&nbsp;- Select Scan&nbsp;Device</i>) \
to perform scanning.");
    }
    else
    {
        msg = i18n("<qt>\
<b>Problem: No scanner found, or unable to access it</b>\
<p>\
There was a problem using the SANE (Scanner Access Now Easy) library to access \
the scanner device.  There may be a problem with your SANE installation, or it \
may not be configured to support your scanner.\
<p>\
Check that SANE is correctly installed and configured on your system, and \
also that the scanner device name and settings are correct.\
<p>\
See the SANE project home page \
(<a href=\"http://www.sane-project.org\">www.sane-project.org</a>) \
for more information on SANE installation and setup.");
    }

    KActiveLabel *lab = new KActiveLabel(msg,this);
    QGridLayout *lay = dynamic_cast<QGridLayout *>(layout());
    if (lay!=NULL) lay->addWidget(lab,0,0);
}



/* This slot will be called if something changes with the option given as a param.
 * This is useful if the parameter - Gui has widgets in his own space, which depend
 * on widget controlled by the KScanOption.
 */
void ScanParams::slOptionNotify( KScanOption *kso )
{
   if( !kso || !kso->valid()) return;
   setEditCustomGammaTableState	();
}


void ScanParams::slSourceSelect( void )
{
   kdDebug(29000) << "Open Window for source selection !" << endl;
   KScanOption so( SANE_NAME_SCAN_SOURCE );
   AdfBehaviour adf = ADF_OFF;

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
	 QString sel_source = d.getText();
	 adf = d.getAdfBehave();

	 /* set the selected Document source, the behavior is stored in a membervar */
	 so.set( QCString(sel_source.latin1()) ); // FIX in ScanSourceDialog, then here
	 sane_device->apply( &so );

         if (source_sel!=NULL)
         {
             source_sel->slReload();
             source_sel->slRedrawWidget(source_sel);
         }

	 kdDebug(29000) << "Dialog finished OK: " << sel_source << ", " << adf << endl;

      }
   }
}


/** Slot which is called if the user switches in the gui between
 *  the SANE-Debug-Mode and the qt image reading
 */
void ScanParams::slVirtScanModeSelect(int id)
{
    if (id==0)						// SANE Debug
    {
        scan_mode = ID_SANE_DEBUG;
        sane_device->guiSetEnabled( SANE_NAME_HAND_SCANNER, true );
        sane_device->guiSetEnabled( SANE_NAME_THREE_PASS, true );
        sane_device->guiSetEnabled( SANE_NAME_GRAYIFY, true );
        sane_device->guiSetEnabled( SANE_NAME_CONTRAST, true );
        sane_device->guiSetEnabled( SANE_NAME_BRIGHTNESS, true );

        ///* Check if the entered filename is valid for this mode, otherwise clear it */
        //if (virt_filename!=NULL)
        //{
        //   QString vf = virt_filename->get();
        //   kdDebug(29000) << "Found File in Filename-Option: " << vf << endl;
        //   QFileInfo fi(vf);
        //   if (fi.extension()!="pnm") virt_filename->set(QCString(""));
        //}
    }
    else						// Virtual Scanner
    {
        scan_mode = ID_QT_IMGIO;
        sane_device->guiSetEnabled( SANE_NAME_HAND_SCANNER, false );
        sane_device->guiSetEnabled( SANE_NAME_THREE_PASS, false );
        sane_device->guiSetEnabled( SANE_NAME_GRAYIFY, false );
        sane_device->guiSetEnabled( SANE_NAME_CONTRAST, false );
        sane_device->guiSetEnabled( SANE_NAME_BRIGHTNESS, false );
    }
}



KScanStat ScanParams::prepareScan(QString *vfp)
{
    kdDebug(29000) << k_funcinfo << "scan mode=" << scan_mode << endl;

    KScanStat stat = KSCAN_OK;
    QString virtfile;

    if (scan_mode==ID_SANE_DEBUG || scan_mode==ID_QT_IMGIO)
    {
        if (virt_filename!=NULL) virtfile = virt_filename->get();
        if (virtfile.isEmpty())
        {
            KMessageBox::sorry(this,i18n("A file must be entered for testing or virtual scanning"));
            stat = KSCAN_ERR_PARAM;
        }

        if (stat==KSCAN_OK)
        {
            QFileInfo fi(virtfile);
            if (!fi.exists())
            {
                KMessageBox::sorry(this,i18n("<qt>The testing or virtual file<br><b>%1</b><br>was not found or is not readable").arg(virtfile));
                stat = KSCAN_ERR_PARAM;
            }
        }

        if (stat==KSCAN_OK)
        {
            if (scan_mode==ID_SANE_DEBUG)
            {
                KMimeType::Ptr mimetype = KMimeType::findByPath(virtfile);
                if (!(mimetype->is("image/x-portable-bitmap") ||
                      mimetype->is("image/x-portable-greymap") ||
                      mimetype->is("image/x-portable-pixmap")))
                {
                    KMessageBox::sorry(this,i18n("<qt>SANE Debug can only read PNM files.<br>"
                                                 "This file is type <b>%1</b>.").arg(mimetype->name()));
                    stat = KSCAN_ERR_PARAM;
                }
            }
        }
    }

    if (vfp!=NULL) *vfp = virtfile;
    return (stat);
}



/* Slot called to start acquiring a preview */
void ScanParams::slAcquirePreview()
{
    if (scan_mode==ID_QT_IMGIO)
    {
        KMessageBox::sorry(this,i18n("Cannot preview in Virtual Scanner mode"));
        return;
    }

    QString virtfile;
    KScanStat stat = prepareScan(&virtfile);
    if (stat!=KSCAN_OK) return;

    kdDebug(29000) << k_funcinfo << "scan mode=" << scan_mode << " virtfile=[" << virtfile << "]" << endl;

    bool gray_preview = false;
    if (cb_gray_preview!=NULL) gray_preview  = cb_gray_preview->isChecked();

    slMaximalScanSize();				/* Always preview at maximal size */

    stat = sane_device->acquirePreview(gray_preview);
    if (stat!=KSCAN_OK) kdDebug(29000) << k_funcinfo << "Error, preview status " << stat << endl;
}


/* Slot called to start scanning */
void ScanParams::slStartScan()
{
    QString virtfile;
    KScanStat stat = prepareScan(&virtfile);
    if (stat!=KSCAN_OK) return;

    kdDebug(29000) << k_funcinfo << "scan mode=" << scan_mode << " virtfile=[" << virtfile << "]" << endl;

    if (scan_mode!=ID_QT_IMGIO)				// acquire via SANE
    {
        if (adf==ADF_OFF)
        {
	    /* Progress-Dialog */
	    progressDialog->setProgress(0);
	    if (progressDialog->isHidden()) progressDialog->show();
	    kdDebug(29000) << "Start to acquire image" << endl;
	    stat = sane_device->acquire();
        }
        else
        {
	    kdDebug(29000) << "ADF Scan not yet implemented :-/" << endl;
	    // stat = performADFScan();
        }
    }
    else						// acquire via Qt-IMGIO
    {
        kdDebug(29000) << "Acquiring from virtual file" << endl;
        stat = sane_device->acquire(virtfile);
    }

    if (stat!=KSCAN_OK) kdDebug(29000) << k_funcinfo << "Error, scan status " << stat << endl;
}



/* Slot called to start the Custom Gamma Table Edit dialog */

void ScanParams::slEditCustGamma( void )
{
    kdDebug(29000) << "Called EditCustGamma ;)" << endl;
    KGammaTable old_gt;


    /* Since gammatable options are not set in the default gui, it must be
     * checked if it is the first edit. If it is, take from loaded default
     * set if available there */
    if( m_firstGTEdit && startupOptset )
    {
       m_firstGTEdit = false;
       KScanOption *gt = startupOptset->get(SANE_NAME_GAMMA_VECTOR);
       if( !gt )
       {
	  /* If it  not gray, it should be one color. */
	  gt = startupOptset->get( SANE_NAME_GAMMA_VECTOR_R );
       }

       if( gt )
	  gt->get( &old_gt );
    }
    else
    {
       /* it is not the first edit, use older values */
       if( sane_device->optionExists( SANE_NAME_GAMMA_VECTOR ) )
       {
	  KScanOption grayGt( SANE_NAME_GAMMA_VECTOR );
	  /* This will be fine for all color gt's. */
	  grayGt.get( &old_gt );
	  kdDebug(29000) << "Gray Gamma Table is active " << endl;
       }
       else
       {
	  /* Gray is not active, but at the current implementation without
	   * red/green/blue gammatables, but one for all, all gammatables
	   * are equally. So taking the red one should be fine. TODO
	   */
	  if( sane_device->optionExists( SANE_NAME_GAMMA_VECTOR_R ))
	  {
	     KScanOption redGt( SANE_NAME_GAMMA_VECTOR_R );
	     redGt.get( &old_gt );
	     kdDebug(29000) << "Getting old gamma table from Red channel" << endl;
	  }
	  else
	  {
	     /* uh ! No current gammatable could be retrieved. Use the 100/0/0 gt
	      * created by KGammaTable's constructor. Nothing to do for that.
	      */
	     kdDebug(29000) << "WRN: Could not retrieve a gamma table" << endl;
	  }
       }
    }

    kdDebug(29000) << "Old gamma table: " << old_gt.getGamma() << ", " << old_gt.getBrightness() << ", " << old_gt.getContrast() << endl;

    GammaDialog gdiag( this );
    connect( &gdiag, SIGNAL( gammaToApply(KGammaTable*) ),
	     this,     SLOT( slApplyGamma(KGammaTable*) ) );

    gdiag.setGt( old_gt );

    if( gdiag.exec() == QDialog::Accepted  )
    {
       slApplyGamma( gdiag.getGt() );
       kdDebug(29000) << "Fine, applied new Gamma Table !" << endl;
    }
    else
    {
       /* reset to old values */
       slApplyGamma( &old_gt );
       kdDebug(29000) << "Cancel, reverted to old Gamma Table !" << endl;
    }

}


void ScanParams::slApplyGamma( KGammaTable* gt )
{
   if( ! gt ) return;

   kdDebug(29000) << "Applying gamma table: " << gt->getGamma() <<
      ", " << gt->getBrightness() << ", " << gt->getContrast() << endl;


   if( sane_device->optionExists( SANE_NAME_GAMMA_VECTOR ) )
   {
      KScanOption grayGt( SANE_NAME_GAMMA_VECTOR );

      /* Now find out, which gamma-Tables are active. */
      if( grayGt.active() )
      {
	 grayGt.set( gt );
	 sane_device->apply( &grayGt, true );
      }
   }

   if( sane_device->optionExists( SANE_NAME_GAMMA_VECTOR_R )) {
      KScanOption rGt( SANE_NAME_GAMMA_VECTOR_R );
      if( rGt.active() )
      {
	 rGt.set( gt );
	 sane_device->apply( &rGt, true );
      }
   }

   if( sane_device->optionExists( SANE_NAME_GAMMA_VECTOR_G )) {
      KScanOption gGt( SANE_NAME_GAMMA_VECTOR_G );
      if( gGt.active() )
      {
	 gGt.set( gt );
	 sane_device->apply( &gGt, true );
      }
   }

   if( sane_device->optionExists( SANE_NAME_GAMMA_VECTOR_B )) {
      KScanOption bGt( SANE_NAME_GAMMA_VECTOR_B );
      if( bGt.active() )
      {
	 bGt.set( gt );
	 sane_device->apply( &bGt, true );
      }
   }
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

    /* Custom Gamma <- What happens if that does not exist for some scanner ? TODO */
    setEditCustomGammaTableState();
}

/*
 * enable editing of the gamma tables if one of the gamma tables
 * exists and is active at the moment
 */
void ScanParams::setEditCustomGammaTableState()
{
   if( !(sane_device && pb_edit_gtable) )
      return;

   bool butState = false;
   kdDebug(29000) << "Checking state of edit custom gamma button !" << endl;

   if( sane_device->optionExists( SANE_NAME_CUSTOM_GAMMA ) )
   {
      KScanOption kso( SANE_NAME_CUSTOM_GAMMA );
      butState = kso.active();
      // kdDebug(29000) << "CustomGamma is active: " << butState << endl;
   }

   if( !butState && sane_device->optionExists( SANE_NAME_GAMMA_VECTOR_R ) )
   {
      KScanOption kso( SANE_NAME_GAMMA_VECTOR_R );
      butState = kso.active();
      // kdDebug(29000) << "CustomGamma Red is active: " << butState << endl;
   }

   if( !butState && sane_device->optionExists( SANE_NAME_GAMMA_VECTOR_G ) )
   {
      KScanOption kso( SANE_NAME_GAMMA_VECTOR_G );
      butState = kso.active();
      // kdDebug(29000) << "CustomGamma Green is active: " << butState << endl;
   }

   if( !butState && sane_device->optionExists( SANE_NAME_GAMMA_VECTOR_B ) )
   {
      KScanOption kso( SANE_NAME_GAMMA_VECTOR_B );
      butState = kso.active();
      // kdDebug(29000) << "CustomGamma blue is active: " << butState << endl;
   }
   pb_edit_gtable->setEnabled( butState );
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


void ScanParams::slNewXResolution(KScanOption *opt)
{
   if(! opt ) return;

   kdDebug(29000) << "Got new X-Resolution !" << endl;

   int x_res = 0;
   opt->get( &x_res );

   int y_res = x_res;

   if( xy_resolution_bind && xy_resolution_bind->active() )
   {
      /* That means, that x and y may be different */
      KScanOption opt_y( SANE_NAME_SCAN_Y_RESOLUTION );
      if( opt_y.valid () )
      {
	 opt_y.get( &y_res );
      }
   }

   emit( scanResolutionChanged( x_res, y_res ) );
}

void ScanParams::slNewYResolution(KScanOption *opt)
{
   if( ! opt ) return;

   int y_res = 0;
   opt->get( &y_res );

   int x_res = y_res;

   if( xy_resolution_bind && xy_resolution_bind->active())
   {
      /* That means, that x and y may be different */
      KScanOption opt_x( SANE_NAME_SCAN_X_RESOLUTION );
      if( opt_x.valid () )
      {
	 opt_x.get( &x_res );
      }
   }

   emit( scanResolutionChanged( x_res, y_res ) );

}


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
