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

#include "scanglobal.h"
#include "scansourcedialog.h"
#include "massscandialog.h"
#include "gammadialog.h"
#include "kgammatable.h"
#include "kscancontrols.h"
#include "scansizeselector.h"
#include "kscanoptset.h"


//  SANE testing options
#define TESTING_OPTIONS

#ifndef SANE_NAME_TEST_PICTURE
#define SANE_NAME_TEST_PICTURE		"test-picture"
#endif
#ifndef SANE_NAME_THREE_PASS
#define SANE_NAME_THREE_PASS		"three-pass"
#endif
#ifndef SANE_NAME_HAND_SCANNER
#define SANE_NAME_HAND_SCANNER		"hand-scanner"
#endif
#ifndef SANE_NAME_GRAYIFY
#define SANE_NAME_GRAYIFY		"grayify"
#endif


ScanParams::ScanParams(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("ScanParams");

    mSaneDevice = NULL;
    virt_filename = NULL;
    pb_edit_gtable = NULL;
    xy_resolution_bind = NULL;
    mProgressDialog = NULL;
    source_sel = NULL;

    mFirstGTEdit = true;

    // Preload the scan mode icons.  The ones from libksane, which will be
    // available if kdegraphics is installed, look better than our rather
    // ancient ones, so use those if possible.  Set canReturnNull to true
    // when looking up those, so the returned path will be empty if they
    // are not installed.

    KIconLoader::global()->addAppDir("libkscan");	// access to our icons

    QString ip = KIconLoader::global()->iconPath("color", KIconLoader::Small, true);
    if (ip.isEmpty()) ip = KIconLoader::global()->iconPath("palette-color", KIconLoader::Small);
    mIconColor = QIcon(ip);

    ip = KIconLoader::global()->iconPath("gray-scale", KIconLoader::Small, true);
    if (ip.isEmpty()) ip = KIconLoader::global()->iconPath("palette-gray", KIconLoader::Small);
    mIconGray = QIcon(ip);

    ip = KIconLoader::global()->iconPath("black-white", KIconLoader::Small, true);
    if (ip.isEmpty()) ip = KIconLoader::global()->iconPath("palette-lineart", KIconLoader::Small);
    mIconLineart = QIcon(ip);

    ip = KIconLoader::global()->iconPath("halftone", KIconLoader::Small, true);
    if (ip.isEmpty()) ip = KIconLoader::global()->iconPath("palette-halftone", KIconLoader::Small);
    mIconHalftone = QIcon(ip);

    /* intialise the default last save warnings */
    startupOptset = NULL;
}


bool ScanParams::connectDevice(KScanDevice *newScanDevice, bool galleryMode)
{
    QGridLayout *lay = new QGridLayout(this);
    lay->setMargin(0);
    lay->setColumnStretch(0,9);

    if (newScanDevice==NULL)				// no scanner device
    {
        kDebug() << "No scan device, gallery=" << galleryMode;
        mSaneDevice = NULL;
        createNoScannerMsg(galleryMode);
        return (true);
    }

    mSaneDevice = newScanDevice;
    mScanMode = ScanParams::NormalMode;
    adf = ADF_OFF;

    QLabel *lab = new QLabel(i18n("<qt><b>Scanner&nbsp;Settings</b>"),this);
    lay->addWidget(lab,0,0,Qt::AlignLeft);

    mLed = new KLed(this);
    mLed->setState( KLed::Off );
    mLed->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ));
    lay->addWidget(mLed,0,1,Qt::AlignRight);

    lab = new QLabel(mSaneDevice->scannerDescription(),this);
    lay->addWidget(lab,1,0,1,2,Qt::AlignLeft);

    lay->addWidget(new KSeparator(Qt::Horizontal,this),2,0,1,-1);

    /* load the startup scanoptions */
    // TODO: check whether the saved scanner options apply to the current scanner?
    // They may be for a completely different one...
    // Or update KScanDevice to save the startup options on a per-scanner basis.
    startupOptset = new KScanOptSet(DEFAULT_OPTIONSET);
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
    mSaneDevice->slotReloadAll();

    lay->addWidget(new KSeparator(Qt::Horizontal,this),4,0,1,-1);

    /* Create the Scan Buttons */
    QPushButton *pb = new QPushButton(KIcon("preview"), i18n("Pre&view"), this);
    pb->setMinimumWidth(100);
    connect(pb, SIGNAL(clicked()), SLOT(slotAcquirePreview()));
    lay->addWidget(pb, 5, 0, Qt::AlignLeft);

    pb = new QPushButton(KIcon("scan"), i18n("Star&t Scan"), this);
    pb->setMinimumWidth(100);
    connect(pb, SIGNAL(clicked()), SLOT(slotStartScan()));
    lay->addWidget(pb, 5, 1, Qt::AlignRight);

    /* Initialise the progress dialog */
    mProgressDialog = new QProgressDialog( i18n("Scanning in progress"),
                                          i18n("Stop"), 0, 100, NULL);
    mProgressDialog->setModal(true);
    mProgressDialog->setAutoClose(true);
    mProgressDialog->setAutoReset(true);
    mProgressDialog->setMinimumDuration(100);
    mProgressDialog->setWindowTitle(i18n("Scanning"));

    connect( mSaneDevice, SIGNAL(sigScanProgress(int)),
             mProgressDialog, SLOT(setValue(int)));

    /* Connect the Progress Dialogs cancel-Button */
    connect( mProgressDialog, SIGNAL( canceled() ), mSaneDevice,
             SLOT( slotStopScanning() ) );

    return (true);
}


