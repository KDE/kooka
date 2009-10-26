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

#include "scanparams.h"
#include "scanparams.moc"

#include <qpushbutton.h>
#include <qimage.h>
#include <qtooltip.h>
#include <qprogressdialog.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qfileinfo.h>
#include <qgridlayout.h>
#include <qlabel.h>
#include <qscrollarea.h>
#include <qstyle.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kled.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <khbox.h>

extern "C"
{
#include <sane/saneopts.h>
}

#include "scansourcedialog.h"
#include "massscandialog.h"
#include "gammadialog.h"
#include "kgammatable.h"
#include "kscancontrols.h"
#include "scansizeselector.h"


//  SANE testing options
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


ScanParams::ScanParams( QWidget *parent)
   : QFrame( parent)
{
    setObjectName("ScanParams");

    sane_device = NULL;
    virt_filename = NULL;
    pb_edit_gtable = NULL;
    cb_gray_preview = NULL;
    xy_resolution_bind = NULL;
    progressDialog = NULL;
    source_sel = NULL;

    m_firstGTEdit = true;

    /* Preload icons */
    KIconLoader::global()->addAppDir("libkscan");	// access to our icons
    mIconColor = KIcon("palette-color");
    mIconGray = KIcon("palette-gray");
    mIconLineart = KIcon("palette-lineart");
    mIconHalftone = KIcon("palette-halftone");

    /* intialise the default last save warnings */
    startupOptset = NULL;
}


bool ScanParams::connectDevice(KScanDevice *newScanDevice, bool galleryMode)
{
   /* Frame stuff for toplevel of scanparams - beautification */
    setFrameStyle(QFrame::Panel|QFrame::Raised);
    setLineWidth(1);

    QGridLayout *lay = new QGridLayout(this);
    lay->setSpacing(KDialog::spacingHint());
    lay->setColumnStretch(0,9);

    if (newScanDevice==NULL)				// no scanner device
    {
        kDebug() << "No scan device, gallery=" << galleryMode;
        sane_device = NULL;
        createNoScannerMsg(galleryMode);
        return (true);
    }

    sane_device = newScanDevice;
    scan_mode = ID_SCAN;
    adf = ADF_OFF;

    QLabel *lab = new QLabel(i18n("<qt><b>Scanner&nbsp;Settings</b>"),this);
    lay->addWidget(lab,0,0,Qt::AlignLeft);

    m_led = new KLed(this);
    m_led->setState( KLed::Off );
    m_led->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ));
    lay->addWidget(m_led,0,1,Qt::AlignRight);

    lab = new QLabel(sane_device->getScannerName(),this);
    lay->addWidget(lab,1,0,1,2,Qt::AlignLeft);

    lay->addWidget(new KSeparator(Qt::Horizontal,this),2,0,1,-1);

    /* load the startup scanoptions */
    // TODO: check whether the saved scanner options apply to the current scanner?
    // They may be for a completely different one...
    // Or update KScanDevice to save the startup options on a per-scanner basis.
    startupOptset = new KScanOptSet( DEFAULT_OPTIONSET );
    Q_CHECK_PTR( startupOptset );

    if( !startupOptset->load( "Startup" ) )
    {
        kDebug() << "Could not load Startup-Options";
        delete startupOptset;
        startupOptset = NULL;
    }

    /* Now create Widgets for the important scan settings */
    QScrollArea *sv = createScannerParams();
    lay->addWidget(sv,3,0,1,2);
    lay->setRowStretch(3,9);

    /* Reload all options to care for inactive options */
    sane_device->slotReloadAll();

    lay->addWidget(new KSeparator(Qt::Horizontal,this),4,0,1,-1);

    /* Create the Scan Buttons */
    QPushButton* pb = new QPushButton(i18n("Pre&view"),this);
    pb->setMinimumWidth(100);
    connect( pb, SIGNAL(clicked()), SLOT(slotAcquirePreview()) );
    lay->addWidget(pb,5,0,Qt::AlignLeft);

    pb = new QPushButton(i18n("Star&t Scan"),this);
    pb->setMinimumWidth(100);
    connect( pb, SIGNAL(clicked()), SLOT(slotStartScan()) );
    lay->addWidget(pb,5,1,Qt::AlignRight);

    /* Initialise the progress dialog */
    progressDialog = new QProgressDialog( i18n("Scanning in progress"),
                                          i18n("Stop"), 0, 100, NULL);
    progressDialog->setModal(true);
    progressDialog->setAutoClose(true);
    progressDialog->setAutoReset(true);
    progressDialog->setMinimumDuration(100);
    progressDialog->setWindowTitle(i18n("Scanning"));

    connect( sane_device, SIGNAL(sigScanProgress(int)),
             progressDialog, SLOT(setValue(int)));

    /* Connect the Progress Dialogs cancel-Button */
    connect( progressDialog, SIGNAL( canceled() ), sane_device,
             SLOT( slotStopScanning() ) );

    return (true);
}


