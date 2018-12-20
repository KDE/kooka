/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include "scanparams.h"
#include "scanparams_p.h"

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
#include <qtabwidget.h>
#include <qdebug.h>
#include <qmimetype.h>
#include <qmimedatabase.h>
#include <qlayout.h>
#include <qdesktopservices.h>

#include <klocalizedstring.h>
#include <kled.h>
#include <kmessagebox.h>
#include <kmessagewidget.h>

extern "C"
{
#include <sane/saneopts.h>
}

#include "scanglobal.h"
//#include "scansourcedialog.h"
//#include "massscandialog.h"
#include "gammadialog.h"
#include "kgammatable.h"
#include "kscancontrols.h"
#include "scansizeselector.h"
#include "kscanoption.h"
#include "kscanoptset.h"
#include "scanicons.h"
#include "dialogbase.h"

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

ScanParamsPage::ScanParamsPage(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setObjectName(name);

    mLayout = new QGridLayout(this);
    mLayout->setSpacing(2*DialogBase::verticalSpacing());
    mLayout->setColumnStretch(2, 1);
    mLayout->setColumnMinimumWidth(1, 2*DialogBase::horizontalSpacing());

    mNextRow = 0;
    mPendingGroup = nullptr;
}

ScanParamsPage::~ScanParamsPage()
{
}

void ScanParamsPage::checkPendingGroup()
{
    if (mPendingGroup != nullptr) {            // separator to add first?
        QWidget *w = mPendingGroup;
        mPendingGroup = nullptr;               // avoid recursion!
        addRow(w);
    }
}

void ScanParamsPage::addRow(QWidget *wid)
{
    if (wid == nullptr) {
        return;    // must have one
    }

    checkPendingGroup();                // add separator if needed
    mLayout->addWidget(wid, mNextRow, 0, 1, -1);
    ++mNextRow;
}

void ScanParamsPage::addRow(QLabel *lab, QWidget *wid, QLabel *unit, Qt::Alignment align)
{
    if (wid == nullptr) {
        return;    // must have one
    }
    wid->setMaximumWidth(MAX_CONTROL_WIDTH);

    checkPendingGroup();                // add separator if needed

    if (lab != nullptr) {
        lab->setMaximumWidth(MAX_LABEL_WIDTH);
        lab->setMinimumWidth(MIN_LABEL_WIDTH);
        mLayout->addWidget(lab, mNextRow, 0, Qt::AlignLeft | align);
    }

    if (unit != nullptr) {
        mLayout->addWidget(wid, mNextRow, 2, align);
        mLayout->addWidget(unit, mNextRow, 3, Qt::AlignLeft | align);
    } else {
        mLayout->addWidget(wid, mNextRow, 2, 1, 2, align);
    }

    ++mNextRow;
}

bool ScanParamsPage::lastRow()
{
    addGroup(nullptr);                 // hide last if present

    mLayout->addWidget(new QLabel(QString::null, this), mNextRow, 0, 1, -1, Qt::AlignTop);
    mLayout->setRowStretch(mNextRow, 9);

    return (mNextRow > 0);
}

void ScanParamsPage::addGroup(QWidget *wid)
{
    if (mPendingGroup != nullptr) {
        mPendingGroup->hide();    // dont't need this after all
    }

    mPendingGroup = wid;
}

ScanParams::ScanParams(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("ScanParams");

    mSaneDevice = nullptr;
    mVirtualFile = nullptr;
    mGammaEditButt = nullptr;
    mResolutionBind = nullptr;
    mProgressDialog = nullptr;
    mSourceSelect = nullptr;
    mLed = nullptr;

    mProblemMessage = nullptr;
    mNoScannerMessage = nullptr;
}

ScanParams::~ScanParams()
{
    //qDebug();
    delete mProgressDialog;
}

