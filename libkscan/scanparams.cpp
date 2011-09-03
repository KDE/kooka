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

#include "scanparams_p.h"
#include "scanparams_p.moc"

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

#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kled.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <khbox.h>
#include <ktabwidget.h>

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
#include "kscanoption.h"
#include "kscanoptset.h"


//  Debugging options
#undef DEBUG_ADF


//  SANE testing options
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
    mVirtualFile = NULL;
    mGammaEditButt = NULL;
    mResolutionBind = NULL;
    mProgressDialog = NULL;
    mSourceSelect = NULL;
    mLed = NULL;

    mFirstGTEdit = true;

    mProblemMessage = NULL;
    mNoScannerMessage = NULL;

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
    mStartupOptions = NULL;
}


ScanParams::~ScanParams()
{
    kDebug();

    delete mStartupOptions;
    delete mProgressDialog;
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

    /* load the startup scanoptions */
    // TODO: check whether the saved scanner options apply to the current scanner?
    // They may be for a completely different one...
    // Or update KScanDevice and here to save/load the startup options
    // on a per-scanner basis.
    mStartupOptions = new KScanOptSet(DEFAULT_OPTIONSET);
    if (!mStartupOptions->loadConfig(mSaneDevice->scannerBackendName()))
    {
        kDebug() << "Could not load startup options";
        delete mStartupOptions;
        mStartupOptions = NULL;
    }

    /* Now create Widgets for the important scan settings */
    QWidget *sv = createScannerParams();
    lay->addWidget(sv, 3, 0, 1, 2);
    lay->setRowStretch(3, 9);

    /* Reload all options to care for inactive options */
    mSaneDevice->reloadAllOptions();

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
    mProgressDialog = new QProgressDialog(i18n("Scanning in progress"),
                                          i18n("Stop"), 0, 100, NULL);
    mProgressDialog->setModal(true);
    mProgressDialog->setAutoClose(true);
    mProgressDialog->setAutoReset(true);
    mProgressDialog->setMinimumDuration(100);
    mProgressDialog->setWindowTitle(i18n("Scanning"));

    connect(mProgressDialog, SIGNAL(canceled()), mSaneDevice, SLOT(slotStopScanning()));
    connect(mSaneDevice, SIGNAL(sigScanProgress(int)), SLOT(slotScanProgress(int)));

    return (true);
}


void ScanParams::initialise(KScanOption *so)
{
    if (so==NULL) return;
    if (mStartupOptions==NULL) return;
    if (!so->isReadable()) return;
    if (!so->isSoftwareSettable()) return;

    QByteArray name = so->getName();
    if (!name.isEmpty())
    {
        QByteArray val = mStartupOptions->getValue(name);
        kDebug() << "Initialising" << name << "with value" << val;
        so->set(val);
        so->apply();
    }
}


ScanParamsPage *ScanParams::createTab(KTabWidget *tw, const QString &title, const char *name)
{
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scroll->setWidgetResizable(true);			// stretch to width
    scroll->setFrameStyle(QFrame::NoFrame);

    ScanParamsPage *frame = new ScanParamsPage(this, name);
    scroll->setWidget(frame);
    tw->addTab(scroll, title);

    return (frame);
}