ScanParams::~ScanParams()
{
    kDebug();

    delete startupOptset;  startupOptset = NULL;
    delete progressDialog; progressDialog = NULL;
}




void ScanParams::initialise(KScanOption *so)
{
    if (so==NULL) return;
    if (startupOptset==NULL) return;

    QByteArray name = so->getName();
    if (!name.isEmpty())
    {
        QByteArray val = startupOptset->getValue(name);
        kDebug() << "Initialising" << name << "with value" << val;
        so->set(val);
        sane_device->apply(so);
    }
}



// TODO: this should take the TITLE and DESC from the backend (via KScanDevice
// and KScanOption), if available
QScrollArea *ScanParams::createScannerParams()
{
    KScanOption *so;

    QScrollArea *sv = new QScrollArea(this);
    sv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sv->setWidgetResizable(false);

    QWidget *frame = new QWidget(this);

    QGridLayout *lay = new QGridLayout(frame);
    lay->setSpacing(2*KDialog::spacingHint());
    lay->setColumnStretch(2,1);
    lay->setColumnMinimumWidth(1,KDialog::marginHint());

    int row = 0;
    QLabel *l;
    QWidget *w;

    virt_filename = sane_device->getGuiElement( SANE_NAME_FILE, frame,
                                                SANE_TITLE_FILE,
                                                SANE_DESC_FILE );
    if (virt_filename!=NULL)
    {
        initialise( virt_filename );
        connect( virt_filename, SIGNAL(guiChange(KScanOption *)),
                 SLOT(slotReloadAllGui(KScanOption *)));

        l = virt_filename->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = virt_filename->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;

        /* Selection for either virtual scanner or SANE debug */
        QGroupBox *vbg = new QGroupBox( i18n("Testing Mode"),frame);
        QVBoxLayout * vbgLayout = new QVBoxLayout();
            QRadioButton *rb1 = new QRadioButton(i18n("SANE Debug (from PNM image)"));
            vbgLayout->addWidget(rb1);

            QRadioButton *rb2 = new QRadioButton(i18n("Virtual Scanner (any image format)"));
            vbgLayout->addWidget(rb2);
        vbg->setLayout(vbgLayout);

        if (scan_mode==ID_SCAN)
            scan_mode = ID_SANE_DEBUG;
        rb1->setChecked(scan_mode==ID_SANE_DEBUG);
        rb2->setChecked(scan_mode==ID_QT_IMGIO);

        // needed for new 'buttonClicked' signal:
        QButtonGroup * vbgGroup = new QButtonGroup(vbg);
            vbgGroup->addButton(rb1, 0);
            vbgGroup->addButton(rb2, 1);
        connect(vbgGroup, SIGNAL(buttonClicked(int)), SLOT(slotVirtScanModeSelect(int)));

        lay->addWidget(vbg,row,2,1,-1);
        ++row;

        lay->addWidget(new KSeparator(Qt::Horizontal,frame),row,0,1,-1);
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

        cb->slotSetIcon(mIconLineart, i18n("Line art"));
        cb->slotSetIcon(mIconLineart, i18n("Lineart"));
        cb->slotSetIcon(mIconLineart, i18n("Binary"));
        cb->slotSetIcon(mIconGray, i18n("Gray"));
        cb->slotSetIcon(mIconGray, i18n("Grey"));
        cb->slotSetIcon(mIconColor, i18n("Color"));
        cb->slotSetIcon(mIconColor, i18n("Colour"));
        cb->slotSetIcon(mIconHalftone, i18n("Halftone"));

        initialise( so );
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 SLOT(slotReloadAllGui( KScanOption* )));
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 SLOT(slotNewScanMode()));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        lay->addWidget(cb,row,2,1,-1);
        l->setBuddy(cb);
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
        so->slotRedrawWidget( so );

        /* connect to slot that passes the resolution to the previewer */
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 this, SLOT( slotNewXResolution(KScanOption*)));
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2);
        l->setBuddy(w);
        lay->addWidget(new QLabel(i18n("dpi"),frame),row,3,Qt::AlignLeft);
							// SANE resolution always in DPI
        ++row;

        xy_resolution_bind = sane_device->getGuiElement(SANE_NAME_RESOLUTION_BIND, frame,
                                                        SANE_TITLE_RESOLUTION_BIND,
                                                        SANE_DESC_RESOLUTION_BIND );
        if (xy_resolution_bind!=NULL)
        {
            initialise( xy_resolution_bind );
            xy_resolution_bind->slotRedrawWidget( xy_resolution_bind );

            /* Connect to Gui-change-Slot */
            connect( xy_resolution_bind, SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slotReloadAllGui( KScanOption* )));

            l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
            w = so->widget(); lay->addWidget(w,row,2,1,-1);
            l->setBuddy(w);
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
            so->slotRedrawWidget( so );

            l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
            w = so->widget(); lay->addWidget(w,row,2);
            l->setBuddy(w);
            lay->addWidget(new QLabel(i18n("dpi"),frame),row,3,Qt::AlignLeft);
            ++row;
        }

        emit scanResolutionChanged(x_y_res,y_res);	// tell the previewer
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
            int x_y_res;
            so->get( &x_y_res );
            so->slotRedrawWidget( so );

            /* connect to slot that passes the resolution to the previewer */
            connect( so, SIGNAL(guiChange(KScanOption*)),
                     this, SLOT( slotNewXResolution(KScanOption*)));
            connect( so, SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slotReloadAllGui( KScanOption* )));

            l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
            w = so->widget(); lay->addWidget(w,row,2);
            l->setBuddy(w);
            lay->addWidget(new QLabel(i18n("dpi"),frame),row,3,Qt::AlignLeft);
            ++row;

            emit scanResolutionChanged(x_y_res,x_y_res);
        }
        else
        {
            kDebug() << "Serious: No Resolution setting available!";
        }
    }

    /* Scan size setting */
    area_sel = new ScanSizeSelector(frame,sane_device->getMaxScanSize());
    connect(area_sel,SIGNAL(sizeSelected(QRect)),SLOT(slotScanSizeSelected(QRect)));
    l = new QLabel("Scan &area:",frame);		// make sure it gets an accel
    lay->addWidget(l,row,0,Qt::AlignTop|Qt::AlignLeft);
    lay->addWidget(area_sel,row,2,1,-1,Qt::AlignTop);
    l->setBuddy(area_sel);
    ++row;

    /* Insert another beautification line */
    lay->addWidget(new KSeparator(Qt::Horizontal,frame),row,0,1,-1);
    ++row;

    /* Add a button for Source-Selection */
    
    source_sel = sane_device->getGuiElement( SANE_NAME_SCAN_SOURCE, frame,
                                             SANE_TITLE_SCAN_SOURCE,
                                             SANE_DESC_SCAN_SOURCE );
    if (source_sel!=NULL)
    {
        initialise( source_sel );
        connect( source_sel, SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = source_sel->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = source_sel->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;

        //  Will need to enable the "Advanced" dialogue later, because that
        //  contains other ADF options.  They are not implemented at the moment
        //  but they may be some day...
        //QPushButton *pb = new QPushButton( i18n("Source && ADF Options..."), frame);
        //connect(pb, SIGNAL(clicked()), SLOT(slotSourceSelect()));
        //lay->addWidget(pb,row,2,1,-1,Qt::AlignRight);
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
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }

    so = sane_device->getGuiElement( SANE_NAME_HALFTONE_DIMENSION, frame,
                                     SANE_TITLE_HALFTONE_DIMENSION,
                                     SANE_DESC_HALFTONE_DIMENSION );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }

    so = sane_device->getGuiElement( SANE_NAME_HALFTONE_PATTERN, frame,
                                     SANE_TITLE_HALFTONE_PATTERN,
                                     SANE_DESC_HALFTONE_PATTERN );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
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
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }

    so = sane_device->getGuiElement( SANE_NAME_BIT_DEPTH, frame,
                                     SANE_TITLE_BIT_DEPTH,
                                     SANE_DESC_BIT_DEPTH );
    if (so!=NULL)
    {
        kDebug() << "Bit-Depth option exists";
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 SLOT(slotNewScanMode()));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
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
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }

    so = sane_device->getGuiElement( SANE_NAME_HAND_SCANNER, frame,
                                     SANE_TITLE_HAND_SCANNER,
                                     SANE_DESC_HAND_SCANNER );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }

    so = sane_device->getGuiElement( SANE_NAME_GRAYIFY, frame,
                                     SANE_TITLE_GRAYIFY,
                                     SANE_DESC_GRAYIFY );
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }
#endif