bool ScanParams::connectDevice(KScanDevice *newScanDevice, bool galleryMode)
{
    QGridLayout *lay = new QGridLayout(this);
    lay->setMargin(0);
    lay->setColumnStretch(0, 9);

    if (newScanDevice == nullptr) {            // no scanner device
        //qDebug() << "No scan device, gallery=" << galleryMode;
        mSaneDevice = nullptr;
        createNoScannerMsg(galleryMode);
        return (true);
    }

    mSaneDevice = newScanDevice;
    mScanMode = ScanParams::NormalMode;
#if 0
    // TOTO: port/update
    adf = ADF_OFF;
#endif
    QLabel *lab = new QLabel(xi18nc("@info", "<emphasis strong=\"1\">Scanner&nbsp;Settings</emphasis>"), this);
    lay->addWidget(lab, 0, 0, Qt::AlignLeft);

    mLed = new KLed(this);
    mLed->setState(KLed::Off);
    mLed->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    lay->addWidget(mLed, 0, 1, Qt::AlignRight);

    lab = new QLabel(mSaneDevice->scannerDescription(), this);
    lay->addWidget(lab, 1, 0, 1, 2, Qt::AlignLeft);

    /* load the startup scanoptions */

    /* Now create Widgets for the important scan settings */
    QWidget *sv = createScannerParams();
    lay->addWidget(sv, 3, 0, 1, 2);
    lay->setRowStretch(3, 9);

    // Load the startup options
    // TODO: check whether the saved scanner options apply to the current scanner?
    // They may be for a completely different one...
    // Or update KScanDevice and here to save/load the startup options
    // on a per-scanner basis.

    //qDebug() << "looking for startup options";
    KScanOptSet startupOptions(KScanOptSet::startupSetName());
    if (startupOptions.loadConfig(mSaneDevice->scannerBackendName()))
    {
        //qDebug() << "loading startup options";
        mSaneDevice->loadOptionSet(&startupOptions);
    }
    //else qDebug() << "Could not load startup options";

    // Reload all options, to take account of inactive ones
    mSaneDevice->reloadAllOptions();

    // Send the current settings to the previewer
    initStartupArea(startupOptions.isEmpty());		// signal newCustomScanSize()
    slotNewScanMode();					// signal scanModeChanged()
    slotNewResolution(nullptr);				// signal scanResolutionChanged

    /* Create the Scan Buttons */
    QPushButton *pb = new QPushButton(QIcon::fromTheme("preview"), i18n("Pre&view"), this);
    pb->setToolTip(i18n("Start a preview scan and show the preview image"));
    pb->setMinimumWidth(100);
    connect(pb, &QPushButton::clicked, this, &ScanParams::slotAcquirePreview);
    lay->addWidget(pb, 5, 0, Qt::AlignLeft);

    pb = new QPushButton(QIcon::fromTheme("scan"), i18n("Star&t Scan"), this);
    pb->setToolTip(i18n("Start a scan and save the scanned image"));
    pb->setMinimumWidth(100);
    connect(pb, &QPushButton::clicked, this, &ScanParams::slotStartScan);
    lay->addWidget(pb, 5, 1, Qt::AlignRight);

    /* Initialise the progress dialog */
    mProgressDialog = new QProgressDialog(QString::null, i18n("Stop"), 0, 100, nullptr);
    mProgressDialog->setModal(true);
    mProgressDialog->setAutoClose(true);
    mProgressDialog->setAutoReset(true);
    mProgressDialog->setWindowTitle(i18n("Scanning"));
    mProgressDialog->setMinimumDuration(100);
    // The next is necessary with Qt5, as otherwise the progress dialogue
    // appears to show itself after the default 'minimumDuration' (= 4 seconds),
    // even despite the previous and no 'value' being set.
    mProgressDialog->reset();
    setScanDestination(QString::null);			// reset destination display

    connect(mProgressDialog, &QProgressDialog::canceled, mSaneDevice, &KScanDevice::slotStopScanning);
    connect(mSaneDevice, &KScanDevice::sigScanProgress, this, &ScanParams::slotScanProgress);

    return (true);
}

KLed *ScanParams::operationLED() const
{
    return (mLed);
}

ScanParamsPage *ScanParams::createTab(QTabWidget *tw, const QString &title, const char *name)
{
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scroll->setWidgetResizable(true);           // stretch to width
    scroll->setFrameStyle(QFrame::NoFrame);

    ScanParamsPage *frame = new ScanParamsPage(this, name);
    scroll->setWidget(frame);
    tw->addTab(scroll, title);

    return (frame);
}