QWidget *ScanParams::createScannerParams()
{
    KTabWidget *tw = new KTabWidget(this);
    tw->setTabsClosable(false);
    tw->setTabPosition(QTabWidget::North);

    ScanParamsPage *basicFrame = createTab(tw, i18n("&Basic"), "BasicFrame");
    ScanParamsPage *otherFrame = createTab(tw, i18n("Other"), "OtherFrame");
    ScanParamsPage *advancedFrame = createTab(tw, i18n("Advanced"), "AdvancedFrame");

    KScanOption *so;
    QLabel *l;
    QWidget *w;
    QLabel *u;
    ScanParamsPage *frame;

    // Initial "Basic" options
    frame = basicFrame;

    // Virtual/debug image file
    mVirtualFile = mSaneDevice->getGuiElement(SANE_NAME_FILE, frame);
    if (mVirtualFile!=NULL)
    {
        initialise(mVirtualFile);
        connect(mVirtualFile, SIGNAL(guiChange(KScanOption *)), SLOT(slotOptionChanged(KScanOption *)));

        l = mVirtualFile->getLabel(frame, true);
        w = mVirtualFile->widget();
        frame->addRow(l, w);

        // Selection for either virtual scanner or SANE debug
        QGroupBox *vbg = new QGroupBox(i18n("Testing Mode"), frame);
        QVBoxLayout *vbgLayout = new QVBoxLayout();
        vbg->setLayout(vbgLayout);

        QRadioButton *rb1 = new QRadioButton(i18n("SANE Debug (from PNM image)"));
        vbgLayout->addWidget(rb1);
        QRadioButton *rb2 = new QRadioButton(i18n("Virtual Scanner (any image format)"));
        vbgLayout->addWidget(rb2);

        if (mScanMode==ScanParams::NormalMode) mScanMode = ScanParams::SaneDebugMode;
        rb1->setChecked(mScanMode==ScanParams::SaneDebugMode);
        rb2->setChecked(mScanMode==ScanParams::VirtualScannerMode);

        // needed for new 'buttonClicked' signal:
        QButtonGroup *vbgGroup = new QButtonGroup(vbg);
        vbgGroup->addButton(rb1, 0);
        vbgGroup->addButton(rb2, 1);
        connect(vbgGroup, SIGNAL(buttonClicked(int)), SLOT(slotVirtScanModeSelect(int)));

        frame->addRow(NULL, vbg);

        // Separator line after these.  Using a KScanGroup with a null text,
        // so that it looks the same as any real group separators following.
        frame->addGroup(new KScanGroup(frame, QString::null));
    }

    // Mode setting
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

        initialise(so);
        connect(so, SIGNAL(guiChange(KScanOption *)), SLOT(slotOptionChanged(KScanOption *)));
        connect(so, SIGNAL(guiChange(KScanOption *)), SLOT(slotNewScanMode()));

        l = so->getLabel(frame, true);
        frame->addRow(l, cb);
    }

    // Resolution setting.  Try "X-Resolution" setting first, this is the
    // option we want if the resolutions are split up.  If there is no such
    // option then try just "Resolution", this may not be the same as
    // "X-Resolution" even though this was the case in SANE<=1.0.19.
    so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_X_RESOLUTION, frame);
    if (so==NULL) so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_RESOLUTION, frame);
    if (so!=NULL)
    {
        initialise(so);

        int x_res;
        so->get(&x_res);
        so->redrawWidget();

        connect(so, SIGNAL(guiChange(KScanOption *)), SLOT(slotOptionChanged(KScanOption *)));
        // Connection that passes the resolution to the previewer
        connect(so, SIGNAL(guiChange(KScanOption *)), SLOT(slotNewResolution(KScanOption *)));

        l = so->getLabel(frame, true);
        w = so->widget();
        u = so->getUnit(frame);
        frame->addRow(l, w, u);

        // Same X/Y resolution option (if present)
        mResolutionBind = mSaneDevice->getGuiElement(SANE_NAME_RESOLUTION_BIND, frame);
        if (mResolutionBind!=NULL)
        {
            initialise(mResolutionBind);
            mResolutionBind->redrawWidget();

            connect(mResolutionBind, SIGNAL(guiChange(KScanOption *)), SLOT(slotOptionChanged(KScanOption *)));

            l = so->getLabel(frame, true);
            w = so->widget();
            frame->addRow(l, w);
        }

        // Now the "Y-Resolution" setting, if there is a separate one
        so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_Y_RESOLUTION, frame);
        int y_res = x_res;

        if (so!=NULL)
        {
            initialise(so);
            if (so->isActive()) so->get(&y_res);
            so->redrawWidget();

            // Connection that passes the resolution to the previewer
            connect(so, SIGNAL(guiChange(KScanOption *)), SLOT(slotNewResolution(KScanOption *)));

            l = so->getLabel(frame, true);
            w = so->widget();
            u = so->getUnit(frame);
            frame->addRow(l, w, u);
        }

        emit scanResolutionChanged(x_res, y_res);	// initialise the previewer
    }
    else
    {
        kDebug() << "Serious: No resolution option available!";
    }

    // Scan size setting
    area_sel = new ScanSizeSelector(frame, mSaneDevice->getMaxScanSize());
    connect(area_sel, SIGNAL(sizeSelected(const QRect &)), SLOT(slotScanSizeSelected(const QRect &)));
    l = new QLabel("Scan &area:", frame);		// make sure it gets an accel
    l->setBuddy(area_sel->focusProxy());
    frame->addRow(l, area_sel, NULL, Qt::AlignTop);

    // Insert another beautification line
    frame->addGroup(new KScanGroup(frame, QString::null));

    // Source selection
    mSourceSelect = mSaneDevice->getGuiElement(SANE_NAME_SCAN_SOURCE, frame);
    if (mSourceSelect!=NULL)
    {
        initialise(mSourceSelect);
        connect(mSourceSelect, SIGNAL(guiChange(KScanOption *)), SLOT(slotOptionChanged( KScanOption *)));

        l = mSourceSelect->getLabel(frame, true);
        w = mSourceSelect->widget();
        frame->addRow(l, w);

        //  TODO: enable the "Advanced" dialogue, because that
        //  contains other ADF options.  They are not implemented at the moment
        //  but they may be some day...
        //QPushButton *pb = new QPushButton( i18n("Source && ADF Options..."), frame);
        //connect(pb, SIGNAL(clicked()), SLOT(slotSourceSelect()));
        //lay->addWidget(pb,row,2,1,-1,Qt::AlignRight);
        //++row;
    }

    // SANE testing options, for the "test" device
    so = mSaneDevice->getGuiElement(SANE_NAME_TEST_PICTURE, frame);
    if (so!=NULL)
    {
        initialise(so);
        connect(so, SIGNAL(guiChange(KScanOption *)), SLOT(slotOptionChanged( KScanOption *)));

        l = so->getLabel(frame);
        w = so->widget();
        frame->addRow(l, w);
    }

    // Now all of the other options which have not been accounted for yet.
    // Split them up into "Other" and "Advanced".
    const QList<QByteArray> opts = mSaneDevice->getAllOptions();
    for (QList<QByteArray>::const_iterator it = opts.constBegin();
         it!=opts.constEnd(); ++it)
    {
        const QByteArray opt = (*it);

        if (opt==SANE_NAME_SCAN_TL_X ||			// ignore these (scan area)
            opt==SANE_NAME_SCAN_TL_Y ||
            opt==SANE_NAME_SCAN_BR_X ||
            opt==SANE_NAME_SCAN_BR_Y) continue;

        so = mSaneDevice->getExistingGuiElement(opt);	// see if already created
        if (so!=NULL) continue;				// if so ignore, don't duplicate

        so = mSaneDevice->getGuiElement(opt, frame);
        if (so!=NULL)
        {
            kDebug() << "creating" << (so->isCommonOption() ? "OTHER" : "ADVANCED") << "option" << opt;
            initialise(so);
            connect(so, SIGNAL(guiChange(KScanOption *)), SLOT(slotOptionChanged(KScanOption *)));

            if (so->isCommonOption()) frame = otherFrame;
            else frame = advancedFrame;

            w = so->widget();
            if (so->isGroup()) frame->addGroup(w);
            else
            {
                l = so->getLabel(frame);
                u = so->getUnit(frame);
                frame->addRow(l, w, u);
            }

            // Some special things to do for particular options
            if (opt==SANE_NAME_BIT_DEPTH)
            {
                connect(so, SIGNAL(guiChange(KScanOption *)), SLOT(slotNewScanMode()));
            }
            else if (opt==SANE_NAME_CUSTOM_GAMMA)
            {
                // Enabling/disabling the edit button is handled by
                // slotOptionChanged() calling setEditCustomGammaTableState()
                //connect(so, SIGNAL(guiChange(KScanOption *)), SLOT(slotOptionNotify(KScanOption *)));

                mGammaEditButt = new QPushButton(i18n("Edit Gamma Table..."), this);
                connect(mGammaEditButt, SIGNAL(clicked()), SLOT(slotEditCustGamma()));
                setEditCustomGammaTableState();

                frame->addRow(NULL, mGammaEditButt, NULL, Qt::AlignRight);
            }
        }
    }

    basicFrame->lastRow();				// final stretch row
    if (!otherFrame->lastRow()) tw->setTabEnabled(1, false);
    if (!advancedFrame->lastRow()) tw->setTabEnabled(2, false);