// TODO: why do some of these not connect guiChange --> slotReloadAllGui?

    /* Speed-Setting - show only if active */
    KScanOption kso_speed( SANE_NAME_SCAN_SPEED );
    if( kso_speed.valid() && kso_speed.softwareSetable() && kso_speed.active())
    {
        so = sane_device->getGuiElement( SANE_NAME_SCAN_SPEED, frame,
                                         SANE_TITLE_SCAN_SPEED,
                                         SANE_DESC_SCAN_SPEED );
        initialise( so );

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }

    /* Threshold-Setting */
    so = sane_device->getGuiElement( SANE_NAME_THRESHOLD, frame,
                                     SANE_TITLE_THRESHOLD,
                                     SANE_DESC_THRESHOLD);
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }

    /* Brightness-Setting */
    so = sane_device->getGuiElement( SANE_NAME_BRIGHTNESS, frame,
                                     SANE_TITLE_BRIGHTNESS,
                                     SANE_DESC_BRIGHTNESS);
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }

    /* Contrast-Setting */
    so = sane_device->getGuiElement( SANE_NAME_CONTRAST, frame,
                                     SANE_TITLE_CONTRAST,
                                     SANE_DESC_CONTRAST );
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }

    /* Sharpness */
    so = sane_device->getGuiElement( "sharpness", frame );
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
        ++row;
    }


    /* The gamma table can be used - add a button for editing */
    if (sane_device->optionExists( SANE_NAME_CUSTOM_GAMMA ) )
    {
        KHBox *hb1 = new KHBox(frame);
        so = sane_device->getGuiElement( SANE_NAME_CUSTOM_GAMMA, hb1,
                                         SANE_TITLE_CUSTOM_GAMMA,
                                         SANE_DESC_CUSTOM_GAMMA );
        if (so!=NULL)
        {
            initialise( so );
            connect( so,   SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slotReloadAllGui( KScanOption* )));

            /* This connection cares for enabling/disabling the edit-Button */
            connect( so,   SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slotOptionNotify(KScanOption*)));
        }

        /* Connect a signal to refresh activity of the gamma tables */
        (void) new QWidget( hb1 ); /* dummy widget to eat space */

        pb_edit_gtable = new QPushButton( i18n("Edit..."), hb1 );

        connect( pb_edit_gtable, SIGNAL( clicked () ),
                 this, SLOT( slotEditCustGamma () ) );
        setEditCustomGammaTableState();

        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addWidget(hb1,row,2,1,-1);
        ++row;
    }
    /* my Epson Perfection backends offer a list of user defined gamma values */

    so = sane_device->getGuiElement( SANE_NAME_NEGATIVE, frame,
                                     SANE_TITLE_NEGATIVE,
                                     SANE_DESC_NEGATIVE );
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        l->setBuddy(w);
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
        lay->addWidget(so->widget(),row,2,1,-1);
        ++row;

        cb_gray_preview = static_cast<QCheckBox *>(so->widget());
        cb_gray_preview->setToolTip(i18n("Acquire a gray preview even in color mode (faster)"));
    }