QWidget *ScanParams::createScannerParams()
{
    QTabWidget *tw = new QTabWidget(this);
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
    if (mVirtualFile != nullptr) {
        connect(mVirtualFile, &KScanOption::guiChange, this, &ScanParams::slotOptionChanged);

        l = mVirtualFile->getLabel(frame, true);
        w = mVirtualFile->widget();
        frame->addRow(l, w);

        // Selection for either virtual scanner or SANE debug

        QWidget *vbg = new QWidget(frame);
        QVBoxLayout *vbl = new QVBoxLayout(vbg);
        vbl->setMargin(0);

        QRadioButton *rb1 = new QRadioButton(i18n("SANE Debug (from PNM image)"), vbg);
        rb1->setToolTip(xi18nc("@info:tooltip", "Operate in the same way that a real scanner does (including scan area, image processing etc.), but reading from the specified image file. See <link url=\"man:sane-pnm(5)\">sane-pnm(5)</link> for more information."));
        vbl->addWidget(rb1);
        QRadioButton *rb2 = new QRadioButton(i18n("Virtual Scanner (any image format)"), vbg);
        rb2->setToolTip(xi18nc("@info:tooltip", "Do not perform any scanning or processing, but simply read the specified image file. This is for testing the image saving, etc."));
        vbl->addWidget(rb2);

        if (mScanMode == ScanParams::NormalMode) mScanMode = ScanParams::SaneDebugMode;
        rb1->setChecked(mScanMode == ScanParams::SaneDebugMode);
        rb2->setChecked(mScanMode == ScanParams::VirtualScannerMode);

        // needed for new 'buttonClicked' signal:
        QButtonGroup *vbgGroup = new QButtonGroup(vbg);
        vbgGroup->addButton(rb1, 0);
        vbgGroup->addButton(rb2, 1);
        connect(vbgGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &ScanParams::slotVirtScanModeSelect);

        l = new QLabel(i18n("Reading mode:"), frame);
        frame->addRow(l, vbg, nullptr, Qt::AlignTop);

        // Separator line after these.  Using a KScanGroup with a null text,
        // so that it looks the same as any real group separators following.
        frame->addGroup(new KScanGroup(frame, QString::null));
    }

    // Mode setting
    so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_MODE, frame);
    if (so != nullptr) {
        KScanCombo *cb = (KScanCombo *) so->widget();

        // The option display strings are translated via the 'sane-backends' message
        // catalogue, see KScanCombo::setList().  But KScanCombo::setIcon() works on
        // the untranslated strings, which are really SANE values.  So the strings
        // here need to cover the full set of possible SANE values for all backends
        // (extra ones which a particular SANE backend doesn't support don't matter).
        //
        // So don't translate these strings or wrap them in any sort of i18n().
        //
        // The combo has already been populated with the current list of SANE values
        // at the KScanOption initialisation (by KScanOption::updateList() calling
        // KScanOption::getList() which reads the list values from SANE).  But
        // this may not work properly if the list of scan modes changes at runtime!

        cb->setIcon(ScanIcons::self()->icon(ScanIcons::BlackWhite), "Lineart");
        cb->setIcon(ScanIcons::self()->icon(ScanIcons::BlackWhite), "Binary");
        cb->setIcon(ScanIcons::self()->icon(ScanIcons::Greyscale), "Gray");
        cb->setIcon(ScanIcons::self()->icon(ScanIcons::Greyscale), "Grayscale");
        cb->setIcon(ScanIcons::self()->icon(ScanIcons::Colour), "Color");
        cb->setIcon(ScanIcons::self()->icon(ScanIcons::Halftone), "Halftone");

        connect(so, &KScanOption::guiChange, this, &ScanParams::slotOptionChanged);
        connect(so, &KScanOption::guiChange, this, &ScanParams::slotNewScanMode);

        l = so->getLabel(frame, true);
        frame->addRow(l, cb);
    }

    // Resolution setting.  Try "X-Resolution" setting first, this is the
    // option we want if the resolutions are split up.  If there is no such
    // option then try just "Resolution", this may not be the same as
    // "X-Resolution" even though this was the case in SANE<=1.0.19.
    so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_X_RESOLUTION, frame);
    if (so == nullptr) {
        so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_RESOLUTION, frame);
    }
    if (so != nullptr) {
        connect(so, &KScanOption::guiChange, this, &ScanParams::slotOptionChanged);
        // Connection that passes the resolution to the previewer
        connect(so, &KScanOption::guiChange, this, &ScanParams::slotNewResolution);

        l = so->getLabel(frame, true);
        w = so->widget();
        u = so->getUnit(frame);
        frame->addRow(l, w, u);

        // Same X/Y resolution option (if present)
        mResolutionBind = mSaneDevice->getGuiElement(SANE_NAME_RESOLUTION_BIND, frame);
        if (mResolutionBind != nullptr) {
            connect(mResolutionBind, &KScanOption::guiChange, this, &ScanParams::slotOptionChanged);

            l = so->getLabel(frame, true);
            w = so->widget();
            frame->addRow(l, w);
        }

        // Now the "Y-Resolution" setting, if there is a separate one
        so = mSaneDevice->getGuiElement(SANE_NAME_SCAN_Y_RESOLUTION, frame);
        if (so!=nullptr)
        {
            // Connection that passes the resolution to the previewer
            connect(so, &KScanOption::guiChange, this, &ScanParams::slotNewResolution);

            l = so->getLabel(frame, true);
            w = so->widget();
            u = so->getUnit(frame);
            frame->addRow(l, w, u);
        }
    } else {
        //qDebug() << "Serious: No resolution option available!";
    }

    // Scan size setting
    mAreaSelect = new ScanSizeSelector(frame, mSaneDevice->getMaxScanSize());
    connect(mAreaSelect, &ScanSizeSelector::sizeSelected, this, &ScanParams::slotScanSizeSelected);
    l = new QLabel("Scan &area:", frame);       // make sure it gets an accel
    l->setBuddy(mAreaSelect->focusProxy());
    frame->addRow(l, mAreaSelect, nullptr, Qt::AlignTop);

    // Insert another beautification line
    frame->addGroup(new KScanGroup(frame, QString::null));

    // Source selection
    mSourceSelect = mSaneDevice->getGuiElement(SANE_NAME_SCAN_SOURCE, frame);
    if (mSourceSelect != nullptr) {
        connect(mSourceSelect, &KScanOption::guiChange, this, &ScanParams::slotOptionChanged);

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
    if (so != nullptr) {
        connect(so, &KScanOption::guiChange, this, &ScanParams::slotOptionChanged);

        l = so->getLabel(frame);
        w = so->widget();
        frame->addRow(l, w);
    }

    // Now all of the other options which have not been accounted for yet.
    // Split them up into "Other" and "Advanced".
    const QList<QByteArray> opts = mSaneDevice->getAllOptions();
    for (QList<QByteArray>::const_iterator it = opts.constBegin();
            it != opts.constEnd(); ++it) {
        const QByteArray opt = (*it);

        if (opt == SANE_NAME_SCAN_TL_X ||		// ignore these (scan area)
            opt == SANE_NAME_SCAN_TL_Y ||
            opt == SANE_NAME_SCAN_BR_X ||
            opt == SANE_NAME_SCAN_BR_Y)
        {
            continue;
        }

        so = mSaneDevice->getExistingGuiElement(opt);   // see if already created
        if (so != nullptr) {
            continue;    // if so ignore, don't duplicate
        }

        so = mSaneDevice->getGuiElement(opt, frame);
        if (so != nullptr) {
            //qDebug() << "creating" << (so->isCommonOption() ? "OTHER" : "ADVANCED") << "option" << opt;
            connect(so, &KScanOption::guiChange, this, &ScanParams::slotOptionChanged);

            if (so->isCommonOption()) {
                frame = otherFrame;
            } else {
                frame = advancedFrame;
            }

            w = so->widget();
            if (so->isGroup()) {
                frame->addGroup(w);
            } else {
                l = so->getLabel(frame);
                u = so->getUnit(frame);
                frame->addRow(l, w, u);
            }

            // Some special things to do for particular options
            if (opt == SANE_NAME_BIT_DEPTH) {
                connect(so, &KScanOption::guiChange, this, &ScanParams::slotNewScanMode);
            } else if (opt == SANE_NAME_CUSTOM_GAMMA) {
                // Enabling/disabling the edit button is handled by
                // slotOptionChanged() calling setEditCustomGammaTableState()
                //connect(so, SIGNAL(guiChange(KScanOption*)), SLOT(slotOptionNotify(KScanOption*)));

                mGammaEditButt = new QPushButton(i18n("Edit Gamma Table..."), this);
                mGammaEditButt->setIcon(QIcon::fromTheme("document-edit"));
                connect(mGammaEditButt, &QPushButton::clicked, this, &ScanParams::slotEditCustGamma);
                setEditCustomGammaTableState();

                frame->addRow(nullptr, mGammaEditButt, nullptr, Qt::AlignRight);
            }
        }
    }

    basicFrame->lastRow();				// final stretch row
    if (!otherFrame->lastRow()) {
        tw->setTabEnabled(1, false);
    }
    if (!advancedFrame->lastRow()) {
        tw->setTabEnabled(2, false);
    }

    return (tw);					// top-level (tab) widget
}