ScanParams::~ScanParams()
{
    kDebug();

    delete startupOptset;
    delete mProgressDialog;
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
        mSaneDevice->apply(so);
    }
}



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

    virt_filename = mSaneDevice->getGuiElement(SANE_NAME_FILE, frame);
    if (virt_filename!=NULL)
    {
        initialise( virt_filename );
        connect( virt_filename, SIGNAL(guiChange(KScanOption *)),
                 SLOT(slotReloadAllGui(KScanOption *)));

        l = virt_filename->getLabel(frame, true); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = virt_filename->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;

        /* Selection for either virtual scanner or SANE debug */
        QGroupBox *vbg = new QGroupBox( i18n("Testing Mode"),frame);
        QVBoxLayout * vbgLayout = new QVBoxLayout();
            QRadioButton *rb1 = new QRadioButton(i18n("SANE Debug (from PNM image)"));
            vbgLayout->addWidget(rb1);

            QRadioButton *rb2 = new QRadioButton(i18n("Virtual Scanner (any image format)"));
            vbgLayout->addWidget(rb2);
        vbg->setLayout(vbgLayout);

        if (mScanMode==ScanParams::NormalMode) mScanMode = ScanParams::SaneDebugMode;
        rb1->setChecked(mScanMode==ScanParams::SaneDebugMode);
        rb2->setChecked(mScanMode==ScanParams::VirtualScannerMode);

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
    so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_MODE, frame);
    if (so!=NULL)
    {
        KScanCombo *cb = (KScanCombo *) so->widget();

        // Having loaded the 'sane-backends' message catalogue, these strings
        // are now translatable.  But we also have to set the untranslated strings
        // just in case there is no translation there (sane-backends does not have
        // translations for the en_GB locale, for example).

        cb->setIcon(mIconLineart, I18N_NOOP("Line art"));
        cb->setIcon(mIconLineart, I18N_NOOP("Lineart"));
        cb->setIcon(mIconLineart, I18N_NOOP("Binary"));
        cb->setIcon(mIconGray, I18N_NOOP("Gray"));
        cb->setIcon(mIconGray, I18N_NOOP("Grey"));
        cb->setIcon(mIconColor, I18N_NOOP("Color"));
        cb->setIcon(mIconColor, I18N_NOOP("Colour"));
        cb->setIcon(mIconHalftone, I18N_NOOP("Halftone"));

        initialise( so );
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 SLOT(slotReloadAllGui( KScanOption* )));
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 SLOT(slotNewScanMode()));

        l = so->getLabel(frame, true); lay->addWidget(l,row,0,Qt::AlignLeft);
        lay->addWidget(cb,row,2,1,-1);
        ++row;
    }

    /* Resolution Setting -> try X-Resolution Setting */
    so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_X_RESOLUTION, frame,
                                    i18n("Resolution"));
    if (so!=NULL)
    {
        initialise( so );
        int x_y_res;
        so->get( &x_y_res );
        so->redrawWidget();

        /* connect to slot that passes the resolution to the previewer */
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 this, SLOT( slotNewXResolution(KScanOption*)));
        connect( so, SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame, true); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2);
        lay->addWidget(new QLabel(i18n("dpi"),frame),row,3,Qt::AlignLeft);
							// SANE resolution always in DPI
        ++row;

        xy_resolution_bind = mSaneDevice->getGuiElement(SANE_NAME_RESOLUTION_BIND, frame);
        if (xy_resolution_bind!=NULL)
        {
            initialise( xy_resolution_bind );
            xy_resolution_bind->redrawWidget();

            /* Connect to Gui-change-Slot */
            connect( xy_resolution_bind, SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slotReloadAllGui( KScanOption* )));

            l = so->getLabel(frame, true); lay->addWidget(l,row,0,Qt::AlignLeft);
            w = so->widget(); lay->addWidget(w,row,2,1,-1);
            ++row;
        }

        /* Resolution Setting -> Y-Resolution Setting */
        so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_Y_RESOLUTION, frame);
        int y_res = x_y_res;
        if (so!=NULL)
        {
            initialise( so );
            if( so->active() )
                so->get( &y_res );
            so->redrawWidget();

            l = so->getLabel(frame, true); lay->addWidget(l,row,0,Qt::AlignLeft);
            w = so->widget(); lay->addWidget(w,row,2);
            lay->addWidget(new QLabel(i18n("dpi"),frame),row,3,Qt::AlignLeft);
            ++row;
        }

        emit scanResolutionChanged(x_y_res,y_res);	// tell the previewer
    }
    else
    {
        /* If the SCAN_X_RES does not exists, perhaps just SCAN_RES does */
        so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_RESOLUTION, frame);
        if (so!=NULL)
        {
            initialise( so );
            int x_y_res;
            so->get( &x_y_res );
            so->redrawWidget();

            /* connect to slot that passes the resolution to the previewer */
            connect( so, SIGNAL(guiChange(KScanOption*)),
                     this, SLOT( slotNewXResolution(KScanOption*)));
            connect( so, SIGNAL(guiChange(KScanOption*)),
                     this, SLOT(slotReloadAllGui( KScanOption* )));

            l = so->getLabel(frame, true); lay->addWidget(l,row,0,Qt::AlignLeft);
            w = so->widget(); lay->addWidget(w,row,2);
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
    area_sel = new ScanSizeSelector(frame, mSaneDevice->getMaxScanSize());
    connect(area_sel,SIGNAL(sizeSelected(const QRect &)),SLOT(slotScanSizeSelected(const QRect &)));
    l = new QLabel("Scan &area:", frame);		// make sure it gets an accel
    lay->addWidget(l,row,0,Qt::AlignTop|Qt::AlignLeft);
    lay->addWidget(area_sel,row,2,1,-1,Qt::AlignTop);
    l->setBuddy(area_sel->focusProxy());
    ++row;

    /* Insert another beautification line */
    lay->addWidget(new KSeparator(Qt::Horizontal,frame),row,0,1,-1);
    ++row;

    /* Add a button for Source-Selection */
    
    source_sel = mSaneDevice->getGuiElement(SANE_NAME_SCAN_SOURCE, frame);
    if (source_sel!=NULL)
    {
        initialise( source_sel );
        connect( source_sel, SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = source_sel->getLabel(frame, true); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = source_sel->widget(); lay->addWidget(w,row,2,1,-1);
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
    so = mSaneDevice->getGuiElement(SANE_NAME_HALFTONE, frame);
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    so = mSaneDevice->getGuiElement(SANE_NAME_HALFTONE_DIMENSION, frame);
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    so = mSaneDevice->getGuiElement(SANE_NAME_HALFTONE_PATTERN, frame);
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    // SANE "test" options
    so = mSaneDevice->getGuiElement(SANE_NAME_TEST_PICTURE, frame);
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    so = mSaneDevice->getGuiElement(SANE_NAME_BIT_DEPTH, frame);
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
        ++row;
    }

#ifdef TESTING_OPTIONS
    so = mSaneDevice->getGuiElement(SANE_NAME_THREE_PASS, frame);
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    so = mSaneDevice->getGuiElement(SANE_NAME_HAND_SCANNER, frame);
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    so = mSaneDevice->getGuiElement(SANE_NAME_GRAYIFY, frame);
    if (so!=NULL)
    {
        initialise(so);
        connect( so,   SIGNAL(guiChange(KScanOption*)),
                 this, SLOT(slotReloadAllGui( KScanOption* )));

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }
#endif

// TODO: why do some of these not connect guiChange --> slotReloadAllGui?

    /* Speed-Setting - show only if active */
    KScanOption kso_speed( SANE_NAME_SCAN_SPEED );
    if( kso_speed.valid() && kso_speed.softwareSetable() && kso_speed.active())
    {
        so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_SPEED, frame);
        initialise( so );

        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    /* Threshold-Setting */
    so = mSaneDevice->getGuiElement(SANE_NAME_THRESHOLD, frame);
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    /* Brightness-Setting */
    so = mSaneDevice->getGuiElement(SANE_NAME_BRIGHTNESS, frame);
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    /* Contrast-Setting */
    so = mSaneDevice->getGuiElement(SANE_NAME_CONTRAST, frame);
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

// TODO: is there a SANE name?
    /* Sharpness */
    so = mSaneDevice->getGuiElement( "sharpness", frame );
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    /* The gamma table can be used - add a button for editing */
    if (mSaneDevice->optionExists(SANE_NAME_CUSTOM_GAMMA))
    {
        KHBox *hb1 = new KHBox(frame);
        so = mSaneDevice->getGuiElement(SANE_NAME_CUSTOM_GAMMA, hb1);
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

    so = mSaneDevice->getGuiElement(SANE_NAME_NEGATIVE, frame);
    if (so!=NULL)
    {
        initialise( so );
        l = so->getLabel(frame); lay->addWidget(l,row,0,Qt::AlignLeft);
        w = so->widget(); lay->addWidget(w,row,2,1,-1);
        ++row;
    }

    /* Preview-Switch */
    so = mSaneDevice->getGuiElement(SANE_NAME_GRAY_PREVIEW, frame);
    if (so!=NULL)
    {
        initialise( so );
        lay->addWidget(so->getLabel(frame),row,0,Qt::AlignLeft);
        lay->addWidget(so->widget(),row,2,1,-1);
        ++row;
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


// TODO: this is Kooka-specific, add a setNoScannerMessage() or make it protected
// so that an application's class can override it
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


void ScanParams::slotOptionNotify(KScanOption *so)
{
    if (so==NULL || !so->valid()) return;
    setEditCustomGammaTableState();
}


void ScanParams::slotSourceSelect()
{
// TODO: should this amd similar use KScanDevice::getExistingGuiElement()?
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
	 so.set(sel_source.toLatin1());			// TODO: FIX in ScanSourceDialog, then here
	 mSaneDevice->apply( &so );

         if (source_sel!=NULL)
         {
             source_sel->reload();
             source_sel->redrawWidget();
         }

	 kDebug() << "new source" << sel_source << "ADF" << adf;
      }
   }
}


/** Slot which is called if the user switches in the gui between
 *  the SANE-Debug-Mode and the qt image reading
 */
void ScanParams::slotVirtScanModeSelect(int but)
{
    ScanParams::ScanMode mode = static_cast<ScanParams::ScanMode>(but);

    if (mode==ScanParams::SaneDebugMode)		// SANE Debug
    {
        mScanMode = mode;
        mSaneDevice->guiSetEnabled( SANE_NAME_HAND_SCANNER, true );
        mSaneDevice->guiSetEnabled( SANE_NAME_THREE_PASS, true );
        mSaneDevice->guiSetEnabled( SANE_NAME_GRAYIFY, true );
        mSaneDevice->guiSetEnabled( SANE_NAME_CONTRAST, true );
        mSaneDevice->guiSetEnabled( SANE_NAME_BRIGHTNESS, true );
    }
    else						// Virtual Scanner
    {
        mScanMode = ScanParams::VirtualScannerMode;
        mSaneDevice->guiSetEnabled( SANE_NAME_HAND_SCANNER, false );
        mSaneDevice->guiSetEnabled( SANE_NAME_THREE_PASS, false );
        mSaneDevice->guiSetEnabled( SANE_NAME_GRAYIFY, false );
        mSaneDevice->guiSetEnabled( SANE_NAME_CONTRAST, false );
        mSaneDevice->guiSetEnabled( SANE_NAME_BRIGHTNESS, false );
    }
}



KScanDevice::Status ScanParams::prepareScan(QString *vfp)
{
    kDebug() << "scan mode=" << mScanMode;

    KScanDevice::Status stat = KScanDevice::Ok;
    QString virtfile;

    if (mScanMode==ScanParams::SaneDebugMode || mScanMode==ScanParams::VirtualScannerMode)
    {
        if (virt_filename!=NULL) virtfile = virt_filename->get();
        if (virtfile.isEmpty())
        {
            KMessageBox::sorry(this,i18n("A file must be entered for testing or virtual scanning"));
            stat = KScanDevice::ParamError;
        }

        if (stat==KScanDevice::Ok)
        {
            QFileInfo fi(virtfile);
            if (!fi.exists())
            {
                KMessageBox::sorry(this,i18n("<qt>The testing or virtual file<br><filename>%1</filename><br>was not found or is not readable", virtfile));
                stat = KScanDevice::ParamError;
            }
        }

        if (stat==KScanDevice::Ok)
        {
            if (mScanMode==ScanParams::SaneDebugMode)
            {
                KMimeType::Ptr mimetype = KMimeType::findByPath(virtfile);
                if (!(mimetype->is("image/x-portable-bitmap") ||
                      mimetype->is("image/x-portable-greymap") ||
                      mimetype->is("image/x-portable-pixmap")))
                {
                    KMessageBox::sorry(this,i18n("<qt>SANE Debug can only read PNM files.<br>"
                                                 "This file is type <b>%1</b>.", mimetype->name()));
                    stat = KScanDevice::ParamError;
                }
            }
        }
    }

    if (vfp!=NULL) *vfp = virtfile;
    return (stat);
}



void ScanParams::startProgress()
{
    mProgressDialog->setValue(0);
    if (mProgressDialog->isHidden()) mProgressDialog->show();
}



/* Slot called to start acquiring a preview */
void ScanParams::slotAcquirePreview()
{
    if (mScanMode==ScanParams::VirtualScannerMode)
    {
        KMessageBox::sorry(this,i18n("Cannot preview in Virtual Scanner mode"));
        return;
    }

    QString virtfile;
    KScanDevice::Status stat = prepareScan(&virtfile);
    if (stat!=KScanDevice::Ok) return;

    kDebug() << "scan mode=" << mScanMode << "virtfile" << virtfile;

    KScanOption *greyPreview = mSaneDevice->getExistingGuiElement(SANE_NAME_GRAY_PREVIEW);
    int gp = 0;
    if (greyPreview!=NULL) greyPreview->get(&gp);

    setMaximalScanSize();				// always preview at maximal size
    area_sel->selectCustomSize(QRect());		// reset selector to reflect that

    startProgress();					// show the progress dialog
    stat = mSaneDevice->acquirePreview(gp);
    if (stat!=KScanDevice::Ok) kDebug() << "Error, preview status " << stat;
}


/* Slot called to start scanning */
void ScanParams::slotStartScan()
{
    QString virtfile;
    KScanDevice::Status stat = prepareScan(&virtfile);
    if (stat!=KScanDevice::Ok) return;

    kDebug() << "scan mode=" << mScanMode << "virtfile" << virtfile;

    if (mScanMode!=ScanParams::VirtualScannerMode)	// acquire via SANE
    {
        if (adf==ADF_OFF)
        {
	    kDebug() << "Start to acquire image";
            startProgress();				// show the progress dialog
	    stat = mSaneDevice->acquireScan();
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
        stat = mSaneDevice->acquireScan(virtfile);
    }

    if (stat!=KScanDevice::Ok) kDebug() << "Error, scan status " << stat;
}



/* Slot called to start the Custom Gamma Table Edit dialog */

void ScanParams::slotEditCustGamma( void )
{
    kDebug();
    KGammaTable old_gt;


    /* Since gammatable options are not set in the default gui, it must be
     * checked if it is the first edit. If it is, take from loaded default
     * set if available there */
    if( mFirstGTEdit && startupOptset )
    {
       mFirstGTEdit = false;
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
       if( mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR ) )
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
	  if( mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR_R ))
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


   if( mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR ) )
   {
      KScanOption grayGt( SANE_NAME_GAMMA_VECTOR );

      /* Now find out, which gamma-Tables are active. */
      if( grayGt.active() )
      {
	 grayGt.set( gt );
	 mSaneDevice->apply( &grayGt, true );
      }
   }

   if( mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR_R )) {
      KScanOption rGt( SANE_NAME_GAMMA_VECTOR_R );
      if( rGt.active() )
      {
	 rGt.set( gt );
	 mSaneDevice->apply( &rGt, true );
      }
   }

   if( mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR_G )) {
      KScanOption gGt( SANE_NAME_GAMMA_VECTOR_G );
      if( gGt.active() )
      {
	 gGt.set( gt );
	 mSaneDevice->apply( &gGt, true );
      }
   }

   if( mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR_B )) {
      KScanOption bGt( SANE_NAME_GAMMA_VECTOR_B );
      if( bGt.active() )
      {
	 bGt.set( gt );
	 mSaneDevice->apply( &bGt, true );
      }
   }
}