#ifdef RESTORE_AREA
    initStartupArea();					// set up and tell previewer
#endif
    slotNewScanMode();					// tell previewer this too

    lay->addWidget(new QLabel("",frame),row,0,1,-1);
    lay->setRowStretch(row,1);				// dummy row for stretch

    frame->setMinimumWidth(frame->sizeHint().width());
    sv->setWidget(frame);
    sv->setMinimumWidth(frame->minimumWidth()+
                        frame->style()->pixelMetric(QStyle::PM_LayoutRightMargin)+
                        frame->style()->pixelMetric(QStyle::PM_ScrollBarExtent));
    return (sv);
}


void ScanParams::initStartupArea()
{
    if (startupOptset==NULL) return;
							// set scan area from saved
    KScanOption tl_x(SANE_NAME_SCAN_TL_X); initialise(&tl_x);
    KScanOption tl_y(SANE_NAME_SCAN_TL_Y); initialise(&tl_y);
    KScanOption br_x(SANE_NAME_SCAN_BR_X); initialise(&br_x);
    KScanOption br_y(SANE_NAME_SCAN_BR_Y); initialise(&br_y);

    QRect rect;
    int val1,val2;
    tl_x.get(&val1); rect.setLeft(val1);		// pass area to previewer
    br_x.get(&val2); rect.setWidth(val2-val1);
    tl_y.get(&val1); rect.setTop(val1);
    br_y.get(&val2); rect.setHeight(val2-val1);
    emit newCustomScanSize(rect);

    area_sel->selectSize(rect);				// set selector to match
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

    QLabel *lab = new QLabel(msg, this);
    lab->setWordWrap(true);
    lab->setOpenExternalLinks(true);
    QGridLayout *lay = dynamic_cast<QGridLayout *>(layout());
    if (lay!=NULL) lay->addWidget(lab, 0, 0, Qt::AlignTop);
}