void ScanParams::initStartupArea(bool dontRestore)
{
// TODO: restore area a user preference
#ifdef RESTORE_AREA
    if (dontRestore)					// no saved options available
#endif
    {
        applyRect(QRect());				// set maximum scan area
        return;
    }
    // set scan area from saved
    KScanOption *tl_x = mSaneDevice->getOption(SANE_NAME_SCAN_TL_X);
    KScanOption *tl_y = mSaneDevice->getOption(SANE_NAME_SCAN_TL_Y);
    KScanOption *br_x = mSaneDevice->getOption(SANE_NAME_SCAN_BR_X);
    KScanOption *br_y = mSaneDevice->getOption(SANE_NAME_SCAN_BR_Y);

    QRect rect;
    int val1, val2;
    tl_x->get(&val1); rect.setLeft(val1);		// pass area to previewer
    br_x->get(&val2); rect.setWidth(val2 - val1);
    tl_y->get(&val1); rect.setTop(val1);
    br_y->get(&val2); rect.setHeight(val2 - val1);
    emit newCustomScanSize(rect);

    mAreaSelect->selectSize(rect);			// set selector to match
}

void ScanParams::createNoScannerMsg(bool galleryMode)
{
    QWidget *lab;
    if (galleryMode) {
        lab = messageScannerNotSelected();
    } else {
        lab = messageScannerProblem();
    }

    QGridLayout *lay = dynamic_cast<QGridLayout *>(layout());
    if (lay != nullptr) {
        lay->addWidget(lab, 0, 0, Qt::AlignTop);
    }
}