/* Slot calls if a widget changes. Things to do:
 * - Apply the option and reload all if the option affects all
 */

void ScanParams::slotReloadAllGui( KScanOption* t)
{
    if( !t || ! mSaneDevice ) return;
    kDebug() << "for widget" << t->getName();
    /* Need to reload all _except_ the one which was actually changed */

    mSaneDevice->slotReloadAllBut( t );

    /* Custom Gamma <- What happens if that does not exist for some scanner ? TODO */
    setEditCustomGammaTableState();
}

/*
 * enable editing of the gamma tables if one of the gamma tables
 * exists and is active at the moment
 */
void ScanParams::setEditCustomGammaTableState()
{
   if( !(mSaneDevice && pb_edit_gtable) )
      return;

   bool butState = false;

   if( mSaneDevice->optionExists( SANE_NAME_CUSTOM_GAMMA ) )
   {
      KScanOption kso( SANE_NAME_CUSTOM_GAMMA );
      butState = kso.active();
      // kdDebug(29000) << "CustomGamma is active: " << butState << endl;
   }

   if( !butState && mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR_R ) )
   {
      KScanOption kso( SANE_NAME_GAMMA_VECTOR_R );
      butState = kso.active();
      // kdDebug(29000) << "CustomGamma Red is active: " << butState << endl;
   }

   if( !butState && mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR_G ) )
   {
      KScanOption kso( SANE_NAME_GAMMA_VECTOR_G );
      butState = kso.active();
      // kdDebug(29000) << "CustomGamma Green is active: " << butState << endl;
   }

   if( !butState && mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR_B ) )
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

    mSaneDevice->apply(&tl_x);
    mSaneDevice->apply(&tl_y);
    mSaneDevice->apply(&br_x);
    mSaneDevice->apply(&br_y);
}


//  The previewer is telling us that the user has drawn or auto-selected a
//  new preview area.
void ScanParams::slotNewPreviewRect(const QRect &rect)
{
    kDebug() << "rect=" << rect;

    applyRect(rect);
    area_sel->selectSize(rect);
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
void ScanParams::setMaximalScanSize()
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
    mSaneDevice->getCurrentFormat(&format,&depth);

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


KScanDevice::Status ScanParams::performADFScan( void )
{
   KScanDevice::Status stat = KScanDevice::Ok;
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