#ifdef RESTORE_AREA
    initStartupArea();					// set up and tell previewer
#endif
    slotNewScanMode();					// tell previewer this too

    return (tw);					// top-level (tab) widget
}


void ScanParams::initStartupArea()
{
    if (mStartupOptions==NULL) return;
							// set scan area from saved
    KScanOption *tl_x = mSaneDevice->getOption(SANE_NAME_SCAN_TL_X);
    KScanOption *tl_y = mSaneDevice->getOption(SANE_NAME_SCAN_TL_Y);
    KScanOption *br_x = mSaneDevice->getOption(SANE_NAME_SCAN_BR_X);
    KScanOption *br_y = mSaneDevice->getOption(SANE_NAME_SCAN_BR_Y);

    initialise(tl_x);
    initialise(tl_y);
    initialise(br_x);
    initialise(br_y);

    QRect rect;
    int val1,val2;
    tl_x->get(&val1); rect.setLeft(val1);		// pass area to previewer
    br_x->get(&val2); rect.setWidth(val2-val1);
    tl_y->get(&val1); rect.setTop(val1);
    br_y->get(&val2); rect.setHeight(val2-val1);
    emit newCustomScanSize(rect);

    area_sel->selectSize(rect);				// set selector to match
}


void ScanParams::createNoScannerMsg(bool galleryMode)
{
    QWidget *lab;
    if (galleryMode) lab = messageScannerNotSelected();
    else lab = messageScannerProblem();

    QGridLayout *lay = dynamic_cast<QGridLayout *>(layout());
    if (lay!=NULL) lay->addWidget(lab, 0, 0, Qt::AlignTop);
}