QWidget *ScanParams::messageScannerNotSelected()
{
    if (mNoScannerMessage==nullptr)
    {
        mNoScannerMessage = new KMessageWidget(
            xi18nc("@info",
                   "<emphasis strong=\"1\">No scanner selected</emphasis>"
                   "<nl/><nl/>"
                   "Select a scanner device to perform scanning."));

        mNoScannerMessage->setMessageType(KMessageWidget::Information);
        mNoScannerMessage->setIcon(QIcon::fromTheme("dialog-information"));
        mNoScannerMessage->setCloseButtonVisible(false);
        mNoScannerMessage->setWordWrap(true);
    }

    return (mNoScannerMessage);
}

QWidget *ScanParams::messageScannerProblem()
{
    if (mProblemMessage==nullptr)
    {
        mProblemMessage = new KMessageWidget(
            xi18nc("@info",
                   "<emphasis strong=\"1\">No scanner found, or unable to access it</emphasis>"
                   "<nl/><nl/>"
                   "There was a problem using the SANE (Scanner Access Now Easy) library to access "
                   "the scanner device.  There may be a problem with your SANE installation, or it "
                   "may not be configured to support your scanner."
                   "<nl/><nl/>"
                   "Check that SANE is correctly installed and configured on your system, and "
                   "also that the scanner device name and settings are correct."
                   "<nl/><nl/>"
                   "See the SANE project home page "
                   "(<link url=\"http://www.sane-project.org\">www.sane-project.org</link>) "
                   "for more information on SANE installation and setup."));

        mProblemMessage->setMessageType(KMessageWidget::Warning);
        mProblemMessage->setIcon(QIcon::fromTheme("dialog-warning"));
        mProblemMessage->setCloseButtonVisible(false);
        mProblemMessage->setWordWrap(true);
        connect(mProblemMessage, &KMessageWidget::linkActivated, this, &ScanParams::slotMessageLinkActivated);
    }

    return (mProblemMessage);
}


void ScanParams::slotMessageLinkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}


void ScanParams::slotSourceSelect()
{
#if 0
// TODO: port/update
    AdfBehaviour adf = ADF_OFF;

    if (mSourceSelect == nullptr) {
        return;    // no source selection GUI
    }
    if (!mSourceSelect->isValid()) {
        return;    // no option on scanner
    }

    const QByteArray &currSource = mSourceSelect->get();
    //qDebug() << "Current source is" << currSource;

    QList<QByteArray> sources = mSourceSelect->getList();
#ifdef DEBUG_ADF
    if (!sources.contains("Automatic Document Feeder")) {
        sources.append("Automatic Document Feeder");
    }
#endif

    // TODO: the 'sources' list has exactly the same options as the
    // scan source combo (apart from the debugging hack above), so
    // what's the point of repeating it in this dialogue?
    ScanSourceDialog d(this, sources, adf);
    d.slotSetSource(currSource);

    if (d.exec() != QDialog::Accepted) {
        return;
    }

    QString sel_source = d.getText();
    adf = d.getAdfBehave();
    //qDebug() << "new source" << sel_source << "ADF" << adf;

    /* set the selected Document source, the behavior is stored in a membervar */
    mSourceSelect->set(sel_source.toLatin1());      // TODO: FIX in ScanSourceDialog, then here
    mSourceSelect->apply();
    mSourceSelect->reload();
    mSourceSelect->redrawWidget();
#endif
}

/* Slot which is called if the user switches in the gui between
 *  the SANE-Debug-Mode and the qt image reading
 */
void ScanParams::slotVirtScanModeSelect(int but)
{
    if (but == 0) mScanMode = ScanParams::SaneDebugMode;	// SANE Debug
    else mScanMode = ScanParams::VirtualScannerMode;		// Virtual Scanner
    const bool enable = (mScanMode == ScanParams::SaneDebugMode);

    mSaneDevice->guiSetEnabled(SANE_NAME_HAND_SCANNER, enable);
    mSaneDevice->guiSetEnabled(SANE_NAME_THREE_PASS, enable);
    mSaneDevice->guiSetEnabled(SANE_NAME_GRAYIFY, enable);
    mSaneDevice->guiSetEnabled(SANE_NAME_CONTRAST, enable);
    mSaneDevice->guiSetEnabled(SANE_NAME_BRIGHTNESS, enable);
    mSaneDevice->guiSetEnabled(SANE_NAME_SCAN_RESOLUTION, enable);
    mSaneDevice->guiSetEnabled(SANE_NAME_SCAN_X_RESOLUTION, enable);
    mSaneDevice->guiSetEnabled(SANE_NAME_SCAN_Y_RESOLUTION, enable);

    mAreaSelect->setEnabled(enable);
}