/* This slot will be called if something changes with the option given as a param.
 * This is useful if the parameter - Gui has widgets in his own space, which depend
 * on widget controlled by the KScanOption.
 */
void ScanParams::slotOptionNotify(const KScanOption *so)
{
    if (so==NULL || !so->valid()) return;
    setEditCustomGammaTableState();
}


void ScanParams::slotSourceSelect()
{
   KScanOption so( SANE_NAME_SCAN_SOURCE );
   AdfBehaviour adf = ADF_OFF;

   const QByteArray &currSource = so.get();
   kDebug() << "Current source is" << currSource;
   QList<QByteArray> sources;

   if( so.valid() )
   {
      sources = so.getList();
#undef CHEAT_FOR_DEBUGGING
#ifdef CHEAT_FOR_DEBUGGING
      if( sources.find( "Automatic Document Feeder" ) == -1)
          sources.append( "Automatic Document Feeder" );
#endif

      ScanSourceDialog d( this, sources, adf );
      d.slotSetSource( currSource );

      if( d.exec() == QDialog::Accepted  )
      {
	 QString sel_source = d.getText();
	 adf = d.getAdfBehave();

	 /* set the selected Document source, the behavior is stored in a membervar */
	 so.set(sel_source.toLatin1());			// FIX in ScanSourceDialog, then here
	 sane_device->apply( &so );

         if (source_sel!=NULL)
         {
             source_sel->slotReload();
             source_sel->slotRedrawWidget(source_sel);
         }

	 kDebug() << "new source" << sel_source << "ADF" << adf;
      }
   }
}