QWidget *ScanParams::messageScannerNotSelected()
{
    if (mNoScannerMessage==NULL)
    {
        mNoScannerMessage = new QLabel(
            i18n("<qt>"
                 "<b>No scanner selected</b>"
                 "<p>"
                 "Select a scanner device to perform scanning."));

        mNoScannerMessage->setWordWrap(true);
    }

    return (mNoScannerMessage);
}


QWidget *ScanParams::messageScannerProblem()
{
    if (mProblemMessage==NULL)
    {
        mProblemMessage = new QLabel(
            i18n("<qt>"
                 "<b>Problem: No scanner found, or unable to access it</b>"
                 "<p>"
                 "There was a problem using the SANE (Scanner Access Now Easy) library to access "
                 "the scanner device.  There may be a problem with your SANE installation, or it "
                 "may not be configured to support your scanner."
                 "<p>"
                 "Check that SANE is correctly installed and configured on your system, and "
                 "also that the scanner device name and settings are correct."
                 "<p>"
                 "See the SANE project home page "
                 "(<a href=\"http://www.sane-project.org\">www.sane-project.org</a>) "
                 "for more information on SANE installation and setup."));

        mProblemMessage->setWordWrap(true);
        mProblemMessage->setOpenExternalLinks(true);
    }

    return (mProblemMessage);
}


void ScanParams::slotSourceSelect()
{
    AdfBehaviour adf = ADF_OFF;

    if (mSourceSelect==NULL) return;			// no source selection GUI
    if (!mSourceSelect->isValid()) return;		// no option on scanner

    const QByteArray &currSource = mSourceSelect->get();
    kDebug() << "Current source is" << currSource;

    QList<QByteArray> sources = mSourceSelect->getList();
#ifdef DEBUG_ADF
    if (!sources.contains("Automatic Document Feeder"))
        sources.append("Automatic Document Feeder");
#endif

    // TODO: the 'sources' list has exactly the same options as the
    // scan source combo (apart from the debugging hack above), so
    // what's the point of repeating it in this dialogue?
    ScanSourceDialog d(this, sources, adf);
    d.slotSetSource(currSource);

    if (d.exec()!=QDialog::Accepted) return;

    QString sel_source = d.getText();
    adf = d.getAdfBehave();
    kDebug() << "new source" << sel_source << "ADF" << adf;

    /* set the selected Document source, the behavior is stored in a membervar */
    mSourceSelect->set(sel_source.toLatin1());		// TODO: FIX in ScanSourceDialog, then here
    mSourceSelect->apply();
    mSourceSelect->reload();
    mSourceSelect->redrawWidget();
}