KScanDevice::Status ScanParams::prepareScan(QString *vfp)
{
    //qDebug() << "scan mode=" << mScanMode;

    setScanDestination(QString::null);          // reset progress display

    // Check compatibility of scan settings
    int format;
    int depth;
    mSaneDevice->getCurrentFormat(&format, &depth);
    if (depth == 1 && format != SANE_FRAME_GRAY) {  // 1-bit scan depth in colour?
        KMessageBox::sorry(this, i18n("1-bit depth scan cannot be done in color"));
        return (KScanDevice::ParamError);
    } else if (depth == 16) {
        KMessageBox::sorry(this, i18n("16-bit depth scans are not supported"));
        return (KScanDevice::ParamError);
    }

    QString virtfile;
    if (mScanMode == ScanParams::SaneDebugMode || mScanMode == ScanParams::VirtualScannerMode) {
        if (mVirtualFile != nullptr) {
            virtfile = QFile::decodeName(mVirtualFile->get());
        }
        if (virtfile.isEmpty()) {
            KMessageBox::sorry(this, i18n("A file must be entered for testing or virtual scanning"));
            return (KScanDevice::ParamError);
        }

        QFileInfo fi(virtfile);
        if (!fi.exists()) {
            KMessageBox::sorry(this, xi18nc("@info", "The testing or virtual file <filename>%1</filename><nl/>was not found or is not readable.", virtfile));
            return (KScanDevice::ParamError);
        }

        if (mScanMode == ScanParams::SaneDebugMode) {
            QMimeDatabase db;
            QMimeType mime = db.mimeTypeForFile(virtfile);
            if (!(mime.inherits("image/x-portable-bitmap") ||
                  mime.inherits("image/x-portable-greymap") ||
                  mime.inherits("image/x-portable-pixmap"))) {
                KMessageBox::sorry(this, xi18nc("@info", "SANE Debug can only read PNM files.<nl/>"
                                              "The specified file is type <icode>%1</icode>.", mime.name()));
                return (KScanDevice::ParamError);
            }
        }
    }

    if (vfp != nullptr) {
        *vfp = virtfile;
    }
    return (KScanDevice::Ok);
}

void ScanParams::setScanDestination(const QString &dest)
{
    //qDebug() << "scan destination is" << dest;
    if (dest.isEmpty()) {
        mProgressDialog->setLabelText(i18n("Scan in progress"));
    } else {
        mProgressDialog->setLabelText(xi18nc("@info", "Scan in progress<nl/><nl/><filename>%1</filename>", dest));
    }
}

void ScanParams::slotScanProgress(int value)
{
    mProgressDialog->setValue(value);
}

/* Slot called to start acquiring a preview */
void ScanParams::slotAcquirePreview()
{

    // TODO: should be able to preview in Virtual Scanner mode, it just means
    // that the preview image will be the same size as the final image (which
    // doesn't matter).

    if (mScanMode == ScanParams::VirtualScannerMode) {
        KMessageBox::sorry(this, i18n("Cannot preview in Virtual Scanner mode"));
        return;
    }

    QString virtfile;
    KScanDevice::Status stat = prepareScan(&virtfile);
    if (stat != KScanDevice::Ok) {
        return;
    }

    //qDebug() << "scan mode=" << mScanMode << "virtfile" << virtfile;

    KScanOption *greyPreview = mSaneDevice->getExistingGuiElement(SANE_NAME_GRAY_PREVIEW);
    int gp = 0;
    if (greyPreview != nullptr) {
        greyPreview->get(&gp);
    }

    setMaximalScanSize();               // always preview at maximal size
    mAreaSelect->selectCustomSize(QRect());     // reset selector to reflect that

    stat = mSaneDevice->acquirePreview(gp);
    if (stat != KScanDevice::Ok) {
        //qDebug() << "Error, preview status " << stat;
    }
}

/* Slot called to start scanning */
void ScanParams::slotStartScan()
{
    QString virtfile;
    KScanDevice::Status stat = prepareScan(&virtfile);
    if (stat != KScanDevice::Ok) {
        return;
    }

    //qDebug() << "scan mode=" << mScanMode << "virtfile" << virtfile;

    if (mScanMode != ScanParams::VirtualScannerMode) { // acquire via SANE
#if 0
// TODO: port/update
        if (adf == ADF_OFF) {
#endif
            //qDebug() << "Start to acquire image";
            stat = mSaneDevice->acquireScan();
#if 0
        } else {
            //qDebug() << "ADF Scan not yet implemented :-/";
            // stat = performADFScan();
        }
#endif
    } else {                    // acquire via Qt-IMGIO
        //qDebug() << "Acquiring from virtual file";
        stat = mSaneDevice->acquireScan(virtfile);
    }

    if (stat != KScanDevice::Ok) {
        //qDebug() << "Error, scan status " << stat;
    }
}