/** Slot which is called if the user switches in the gui between
 *  the SANE-Debug-Mode and the qt image reading
 */
void ScanParams::slotVirtScanModeSelect(int id)
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
    kDebug() << "scan mode=" << scan_mode;

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
                KMessageBox::sorry(this,i18n("<qt>The testing or virtual file<br><filename>%1</filename><br>was not found or is not readable", virtfile));
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
                                                 "This file is type <b>%1</b>.", mimetype->name()));
                    stat = KSCAN_ERR_PARAM;
                }
            }
        }
    }

    if (vfp!=NULL) *vfp = virtfile;
    return (stat);
}



void ScanParams::startProgress()
{
    progressDialog->setValue(0);
    if (progressDialog->isHidden()) progressDialog->show();
}



/* Slot called to start acquiring a preview */
void ScanParams::slotAcquirePreview()
{
    if (scan_mode==ID_QT_IMGIO)
    {
        KMessageBox::sorry(this,i18n("Cannot preview in Virtual Scanner mode"));
        return;
    }

    QString virtfile;
    KScanStat stat = prepareScan(&virtfile);
    if (stat!=KSCAN_OK) return;

    kDebug() << "scan mode=" << scan_mode << "virtfile" << virtfile;

    bool gray_preview = false;
    if (cb_gray_preview!=NULL) gray_preview  = cb_gray_preview->isChecked();

    slotMaximalScanSize();				// always preview at maximal size
    area_sel->selectCustomSize(QRect());		// reset selector to reflect that

    startProgress();					// show the progress dialog
    stat = sane_device->acquirePreview(gray_preview);
    if (stat!=KSCAN_OK) kDebug() << "Error, preview status " << stat;
}


/* Slot called to start scanning */
void ScanParams::slotStartScan()
{
    QString virtfile;
    KScanStat stat = prepareScan(&virtfile);
    if (stat!=KSCAN_OK) return;

    kDebug() << "scan mode=" << scan_mode << "virtfile" << virtfile;

    if (scan_mode!=ID_QT_IMGIO)				// acquire via SANE
    {
        if (adf==ADF_OFF)
        {
	    kDebug() << "Start to acquire image";
            startProgress();				// show the progress dialog
	    stat = sane_device->acquire();
        }
        else
        {
	    kDebug() << "ADF Scan not yet implemented :-/";
	    // stat = performADFScan();
        }
    }
    else						// acquire via Qt-IMGIO
    {
        kDebug() << "Acquiring from virtual file";
        stat = sane_device->acquire(virtfile);
    }

    if (stat!=KSCAN_OK) kDebug() << "Error, scan status " << stat;
}



/* Slot called to start the Custom Gamma Table Edit dialog */

void ScanParams::slotEditCustGamma( void )
{
    kDebug();
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
	  kDebug() << "Gray Gamma Table is active ";
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
	     kDebug() << "Getting old gamma table from Red channel";
	  }
	  else
	  {
	     /* uh ! No current gammatable could be retrieved. Use the 100/0/0 gt
	      * created by KGammaTable's constructor. Nothing to do for that.
	      */
	     kDebug() << "Could not retrieve a gamma table";
	  }
       }
    }

    kDebug() << "Old gamma table:" << old_gt.getGamma() << old_gt.getBrightness() << old_gt.getContrast();

    GammaDialog gdiag( this );
    connect( &gdiag, SIGNAL( gammaToApply(KGammaTable*) ),
	     this,     SLOT( slotApplyGamma(const KGammaTable *) ) );

    gdiag.setGt( old_gt );

    if( gdiag.exec() == QDialog::Accepted  )
    {
       slotApplyGamma( gdiag.getGt() );
       kDebug() << "Applied new Gamma Table";
    }
    else
    {
       /* reset to old values */
       slotApplyGamma( &old_gt );
       kDebug() << "Cancelled, reverted to old Gamma Table";
    }

}