/* Slot which is called if the user switches in the gui between
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
        if (mVirtualFile!=NULL) virtfile = mVirtualFile->get();
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


void ScanParams::slotScanProgress(int value)
{
    mProgressDialog->setValue(value);
    if (value==0 && !mProgressDialog->isVisible()) mProgressDialog->show();
}


/* Slot called to start acquiring a preview */
void ScanParams::slotAcquirePreview()
{

    // TODO: should be able to preview in Virtual Scanner mode, it just means
    // that the preview image will be the same size as the final image (which
    // doesn't matter).

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

#warning Check and port gamma table editing
// TODO: check handling of gamma tables
//
// Maybe need to have a GUI widget for them - a little preview of the
// gamma curve (a DispGamma) with an edit button.  Will avoid the special-case
// for the SANE_NAME_CUSTOM_GAMMA button followed by the edit button, and
// will allow separate editing of the R/G/B gamma tables if the scanner has them.
// Then the gamma will be initialised correctly along with all the others,
// avoiding the special case here.
#if 0
    /* Since gammatable options are not set in the default gui, it must be
     * checked if it is the first edit. If it is, take from loaded default
     * set if available there */
    if( mFirstGTEdit && mStartupOptions!=NULL)
    {
       mFirstGTEdit = false;
       QByteArray gtValue = mStartupOptions->getOption(SANE_NAME_GAMMA_VECTOR);
	  /* If it  not gray, it should be one color. */
       if (gtValue.isNull()) gt = mStartupOptions->getOption(SANE_NAME_GAMMA_VECTOR_R);
       if (!gtValue.isNull())
       {
	  gt->get( &old_gt );
    }
    else
    {
       /* it is not the first edit, use older values */
       if( mSaneDevice->optionExists( SANE_NAME_GAMMA_VECTOR ) )
       {
// TODO: should this and similar use KScanDevice::getExistingGuiElement()?
	  KScanOption grayGt( SANE_NAME_GAMMA_VECTOR, mSaneDevice );
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
	     KScanOption redGt( SANE_NAME_GAMMA_VECTOR_R, mSaneDevice);
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
#endif

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
    if (gt==NULL) return;

    kDebug() << "Applying gamma table:" << gt->getGamma() << gt->getBrightness() << gt->getContrast();

    KScanOption *so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR, false);
    if (so!=NULL && so->isActive())
    {
        so->set(gt);
        so->apply();
    }

    so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR_R, false);
    if (so!=NULL && so->isActive())
    {
        so->set(gt);
        so->apply();
    }

    so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR_G, false);
    if (so!=NULL && so->isActive())
    {
        so->set(gt);
        so->apply();
    }

    so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR_B, false);
    if (so!=NULL && so->isActive())
    {
        so->set(gt);
        so->apply();
    }
}


// The user has changed an option.  Apply that;  as a result of doing so,
// it may be necessary to reload every other scanner option apart from
// this one.

void ScanParams::slotOptionChanged(KScanOption *so)
{
    if (so==NULL || mSaneDevice==NULL) return;
    mSaneDevice->applyOption(so);

    // Update the gamma edit button state, if the option exists
    setEditCustomGammaTableState();
}


/*
 * enable editing of the gamma tables if one of the gamma tables
 * exists and is active at the moment
 */