bool ScanParams::getGammaTableFrom(const QByteArray &opt, KGammaTable *gt)
{
    KScanOption *so = mSaneDevice->getOption(opt, false);
    if (so == nullptr) {
        return (false);
    }

    if (!so->get(gt)) {
        return (false);
    }
    //qDebug() << "got from" << so->getName() << "=" << gt->toString();
    return (true);
}

bool ScanParams::setGammaTableTo(const QByteArray &opt, const KGammaTable *gt)
{
    KScanOption *so = mSaneDevice->getOption(opt, false);
    if (so == nullptr) {
        return (false);
    }

    //qDebug() << "set" << so->getName() << "=" << gt->toString();
    so->set(gt);
    return (so->apply());
}

void ScanParams::slotEditCustGamma()
{
    KGammaTable gt;                 // start with default values

    // Get the current gamma table from either the combined gamma table
    // option, or any one of the colour channel gamma tables.
    if (!getGammaTableFrom(SANE_NAME_GAMMA_VECTOR, &gt)) {
        if (!getGammaTableFrom(SANE_NAME_GAMMA_VECTOR_R, &gt)) {
            if (!getGammaTableFrom(SANE_NAME_GAMMA_VECTOR_G, &gt)) {
                if (!getGammaTableFrom(SANE_NAME_GAMMA_VECTOR_B, &gt)) {
                    // Should not happen... but if it does, no problem
                    // the dialogue will just use the default values
                    // for an empty gamma table.
                    //qDebug() << "no existing/active gamma option!";
                }
            }
        }
    }
    //qDebug() << "initial gamma table" << gt.toString();

// TODO; Maybe need to have a separate GUI widget for each gamma table, a
// little preview of the gamma curve (a GammaWidget) with an edit button.
// Will avoid the special case for the SANE_NAME_CUSTOM_GAMMA button followed
// by the edit button, and will allow separate editing of the R/G/B gamma
// tables if the scanner has them.

    GammaDialog gdiag(&gt, this);
    connect(&gdiag, &GammaDialog::gammaToApply, this, &ScanParams::slotApplyGamma);
    gdiag.exec();
}

void ScanParams::slotApplyGamma(const KGammaTable *gt)
{
    if (gt == nullptr) {
        return;
    }

    bool reload = false;

    KScanOption *so = mSaneDevice->getOption(SANE_NAME_CUSTOM_GAMMA);
    if (so != nullptr) {               // do we have a gamma switch?
        int cg = 0;
        if (so->get(&cg) && !cg) {          // yes, see if already on
            // if not, set it on now
            //qDebug() << "Setting gamma switch on";
            so->set(true);
            reload = so->apply();
        }
    }

    //qDebug() << "Applying gamma table" << gt->toString();
    reload |= setGammaTableTo(SANE_NAME_GAMMA_VECTOR, gt);
    reload |= setGammaTableTo(SANE_NAME_GAMMA_VECTOR_R, gt);
    reload |= setGammaTableTo(SANE_NAME_GAMMA_VECTOR_G, gt);
    reload |= setGammaTableTo(SANE_NAME_GAMMA_VECTOR_B, gt);

    if (reload) {
        mSaneDevice->reloadAllOptions();    // reload is needed
    }
}

// The user has changed an option.  Apply that;  as a result of doing so,
// it may be necessary to reload every other scanner option apart from
// this one.

void ScanParams::slotOptionChanged(KScanOption *so)
{
    if (so == nullptr || mSaneDevice == nullptr) {
        return;
    }
    mSaneDevice->applyOption(so);

    // Update the gamma edit button state, if the option exists
    setEditCustomGammaTableState();
}

// Enable editing of the gamma tables if any one of the gamma tables
// exists and is currently active.

void ScanParams::setEditCustomGammaTableState()
{
    if (mSaneDevice == nullptr) {
        return;
    }
    if (mGammaEditButt == nullptr) {
        return;
    }

    bool butState = false;

    KScanOption *so = mSaneDevice->getOption(SANE_NAME_CUSTOM_GAMMA, false);
    if (so != nullptr) {
        butState = so->isActive();
    }

    if (!butState) {
        KScanOption *so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR, false);
        if (so != nullptr) {
            butState = so->isActive();
        }
    }

    if (!butState) {
        KScanOption *so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR_R, false);
        if (so != nullptr) {
            butState = so->isActive();
        }
    }

    if (!butState) {
        KScanOption *so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR_G, false);
        if (so != nullptr) {
            butState = so->isActive();
        }
    }

    if (!butState) {
        KScanOption *so = mSaneDevice->getOption(SANE_NAME_GAMMA_VECTOR_B, false);
        if (so != nullptr) {
            butState = so->isActive();
        }
    }

    //qDebug() << "State of edit custom gamma button=" << butState;
    mGammaEditButt->setEnabled(butState);
}