void ScanParams::slotApplyGamma(const KGammaTable *gt)
{
   if( ! gt ) return;

   kDebug(29000) << "Applying gamma table:" << gt->getGamma() << gt->getBrightness() << gt->getContrast();


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

void ScanParams::slotReloadAllGui( KScanOption* t)
{
    if( !t || ! sane_device ) return;
    kDebug() << "for widget" << t->getName();
    /* Need to reload all _except_ the one which was actually changed */

    sane_device->slotReloadAllBut( t );

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

   kDebug() << "State of edit custom gamma button:" << butState;
   pb_edit_gtable->setEnabled( butState );
}


void ScanParams::applyRect(const QRect &rect)
{
    kDebug() << "rect=" << rect;

    KScanOption tl_x(SANE_NAME_SCAN_TL_X);
    KScanOption tl_y(SANE_NAME_SCAN_TL_Y);
    KScanOption br_x(SANE_NAME_SCAN_BR_X);
    KScanOption br_y(SANE_NAME_SCAN_BR_Y);

    double min1,max1;
    double min2,max2;

    if (!rect.isValid())					// set full scan area
    {
        tl_x.getRange(&min1,&max1); tl_x.set(min1);
        br_x.getRange(&min1,&max1); br_x.set(max1);
        tl_y.getRange(&min2,&max2); tl_y.set(min2);
        br_y.getRange(&min2,&max2); br_y.set(max2);

        kDebug() << "setting full area" << min1 << min2 << "-" << max1 << max2;
    }
    else
    {
        double tlx = rect.left();
        double tly = rect.top();
        double brx = rect.right();
        double bry = rect.bottom();

        tl_x.getRange(&min1,&max1);
        if (tlx<min1)
        {
            brx += (min1-tlx);
            tlx = min1;
        }
        tl_x.set(tlx); br_x.set(brx);

        tl_y.getRange(&min2,&max2);
        if (tly<min2)
        {
            bry += (min2-tly);
            tly = min2;
        }
        tl_y.set(tly); br_y.set(bry);

        kDebug() << "setting area" << tlx << tly << "-" << brx << bry;
    }

    sane_device->apply(&tl_x);
    sane_device->apply(&tl_y);
    sane_device->apply(&br_x);
    sane_device->apply(&br_y);
}


//  The previewer is telling us that the user has drawn or auto-selected a
//  new preview area.
void ScanParams::slotNewPreviewRect(const QRect &rect)
{
    kDebug() << "rect=" << rect;

    applyRect(rect);
    area_sel->selectCustomSize(rect);
}


//  A new preset scan size or orientation chosen by the user
void ScanParams::slotScanSizeSelected(const QRect &rect)
{
    kDebug() << "rect=" << rect << "full=" << !rect.isValid();

    applyRect(rect);
    emit newCustomScanSize(rect);
}


/**
 * sets the scan area to the default, which is the whole area.
 */
void ScanParams::slotMaximalScanSize()
{
    kDebug() << "Setting to default";
    slotScanSizeSelected(QRect());
}


void ScanParams::slotNewXResolution(KScanOption *opt)
{
   if(! opt ) return;

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

void ScanParams::slotNewYResolution(KScanOption *opt)
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


void ScanParams::slotNewScanMode()
{
    int format = SANE_FRAME_RGB;
    int depth = 8;
    sane_device->getCurrentFormat(&format,&depth);

    int strips = (format==SANE_FRAME_GRAY ? 1 : 3);

    kDebug() << "format" << format << "depth" << depth << "-> strips " << strips;

    if (strips==1 && depth==1)				// bitmap scan
    {
        emit scanModeChanged(0);			// 8 pixels per byte
    }
    else
    {							// bytes per pixel
        emit scanModeChanged(strips*(depth==16 ? 2 : 1));
    }
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