void ScanParams::setEditCustomGammaTableState()
{
    if (mSaneDevice==NULL) return;
    if (mGammaEditButt==NULL) return;

    bool butState = false;

    // TODO: should this be SANE_NAME_GAMMA_VECTOR to match 
    // slotApplyGamma() above?
    KScanOption *so = mSaneDevice->getOption(SANE_NAME_CUSTOM_GAMMA, false);
    if (so!=NULL) butState = so->isActive();
    //kDebug() << "CustomGamma is active=" << butState;

    if (!butState)
    {
        KScanOption *so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR_R, false);
        if (so!=NULL) butState = so->isActive();
        //kDebug() << "CustomGamma Red is active=" << butState;
    }

    if (!butState)
    {
        KScanOption *so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR_G, false);
        if (so!=NULL) butState = so->isActive();
        //kDebug() << "CustomGamma Green is active=" << butState;
    }

    if (!butState)
    {
        KScanOption *so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR_B, false);
        if (so!=NULL) butState = so->isActive();
        //kDebug() << "CustomGamma blue is active=" << butState;
    }

    kDebug() << "State of edit custom gamma button=" << butState;
    mGammaEditButt->setEnabled(butState);
}


void ScanParams::applyRect(const QRect &rect)
{
    kDebug() << "rect=" << rect;

    KScanOption *tl_x = mSaneDevice->getOption(SANE_NAME_SCAN_TL_X);
    KScanOption *tl_y = mSaneDevice->getOption(SANE_NAME_SCAN_TL_Y);
    KScanOption *br_x = mSaneDevice->getOption(SANE_NAME_SCAN_BR_X);
    KScanOption *br_y = mSaneDevice->getOption(SANE_NAME_SCAN_BR_Y);

    double min1,max1;
    double min2,max2;

    if (!rect.isValid())				// set full scan area
    {
        tl_x->getRange(&min1, &max1); tl_x->set(min1);
        br_x->getRange(&min1, &max1); br_x->set(max1);
        tl_y->getRange(&min2, &max2); tl_y->set(min2);
        br_y->getRange(&min2, &max2); br_y->set(max2);

        kDebug() << "setting full area" << min1 << min2 << "-" << max1 << max2;
    }
    else
    {
        double tlx = rect.left();
        double tly = rect.top();
        double brx = rect.right();
        double bry = rect.bottom();

        tl_x->getRange(&min1, &max1);
        if (tlx<min1)
        {
            brx += (min1-tlx);
            tlx = min1;
        }
        tl_x->set(tlx); br_x->set(brx);

        tl_y->getRange(&min2,&max2);
        if (tly<min2)
        {
            bry += (min2-tly);
            tly = min2;
        }
        tl_y->set(tly); br_y->set(bry);

        kDebug() << "setting area" << tlx << tly << "-" << brx << bry;
    }

    tl_x->apply();
    tl_y->apply();
    br_x->apply();
    br_y->apply();
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


/*
 * sets the scan area to the default, which is the whole area.
 */
void ScanParams::setMaximalScanSize()
{
    kDebug() << "Setting to default";
    slotScanSizeSelected(QRect());
}


void ScanParams::slotNewResolution(KScanOption *opt)
{
    KScanOption *opt_x = mSaneDevice->getExistingGuiElement(SANE_NAME_SCAN_X_RESOLUTION);
    if (opt_x==NULL) opt_x = mSaneDevice->getExistingGuiElement(SANE_NAME_SCAN_RESOLUTION);
    KScanOption *opt_y = mSaneDevice->getExistingGuiElement(SANE_NAME_SCAN_Y_RESOLUTION);

    int x_res = 0;                                      // get the X resolution
    if (opt_x!=NULL && opt_x->isValid()) opt_x->get(&x_res);

    int y_res = 0;                                      // get the Y resolution
    if (opt_y!=NULL && opt_y->isValid()) opt_y->get(&y_res);

    kDebug() << "X/Y resolution" << x_res << y_res;

    if (y_res==0) y_res = x_res;                        // use X res if Y unavailable
    if (x_res==0) x_res = y_res;                        // unlikely, but orthogonal

    if (x_res==0 && y_res==0) kDebug() << "resolution not available!";
    else emit scanResolutionChanged(x_res, y_res);
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