// This assumes that the SANE unit for the scan area is millimetres.
// All scanners out there appear to do this.
void ScanParams::applyRect(const QRect &rect)
{
    //qDebug() << "rect=" << rect;

    KScanOption *tl_x = mSaneDevice->getOption(SANE_NAME_SCAN_TL_X);
    KScanOption *tl_y = mSaneDevice->getOption(SANE_NAME_SCAN_TL_Y);
    KScanOption *br_x = mSaneDevice->getOption(SANE_NAME_SCAN_BR_X);
    KScanOption *br_y = mSaneDevice->getOption(SANE_NAME_SCAN_BR_Y);

    double min1, max1;
    double min2, max2;

    if (!rect.isValid()) {              // set full scan area
        tl_x->getRange(&min1, &max1); tl_x->set(min1);
        br_x->getRange(&min1, &max1); br_x->set(max1);
        tl_y->getRange(&min2, &max2); tl_y->set(min2);
        br_y->getRange(&min2, &max2); br_y->set(max2);

        //qDebug() << "setting full area" << min1 << min2 << "-" << max1 << max2;
    } else {
        double tlx = rect.left();
        double tly = rect.top();
        double brx = rect.right();
        double bry = rect.bottom();

        tl_x->getRange(&min1, &max1);
        if (tlx < min1) {
            brx += (min1 - tlx);
            tlx = min1;
        }
        tl_x->set(tlx); br_x->set(brx);

        tl_y->getRange(&min2, &max2);
        if (tly < min2) {
            bry += (min2 - tly);
            tly = min2;
        }
        tl_y->set(tly); br_y->set(bry);

        //qDebug() << "setting area" << tlx << tly << "-" << brx << bry;
    }

    tl_x->apply();
    tl_y->apply();
    br_x->apply();
    br_y->apply();
}

//  The previewer is telling us that the user has drawn or auto-selected a
//  new preview area (specified in millimetres).
void ScanParams::slotNewPreviewRect(const QRect &rect)
{
    //qDebug() << "rect=" << rect;

    applyRect(rect);
    mAreaSelect->selectSize(rect);
}

//  A new preset scan size or orientation chosen by the user
void ScanParams::slotScanSizeSelected(const QRect &rect)
{
    //qDebug() << "rect=" << rect << "full=" << !rect.isValid();

    applyRect(rect);
    emit newCustomScanSize(rect);
}

/*
 * sets the scan area to the default, which is the whole area.
 */
void ScanParams::setMaximalScanSize()
{
    //qDebug() << "Setting to default";
    slotScanSizeSelected(QRect());
}

void ScanParams::slotNewResolution(KScanOption *opt)
{
    KScanOption *opt_x = mSaneDevice->getExistingGuiElement(SANE_NAME_SCAN_X_RESOLUTION);
    if (opt_x == nullptr) {
        opt_x = mSaneDevice->getExistingGuiElement(SANE_NAME_SCAN_RESOLUTION);
    }
    KScanOption *opt_y = mSaneDevice->getExistingGuiElement(SANE_NAME_SCAN_Y_RESOLUTION);

    int x_res = 0;                                      // get the X resolution
    if (opt_x != nullptr && opt_x->isValid()) {
        opt_x->get(&x_res);
    }

    int y_res = 0;                                      // get the Y resolution
    if (opt_y != nullptr && opt_y->isValid()) {
        opt_y->get(&y_res);
    }

    //qDebug() << "X/Y resolution" << x_res << y_res;

    if (y_res == 0) {
        y_res = x_res;    // use X res if Y unavailable
    }
    if (x_res == 0) {
        x_res = y_res;    // unlikely, but orthogonal
    }

    if (x_res == 0 && y_res == 0) {
        //qDebug() << "resolution not available!";
    } else {
        emit scanResolutionChanged(x_res, y_res);
    }
}

void ScanParams::slotNewScanMode()
{
    int format = SANE_FRAME_RGB;
    int depth = 8;
    mSaneDevice->getCurrentFormat(&format, &depth);

    int strips = (format == SANE_FRAME_GRAY ? 1 : 3);

    qDebug() << "format" << format << "depth" << depth << "-> strips " << strips;

    if (strips == 1 && depth == 1) {        // bitmap scan
        emit scanModeChanged(0);            // 8 pixels per byte
    } else {
        // bytes per pixel
        emit scanModeChanged(strips * (depth == 16 ? 2 : 1));
    }
}

KScanDevice::Status ScanParams::performADFScan(void)
{
    KScanDevice::Status stat = KScanDevice::Ok;
    bool          scan_on = true;

#if 0
// TODO: port/update
    MassScanDialog *msd = new MassScanDialog(this);
    msd->show();
#endif

    /* The scan source should be set to ADF by the SourceSelect-Dialog */

    while (scan_on) {
        scan_on = false;
    }
    return (stat);
}
