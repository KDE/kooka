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

#include "kscandevice.h"

#include <qimage.h>
#include <qfileinfo.h>
#include <qapplication.h>
#include <qsocketnotifier.h>
#include <qstandardpaths.h>
#include <qtimer.h>

#include <klocalizedstring.h>
#include <kconfig.h>
#include <kpassworddialog.h>

#include "scanglobal.h"
#include "scandevices.h"
#include "kgammatable.h"
#include "kscancontrols.h"
#include "kscanoption.h"
#include "deviceselector.h"
#include "scansettings.h"
#include "multiscanoptions.h"
#include "continuescandialog.h"
#include "libkookascan_logging.h"

extern "C" {
#include <sane/saneopts.h>
}

#define MIN_PREVIEW_DPI		75
#define MAX_PROGRESS		100

#define BUSY_RETRIES		2
#define BUSY_DELAY		500


// Debugging options
#define DEBUG_OPTIONS
#undef DEBUG_RELOAD
#undef DEBUG_CREATE
#define DEBUG_PARAMS

#ifdef DEBUG_OPTIONS
#include <iostream>
#endif // DEBUG_OPTIONS


//  Accessing GUI options
//  ---------------------

// Used only by ScanParams::slotVirtScanModeSelect()
void KScanDevice::guiSetEnabled(const QByteArray &name, bool state)
{
    KScanOption *so = getExistingGuiElement(name);
    if (so==nullptr) return;

    QWidget *w = so->widget();
    if (w==nullptr) return;

    w->setEnabled(state && so->isSoftwareSettable());
}


KScanOption *KScanDevice::getOption(const QByteArray &name, bool create)
{
    QByteArray alias = aliasName(name);

    if (mCreatedOptions.contains(alias))
    {
#ifdef DEBUG_CREATE
        qCDebug(LIBKOOKASCAN_LOG) << "already exists" << alias;
#endif // DEBUG_CREATE
        return (mCreatedOptions.value(alias));
    }

    if (!create)
    {
#ifdef DEBUG_CREATE
        qCDebug(LIBKOOKASCAN_LOG) << "does not exist" << alias;
#endif // DEBUG_CREATE
        return (nullptr);
    }

#ifdef DEBUG_CREATE
    qCDebug(LIBKOOKASCAN_LOG) << "creating new" << alias;
#endif // DEBUG_CREATE
    KScanOption *so = new KScanOption(alias, this);
    mCreatedOptions.insert(alias, so);
    return (so);
}


KScanOption *KScanDevice::getExistingGuiElement(const QByteArray &name) const
{
    KScanOption *opt = mCreatedOptions.value(aliasName(name));
    if (opt!=nullptr && opt->isGuiElement()) return (opt);
    return (nullptr);
}


KScanOption *KScanDevice::getGuiElement(const QByteArray &name, QWidget *parent)
{
    if (name.isEmpty()) return (nullptr);
    if (!optionExists(name)) return (nullptr);

    //qCDebug(LIBKOOKASCAN_LOG) << "for" << name;

    KScanOption *so = getExistingGuiElement(name);	// see if already exists
    if (so!=nullptr) return (so);			// if so, just return that

    so = getOption(name);				// create a new scan option
    if (so->isValid())					// option was created
    {
        QWidget *w = so->createWidget(parent);		// create widget for option
        if (w!=nullptr) w->setEnabled(so->isActive() && so->isSoftwareSettable());
        else qCDebug(LIBKOOKASCAN_LOG) << "no widget created for" << name;
    }
    else						// option not valid
    {							// (not known by scanner?)
        qCDebug(LIBKOOKASCAN_LOG) << "option invalid" << name;
        so = nullptr;
    }

    return (so);
}


//  Constructor & destructor
//  ------------------------

KScanDevice::KScanDevice(QObject *parent)
   : QObject(parent)
{
    qCDebug(LIBKOOKASCAN_LOG);

    ScanGlobal::self()->init();				// do sane_init() first of all

    mScannerHandle = nullptr;
    mScannerInitialised = false;			// is device opened yet?
    mScannerName = "";

    mScanningState = KScanDevice::ScanIdle;
    mScanningAdf = false;				// assume no ADF so far

    mScanBuf = nullptr;					// image data buffer while scanning
    mScanImage.clear();					// temporary image to scan into
    mTestFormat = ScanImage::None;			// format deduced from scan

    mSocketNotifier = nullptr;				// socket notifier for async scanning

    mBytesRead = 0;
    mBytesUsed = 0;
    mPixelX = 0;
    mPixelY = 0;
}


KScanDevice::~KScanDevice()
{
// TODO: need to check and do closeDevice() here?
    ScanGlobal::self()->setScanDevice(nullptr);		// going away, don't call me

    qCDebug(LIBKOOKASCAN_LOG) << "done";
}


//  Opening/closing the scanner device
//  ----------------------------------

KScanDevice::Status KScanDevice::openDevice(const QByteArray &backend)
{
    KScanDevice::Status stat = KScanDevice::Ok;

    qCDebug(LIBKOOKASCAN_LOG) << "backend" << backend;

    mSaneStatus = SANE_STATUS_UNSUPPORTED;
    if (backend.isEmpty()) return (KScanDevice::ParamError);

    // search for scanner
    if (ScanDevices::self()->deviceInfo(backend)==nullptr) return (KScanDevice::NoDevice);

    mScannerName = backend;				// set now for authentication
    QApplication::setOverrideCursor(Qt::WaitCursor);	// potential lengthy operation
    ScanGlobal::self()->setScanDevice(this);		// for possible authentication

    ScanDevices::self()->deactivateNetworkProxy();
    mSaneStatus = sane_open(backend.constData(), &mScannerHandle);
    ScanDevices::self()->reactivateNetworkProxy();

    if (mSaneStatus==SANE_STATUS_ACCESS_DENIED)		// authentication failed?
    {
        clearSavedAuth();				// clear any saved password
        qCDebug(LIBKOOKASCAN_LOG) << "retrying authentication";	// try again once more
        mSaneStatus = sane_open(backend.constData(), &mScannerHandle);
    }

    if (mSaneStatus==SANE_STATUS_GOOD)
    {
        stat = findOptions();				// fill dictionary with options
        mScannerInitialised = true;			// note scanner opened OK
    }
    else
    {
        stat = KScanDevice::OpenDevice;
        mScannerName = "";
    }

    QApplication::restoreOverrideCursor();
    return (stat);
}


void KScanDevice::closeDevice()
{
    emit sigCloseDevice();				// tell callers we're closing

    //qCDebug(LIBKOOKASCAN_LOG) << "Saving default scan settings";
    saveStartupConfig();				// save config for next startup

    if (mScannerHandle!=nullptr)
    {
        ScanDevices::self()->deactivateNetworkProxy();
        if (mScanningState!=KScanDevice::ScanIdle)
        {
            qCDebug(LIBKOOKASCAN_LOG) << "Scanner is still active, calling sane_cancel()";
            sane_cancel(mScannerHandle);
        }
        sane_close(mScannerHandle);			// close the SANE device
        mScannerHandle = nullptr;			// scanner no longer open
        ScanDevices::self()->reactivateNetworkProxy();
    }

    // clear lists of options
    QList<KScanOption *> opts = mCreatedOptions.values();
    while (!opts.isEmpty()) delete opts.takeFirst();
    mCreatedOptions.clear();
    mKnownOptions.clear();

    mScannerName = "";
    mScannerInitialised = false;
}


//  Scanner and image information
//  -----------------------------

QString KScanDevice::scannerDescription() const
{
    QString ret;

    if (!mScannerName.isNull() && mScannerInitialised)
    {
        ret = ScanDevices::self()->deviceDescription(mScannerName);
    }
    else
    {
        ret = i18n("No scanner selected");
    }

    return (ret);
}


QSize KScanDevice::getMaxScanSize()
{
    QSize s;
    double min, max;

    KScanOption *so_w = getOption(SANE_NAME_SCAN_BR_X);
    so_w->getRange(&min, &max);
    s.setWidth(qRound(max));

    KScanOption *so_h = getOption(SANE_NAME_SCAN_BR_Y);
    so_h->getRange(&min, &max);
    s.setHeight(qRound(max));

    return (s);
}


void KScanDevice::getCurrentFormat(int *format, int *depth)
{
    sane_get_parameters(mScannerHandle, &mSaneParameters);
    *format = mSaneParameters.format;
    *depth = mSaneParameters.depth;
}


/* static */ bool KScanDevice::matchesAdf(const QByteArray &val)
{
    // There does not seem to be any "official" SANE way to find out whether
    // the scan source is an ADF, so it has to be done by looking at the
    // string value of the option.  Not sure whether this will work properly
    // if SANE is I18N'ed.
    return (val.contains("ADF") ||			// case sensitive
            val.startsWith("Auto") ||			// case sensitive
            val.toLower().contains("feeder"));		// case insensitive
}


void KScanDevice::updateAdfState(const KScanOption *so)
{
    if (so==nullptr) so = getOption(SANE_NAME_SCAN_SOURCE, false);
    if (so==nullptr) mScanningAdf = false;		// no source option, assume no ADF
    else						// there is a source option
    {
        mScanningAdf = matchesAdf(so->get());
        qCDebug(LIBKOOKASCAN_LOG) << "ADF in use?" << mScanningAdf;
    }
}


bool KScanDevice::isAdfAvailable()
{
    const KScanOption *so = getOption(SANE_NAME_SCAN_SOURCE, false);
    if (so==nullptr) return (false);			// no source option, assume no ADF

    const QList<QByteArray> sources = so->getList();
    if (sources.isEmpty()) return (false);		// unexpected option type, assume no ADF

    for (const QByteArray &src : std::as_const(sources))
    {
        if (matchesAdf(src)) return (true);		// this source is an ADF
    }

    return (false);					// no ADF found
}


//  Listing the available options
//  -----------------------------

KScanDevice::Status KScanDevice::findOptions()
{
    SANE_Int n;
    SANE_Int opt;

    if (sane_control_option(mScannerHandle, 0, SANE_ACTION_GET_VALUE,
                            &n, &opt)!=SANE_STATUS_GOOD)
    {
        qCWarning(LIBKOOKASCAN_LOG) << "cannot read option 0 (count)";
        return (KScanDevice::ControlError);
    }

    mKnownOptions.clear();
    for (int i = 1; i<n; i++)
    {
        const SANE_Option_Descriptor *d = sane_get_option_descriptor(mScannerHandle, i);
        if (d==nullptr) continue;			// could not get descriptor

        QByteArray name;
        if (d->name!=nullptr && strlen(d->name)>0) name = d->name;

        if (d->type==SANE_TYPE_GROUP)			// option is a group,
        {						// give it a dummy name
            name = "group-";
            name += QByteArray::number(i);
        }

        if (!name.isEmpty())				// must now have a name
        {
#ifdef DEBUG_OPTIONS
            qCDebug(LIBKOOKASCAN_LOG) << "Option" << i << "is" << name;
#endif // DEBUG_OPTIONS
            mKnownOptions.insert(i, name);
        }
        else qCWarning(LIBKOOKASCAN_LOG) << "Invalid option" << i << "(no name and not a group)";
    }

    return (KScanDevice::Ok);
}


QList<QByteArray> KScanDevice::getAllOptions() const
{
    return (mKnownOptions.values());
}


QList<QByteArray> KScanDevice::getCommonOptions() const
{
    QList<QByteArray> opts;

    for (OptionHash::const_iterator it = mCreatedOptions.constBegin();
         it!=mCreatedOptions.constEnd(); ++it)
    {
        const KScanOption *so = it.value();
        if (so->isCommonOption()) opts.append(it.key());
    }

    return (opts);
}


QList<QByteArray> KScanDevice::getAdvancedOptions() const
{
    QList<QByteArray> opts;

    for (OptionHash::const_iterator it = mCreatedOptions.constBegin();
         it!=mCreatedOptions.constEnd(); ++it)
    {
        const KScanOption *so = it.value();
        if (!so->isCommonOption()) opts.append(it.key());
    }

    return (opts);
}


//  Controlling options
//  -------------------

int KScanDevice::getOptionIndex(const QByteArray &name) const
{
    return (mKnownOptions.key(name));
}


bool KScanDevice::optionExists(const QByteArray &name) const
{
   if (name.isEmpty()) return (false);
   QByteArray alias = aliasName(name);
   return (mKnownOptions.key(alias)!=0);
}


/* This function tries to find name aliases which appear from backend to backend.
 *  Example: Custom-Gamma is for epson backends 'gamma-correction' - not a really
 *  cool thing :-|
 *  Maybe this helps us out ?
 */
QByteArray KScanDevice::aliasName( const QByteArray& name ) const
{
    if (mCreatedOptions.contains(name)) return (name);

    QByteArray ret = name;
    if (name == SANE_NAME_CUSTOM_GAMMA)
    {
        if (mCreatedOptions.contains("gamma-correction")) ret = "gamma-correction";
    }

    if (ret!=name) qCDebug(LIBKOOKASCAN_LOG) << "Found alias for" << name << "which is" << ret;
    return (ret);
}


void KScanDevice::applyOption(KScanOption *opt)
{
    bool reload = true;					// is a reload needed?

    if (opt!=nullptr)					// an option is specified
    {
#ifdef DEBUG_RELOAD
        qCDebug(LIBKOOKASCAN_LOG) << "option" << opt->getName();
#endif // DEBUG_RELOAD
        reload = opt->apply();				// apply this option
							// update the ADF state
        if (opt->getName()==SANE_NAME_SCAN_SOURCE) updateAdfState(opt);
    }

    if (!reload)					// need to reload now?
    {
#ifdef DEBUG_RELOAD
        qCDebug(LIBKOOKASCAN_LOG) << "Reload of others not needed";
#endif // DEBUG_RELOAD
        return;
    }
							// reload of all others needed
    // TODO: range-based for over hash values
    for (OptionHash::const_iterator it = mCreatedOptions.constBegin();
         it!=mCreatedOptions.constEnd(); ++it)
    {
        KScanOption *so = it.value();
        if (!so->isGuiElement()) continue;
        if (opt==nullptr || so!=opt)
        {
#ifdef DEBUG_RELOAD
            qCDebug(LIBKOOKASCAN_LOG) << "Reloading" << so->getName();
#endif // DEBUG_RELOAD
            so->reload();
            so->redrawWidget();
        }
    }

#ifdef DEBUG_RELOAD
    qCDebug(LIBKOOKASCAN_LOG) << "Finished";
#endif // DEBUG_RELOAD
}


void KScanDevice::reloadAllOptions()
{
#ifdef DEBUG_RELOAD
    qCDebug(LIBKOOKASCAN_LOG);
#endif // DEBUG_RELOAD
    applyOption(nullptr);
}


void KScanDevice::applyAllOptions(bool prio)
{
    for (OptionHash::const_iterator it = mCreatedOptions.constBegin();
         it!=mCreatedOptions.constEnd(); ++it)
    {
            KScanOption *so = it.value();
            if (!so->isGuiElement()) continue;
            if (so->isPriorityOption() ^ prio) continue;
            if (so->isActive() && so->isSoftwareSettable()) so->apply();
            if (so->getName()==SANE_NAME_SCAN_SOURCE) updateAdfState(so);
    }
}


//  Scanning control
//  ----------------

void KScanDevice::slotStopScanning()
{
    qCDebug(LIBKOOKASCAN_LOG);
    mScanningState = KScanDevice::ScanStopNow;
}


//  Preview image
//  -------------

const QString KScanDevice::previewFile() const
{
    // TODO: this doesn't work if that directory doesn't exist,
    // and nothing ever creates that directory!
    //
    // Do we want this feature to work?
    // If so, create the directory in savePreviewImage() below
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/previews/";
    QString sname(scannerDescription());
    sname.replace('/', "_");
    return (dir+sname);
}


QImage KScanDevice::loadPreviewImage() const
{
    const QString prevFile = previewFile();
    qCDebug(LIBKOOKASCAN_LOG) << "Loading preview from" << prevFile;
    return (QImage(prevFile));
}


// TODO: take a ScanImage::Ptr
bool KScanDevice::savePreviewImage(const QImage &image) const
{
    if (image.isNull()) return (false);

    const QString prevFile = previewFile();
    qCDebug(LIBKOOKASCAN_LOG) << "Saving preview to" << prevFile;
    return (image.save(prevFile, "BMP"));
}


//  Displaying scan options
//  -----------------------
//
//  For debugging.  Originally showOptions() was called prepareScan() and had
//  the comment:
//
//    prepareScan tries to set as much as parameters as possible.
//
//    Function which applies all Options which need to be applied.
//    See SANE-Documentation Table 4.5, description for SANE_CAP_SOFT_DETECT.
//    The function sets the options which have SANE_CAP_AUTOMATIC set
//    to automatic adjust.
//
//  But this wasn't true - it only reports the current state of the options.

#ifdef DEBUG_OPTIONS

inline const char *optionNotifyString(int opt)
{
    return (opt!=0 ? " X |" : " - |");
}


void KScanDevice::showOptions()
{
    qCDebug(LIBKOOKASCAN_LOG) << "for" << mScannerName;
    std::cerr << "----------------------------------+---+---+---+---+---+---+---+---+-------" << std::endl;
    std::cerr << " Option                           |SSL|HSL|SDT|EMU|AUT|INA|ADV|PRI| Value" << std::endl;
    std::cerr << "----------------------------------+---+---+---+---+---+---+---+---+-------" << std::endl;

    QList<QByteArray> optionNames = mCreatedOptions.keys();
    std::sort(optionNames.begin(), optionNames.end());
    for (const QByteArray &optionName : std::as_const(optionNames))
    {
        const KScanOption *so = mCreatedOptions.value(optionName);
        if (so->isGroup()) continue;

        int cap = so->getCapabilities();
        std::cerr <<
            " " << qPrintable(optionName.left(31).leftJustified(32)) << " |" <<
            optionNotifyString((cap & SANE_CAP_SOFT_SELECT)) << 
            optionNotifyString((cap & SANE_CAP_HARD_SELECT)) << 
            optionNotifyString((cap & SANE_CAP_SOFT_DETECT)) << 
            optionNotifyString((cap & SANE_CAP_EMULATED)) << 
            optionNotifyString((cap & SANE_CAP_AUTOMATIC)) << 
            optionNotifyString((cap & SANE_CAP_INACTIVE)) << 
            optionNotifyString((cap & SANE_CAP_ADVANCED)) <<
            optionNotifyString(so->isPriorityOption()) <<
            " " << qPrintable(so->get()) << std::endl;
    }
    std::cerr << "----------------------------------+---+---+---+---+---+---+---+---+-------" << std::endl;
}

#endif // DEBUG_OPTIONS


//  ------------------------------------------------

// Deduce the scanned image type from the SANE parameters.
// The logic is very similar to ScanImage::imageType(),
// except that the image type cannot be LowColour.
static ScanImage::ImageType getImageFormat(const SANE_Parameters *p)
{
    if (p==nullptr) return (ScanImage::None);

    if (p->depth==1) 					// Line art (bitmap)
    {
        return (ScanImage::BlackWhite);
    }
    else if (p->depth==8)				// 8 bit RGB
    {
        if (p->format==SANE_FRAME_GRAY)			// Grey scale
        {
            return (ScanImage::Greyscale);
        }
        else						// True colour
        {
            return (ScanImage::HighColour);
        }
    }
    else						// Error, no others supported
    {
        qCWarning(LIBKOOKASCAN_LOG) << "Only bit depths 1 or 8 supported!";
        return (ScanImage::None);
    }
}


//  Create a new image to receive the scan or preview
KScanDevice::Status KScanDevice::createNewImage(const SANE_Parameters *p)
{
    QImage::Format fmt;
    ScanImage::ImageType itype = getImageFormat(p);	// what format should this be?
    switch (itype)					// choose QImage option for that
    {
default:
case ScanImage::None:		return (KScanDevice::ParamError);
case ScanImage::BlackWhite:	fmt = QImage::Format_Mono;		break;
case ScanImage::Greyscale:	fmt = QImage::Format_Indexed8;		break;
case ScanImage::HighColour:	fmt = QImage::Format_RGB32;		break;
    }

    // mScanImage is the master allocated image for the result of the scan.
    // Once the QSharedPointer has been reset() to point to the image, it
    // must only be copied or passed around using the QSharedPointer.  The
    // image will automatically be deleted when the last QSharedPointer to
    // it is clear()'ed or destroyed.
    //
    // An image can also be allocated in acquireScan() in virtual scanner
    // mode.  The same restriction applies.
    ScanImage *newImage = new ScanImage(p->pixels_per_line, p->lines, fmt);
    if (newImage==nullptr) return (KScanDevice::NoMemory);

    mScanImage.clear();					// remove reference to previous
    mScanImage.reset(newImage);				// capture the new image pointer
    mScanImage->setImageType(itype);			// remember the image format

    if (itype==ScanImage::BlackWhite)			// line art (bitmap)
    {
        mScanImage->setColor(0,qRgb(0x00,0x00,0x00));	// set black/white palette
        mScanImage->setColor(1,qRgb(0xFF,0xFF,0xFF));
    }
    else if (itype==ScanImage::Greyscale)		// 8 bit grey
    {							// set grey scale palette
        for (int i = 0; i<256; i++) mScanImage->setColor(i,qRgb(i,i,i));
    }

    return (KScanDevice::Ok);
}


//  Acquiring preview/scan image
//  ----------------------------

KScanDevice::Status KScanDevice::acquirePreview(bool forceGray, int dpi)
{
    KScanOptSet savedOptions("SavedForPreview");

    /* set Preview = ON if exists */
    KScanOption *prev = getOption(SANE_NAME_PREVIEW, false);
    if (prev!=nullptr)
    {
        prev->set(true);
        prev->apply();
        prev->set(false);				// Ensure that it gets restored
        savedOptions.backupOption(prev);		// back to 'false' after previewing
    }

    // TODO: this block doesn't make sense (set the option to true if
    // it is already true?)
    /* Gray-Preview only done by widget? */
    KScanOption *so = getOption(SANE_NAME_GRAY_PREVIEW, false);
    if (so!=nullptr)
    {
        if (so->get()=="true")
        {
            /* Gray preview on */
            so->set(true);
            qCDebug(LIBKOOKASCAN_LOG) << "Setting GrayPreview ON";
        }
        else
        {
            so->set(false);
            qCDebug(LIBKOOKASCAN_LOG) << "Setting GrayPreview OFF";
        }
        so->apply();
    }

    KScanOption *mode = getOption(SANE_NAME_SCAN_MODE, false);
    if (mode!=nullptr)
    {
        qCDebug(LIBKOOKASCAN_LOG) << "Scan mode before preview is" << mode->get();
        savedOptions.backupOption(mode);
        /* apply if it has a widget, or apply always? */
        if (mode->isGuiElement()) mode->apply();
    }

    /* Some sort of Scan Resolution option should always exist */
    KScanOption *xres = getOption(SANE_NAME_SCAN_X_RESOLUTION, false);
    if (xres==nullptr) xres = getOption(SANE_NAME_SCAN_RESOLUTION, false);
    if (xres!=nullptr)
    {
        qCDebug(LIBKOOKASCAN_LOG) << "Scan resolution before preview is" << xres->get();
        savedOptions.backupOption(xres);

        int preview_dpi = dpi;
        if (dpi==0)					// preview DPI not specified
        {
            double min, max;
            if (!xres->getRange(&min, &max))
            {
                qCDebug(LIBKOOKASCAN_LOG) << "Could not retrieve resolution range!";
                min = 75.0;				// hope every scanner can do this
            }

            preview_dpi = int(min);
            if (preview_dpi<MIN_PREVIEW_DPI) preview_dpi = MIN_PREVIEW_DPI;
            qCDebug(LIBKOOKASCAN_LOG) << "Resolution range" << min << "-" << max << "preview at" << preview_dpi;
        }

        KScanOption *yres = getOption(SANE_NAME_SCAN_Y_RESOLUTION, false);
        if (yres!=nullptr)
        {
            /* if active ? */
            savedOptions.backupOption(yres);
            yres->set(preview_dpi);
            yres->apply();
            yres->get(&mCurrScanResolutionY);
        }
        else mCurrScanResolutionY = 0;

        /* Resolution bind switch? */
        KScanOption *bind = getOption(SANE_NAME_RESOLUTION_BIND, false);
        if (bind!=nullptr)
        {
            /* Switch binding on if available */
            savedOptions.backupOption(bind);
            bind->set(true);
            bind->apply();
        }

        xres->set(preview_dpi);
        xres->apply();

        /* Store the resulting preview for additional image information */
        xres->get(&mCurrScanResolutionX);
        if (mCurrScanResolutionY==0) mCurrScanResolutionY = mCurrScanResolutionX;
    }

    KScanDevice::Status status = acquireData(true);	// perform the preview
    scanFinished(status);				// clean up after scan
    loadOptionSet(&savedOptions);			// restore original scan settings
    return (status);
}


/* Starts scanning
 *  depending on if a filename is given or not, the function tries to open
 *  the file using the Qt-Image-IO or really scans the image.
 */
KScanDevice::Status KScanDevice::acquireScan()
{
    applyAllOptions(true);				// apply priority options
    applyAllOptions(false);				// apply non-priority options

    // Some scanners seem to forget the scan area options after certain
    // other options have been applied (which means that they should be
    // considered to be priority options or flagged as 'reload', but this
    // is not always the case).  The area options do not get applied above
    // because they will have already been applied (so their KScanOption's
    // are clean) when the scan area is selected.  Ensure that they are
    // applied after all other options.
    //
    // Seen with the HP OfficeJet 8610 via HPLIP.
    KScanOption *opt;
    opt = getOption(SANE_NAME_SCAN_TL_X, false); if (opt!=nullptr) opt->apply();
    opt = getOption(SANE_NAME_SCAN_TL_Y, false); if (opt!=nullptr) opt->apply();
    opt = getOption(SANE_NAME_SCAN_BR_X, false); if (opt!=nullptr) opt->apply();
    opt = getOption(SANE_NAME_SCAN_BR_Y, false); if (opt!=nullptr) opt->apply();

    // One of the Scan Resolution parameters should always exist.
    KScanOption *xres = getOption(SANE_NAME_SCAN_X_RESOLUTION, false);
    if (xres==nullptr) xres = getOption(SANE_NAME_SCAN_RESOLUTION, false);
    if (xres!=nullptr)
    {
        xres->get(&mCurrScanResolutionX);

        KScanOption *yres = getOption(SANE_NAME_SCAN_Y_RESOLUTION, false);
        if (yres!=nullptr) yres->get(&mCurrScanResolutionY);
        else mCurrScanResolutionY = mCurrScanResolutionX;
    }

    // Main loop of the individual or batch scan.
    bool first = true;
    if (mScanningAdf) qCDebug(LIBKOOKASCAN_LOG) << "Starting ADF loop";
    const MultiScanOptions::Flags f = mMultiScanOptions.flags();
    if (f & MultiScanOptions::MultiScan) qCDebug(LIBKOOKASCAN_LOG) << "Multi scan options" << qPrintable(mMultiScanOptions.toString());

    while (true)
    {
        KScanDevice::Status stat = acquireData(false);

        // If the ADF is empty the first time through the loop, then it
        // is an error which the user should see.  For subsequent scans,
        // it just means that the scanning is finished.
        if (!first && stat==KScanDevice::AdfNoDoc)
        {
            qCDebug(LIBKOOKASCAN_LOG) << "ADF empty, scan finished";
            stat = KScanDevice::AdfEmpty;
        }

        // Signal the scan status to the application, adjusted for the
        // ADF state as above.  This may in turn adjust the final state.
        // Then, if any error has happened, exit the loop now.
        stat = scanFinished(stat);
        if (stat!=KScanDevice::Ok) return (stat);

        // If doing a single scan, whether from the ADF or flatbed,
        // then exit the loop now.
        if (!(f & MultiScanOptions::MultiScan)) return (stat);

        // If doing a manual or delayed wait, then ask or indicate to
        // the user respectively.
        //
        // TODO: maybe do this at the top of the loop so as to be
        // able to wait or delay the first page of the scan.
        if (f & (MultiScanOptions::ManualWait|MultiScanOptions::DelayWait))
        {
            emit sigScanPauseStart();
            ContinueScanDialog dlg(((f & MultiScanOptions::DelayWait) ? mMultiScanOptions.delay() : 0), nullptr);
            int res = dlg.exec();
            emit sigScanPauseEnd();

            // scanFinished() above will not have called sane_cancel() if the
            // scan status was KScanDevice::Ok, because we may be intending
            // to continue a multiple scan loop.  But, if the user has requested
            // to end or cancel the batch, sane_cancel() needs to be called now.
            if (res!=QDialogButtonBox::Ok)
            {
                ScanDevices::self()->deactivateNetworkProxy();
                sane_cancel(mScannerHandle);
                ScanDevices::self()->reactivateNetworkProxy();
            }

            if (res==QDialogButtonBox::Cancel)
            {
                qCDebug(LIBKOOKASCAN_LOG) << "User cancelled batch";
                return (KScanDevice::Cancelled);
            }
            if (res==QDialogButtonBox::Close)
            {
                qCDebug(LIBKOOKASCAN_LOG) << "Manual end of batch";
                return (KScanDevice::Ok);
            }
        }

        // If not doing an ADF scan, then do not try to loop.  If we do, then
        // the second attempt to scan may fail with a SANE_STATUS_DEVICE_BUSY
        // as the scan carriage returns to its rest position.  This does not
        // affect the scan that has just been completed, but the error will be
        // reported to the user.
        //if (!mScanningAdf) return (stat);
        //
        // TODO: could happen for flatbed multi scan, may need a minimum delay here

        // Otherwise, just note that this is now not the first time through
        // the ADF loop.
        first = false;
    }
}


KScanDevice::Status KScanDevice::acquireScan(const QString &filename)
{
    QFileInfo file(filename);
    if (!file.exists())
    {
        qCWarning(LIBKOOKASCAN_LOG) << "virtual file" << filename << "does not exist";
        return (KScanDevice::ParamError);
    }

    QImage img(filename);
    if (img.isNull())
    {
        qCWarning(LIBKOOKASCAN_LOG) << "virtual file" << filename << "could not load";
        return (KScanDevice::ParamError);
    }

    // See createNewImage() for further information regarding the
    // allocated image and shared pointer.
    ScanImage *newImage = new ScanImage(img);
    if (newImage==nullptr) return (KScanDevice::NoMemory);

    mScanImage.clear();					// remove reference to previous
    mScanImage.reset(newImage);				// capture the new image pointer

    // Copy the resolution from the original image
    mScanImage->setXResolution(DPM_TO_DPI(img.dotsPerMeterX()));
    mScanImage->setYResolution(DPM_TO_DPI(img.dotsPerMeterY()));
    // Set the original image file name as the scanner name
    mScanImage->setScannerName(QFile::encodeName(filename));
    emit sigNewImage(mScanImage);
    return (KScanDevice::Ok);
}


#ifdef DEBUG_PARAMS
static void dumpParams(const QString &msg, const SANE_Parameters *p)
{
    QByteArray formatName("UNKNOWN");
    switch (p->format)
    {
case SANE_FRAME_GRAY:	formatName = "GREY";	break;
case SANE_FRAME_RGB:	formatName = "RGB";	break;
case SANE_FRAME_RED:	formatName = "RED";	break;
case SANE_FRAME_GREEN:	formatName = "GREEN";	break;
case SANE_FRAME_BLUE:	formatName = "BLUE";	break;
    }

    qCDebug(LIBKOOKASCAN_LOG) << qPrintable(msg);
    qCDebug(LIBKOOKASCAN_LOG) << "  format:          " << p->format << "=" << qPrintable(formatName);
    qCDebug(LIBKOOKASCAN_LOG) << "  last_frame:      " << p->last_frame;
    qCDebug(LIBKOOKASCAN_LOG) << "  lines:           " << p->lines;
    qCDebug(LIBKOOKASCAN_LOG) << "  depth:           " << p->depth;
    qCDebug(LIBKOOKASCAN_LOG) << "  pixels_per_line: " << p->pixels_per_line;
    qCDebug(LIBKOOKASCAN_LOG) << "  bytes_per_line:  " << p->bytes_per_line;
}
#endif // DEBUG_PARAMS


KScanDevice::Status KScanDevice::startAcquire(ScanImage::ImageType fmt)
{
    // First tell the application that a scan is about to start.
    if (mScanningState==KScanDevice::ScanStarting) emit sigScanStart(fmt);

    // In response to the above (synchronous) signal, the application
    // may have prompted for a file name.  If the user cancels that,
    // it will have called our slotStopScanning() which sets mScanningState
    // to KScanDevice::ScanStopNow.  If that is the case, then finish here.
    if (mScanningState==KScanDevice::ScanStopNow)
    {
        qCDebug(LIBKOOKASCAN_LOG) << "user cancelled before start";
        return (KScanDevice::Cancelled);
    }

    return (KScanDevice::Ok);
}


KScanDevice::Status KScanDevice::acquireData(bool isPreview)
{
    KScanDevice::Status stat = KScanDevice::Ok;
    int frameCount = 0;					// count of frames processed
    int retryCount = 0;					// count of busy retries

#ifdef DEBUG_OPTIONS
    showOptions();					// dump the current options
#endif

    mScanningPreview = isPreview;
    mScanningState = KScanDevice::ScanStarting;
    mBytesRead = 0;
    mBlocksRead = 0;

    ScanGlobal::self()->setScanDevice(this);		// for possible authentication

    // Tell the application that scanning is about to start.
    ScanImage::ImageType fmt = ScanImage::None;
    if (!isPreview)					// scanning to eventually save
    {
        mSaneStatus = sane_get_parameters(mScannerHandle, &mSaneParameters);
        {
#ifdef DEBUG_PARAMS
            dumpParams("Before scan:", &mSaneParameters);
#endif // DEBUG_PARAMS

            if (mSaneParameters.lines>=1 && mSaneParameters.pixels_per_line>0)
            {						// check for a plausible image
                fmt = getImageFormat(&mSaneParameters);	// find format it will have
                if (fmt==ScanImage::None)		// scan format not recognised?
                {
                    stat = KScanDevice::ParamError;	// no point starting scan
                    goto finish;			// clean up anything started
                }
            }
        }

        if (mTestFormat!=ScanImage::None)
        {
            fmt = mTestFormat;
            qCDebug(LIBKOOKASCAN_LOG) << "Testing with image format" << fmt;
        }
    }
    else fmt = ScanImage::Preview;			// special to indicate preview

    // If the scan is from the ADF, then it is first necessary to do the
    // sane_start() in order to detect the "ADF empty" status.  However, doing
    // this in all cases (including non-ADF scans) introduces a significant GUI
    // delay before asking for the file name and/or type, if the user has chosen
    // to do so before scanning.
    //
    // Therefore, if the scan is not from the ADF, then prompt here before doing
    // the sane_start().  If the scan is from the ADF, then delay asking until
    // sane_start() has been done and we know that the ADF is not empty.
    //
    // This is necessary because doing sane_start() seems to be the only way
    // to find out whether the ADF has documents loaded.
    if (!mScanningAdf)
    {
        stat = startAcquire(fmt);
        if (stat!=KScanDevice::Ok) goto finish;
    }

    ScanDevices::self()->deactivateNetworkProxy();
    while (true)					// loop while frames available
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        mSaneStatus = sane_start(mScannerHandle);	// potential lengthy operation
        QApplication::restoreOverrideCursor();

        if (mSaneStatus==SANE_STATUS_NO_DOCS)
        {
            qCDebug(LIBKOOKASCAN_LOG) << "ADF is empty";
            mScanningState = KScanDevice::ScanStopAdfEmpty;
            break;
        }
        else if (mSaneStatus==SANE_STATUS_ACCESS_DENIED)
        {						// authentication failed?
            qCDebug(LIBKOOKASCAN_LOG) << "retrying authentication";
            clearSavedAuth();				// clear any saved password
            mSaneStatus = sane_start(mScannerHandle);	// try again once more
        }

        if (mSaneStatus==SANE_STATUS_GOOD)
        {
            if (mScanningAdf)
            {
                // If the ADF is empty, that will have been detected above.
                // Therefore here we know that the ADF is not empty and so the
                // application can now ask for a file name and/or type if
                // necessary.
                stat = startAcquire(fmt);
                if (stat!=KScanDevice::Ok) goto finish;
            }

            mSaneStatus = sane_get_parameters(mScannerHandle, &mSaneParameters);
            if (mSaneStatus==SANE_STATUS_GOOD)
            {
#ifdef DEBUG_PARAMS
                dumpParams(QString("For frame %1:").arg(frameCount+1), &mSaneParameters);
#endif // DEBUG_PARAMS

                // TODO: implement "Hand Scanner" support
                if (mSaneParameters.lines<1)
                {
                    qCWarning(LIBKOOKASCAN_LOG) << "Hand Scanner not supported";
                    stat = KScanDevice::NotSupported;
                }
                else if (mSaneParameters.pixels_per_line==0)
                {
                    qCWarning(LIBKOOKASCAN_LOG) << "Nothing to acquire!";
                    stat = KScanDevice::EmptyPic;
                }
            }
            else
            {
                stat = KScanDevice::OpenDevice;
                qCDebug(LIBKOOKASCAN_LOG) << "sane_get_parameters() error" << lastSaneErrorMessage();
            }
        }
        else
        {
            // By observation, at least with the Brother MFC-J4540 and the "escl"
            // driver, if the scan source is the flatbed it is necessary to do a
            // sane_cancel() here in order to be ready for the next scan.  If
            // this is not done the next sane_start() will immediately return a
            // SANE_STATUS_DEVICE_BUSY.  However, this does not seem to be
            // necessary if scanning from the ADF.  We do not want to do a
            // sane_cancel() here if it is not necessary because it may have
            // side effects, for example on the HP OfficeJet 8610 with the
            // "hpaio" driver it has the effect of feeding all remaining
            // documents through the ADF and discarding them.
            //
            // Even if a sane_cancel() is needed, we do not want to get into
            // an uncancellable loop if there is some other problem.  So
            // limit the number of retries and introduce a delay between them.
            if (mSaneStatus==SANE_STATUS_DEVICE_BUSY && !mScanningAdf)
            {
                ++retryCount;
                qCDebug(LIBKOOKASCAN_LOG) << "sane_start() error busy, retry" << retryCount;
                if (retryCount<=BUSY_RETRIES)
                {
                    qCDebug(LIBKOOKASCAN_LOG) << "doing sane_cancel()";
                    sane_cancel(mScannerHandle);

                    QTimer delayTimer;
                    delayTimer.setSingleShot(true);
                    delayTimer.start(BUSY_DELAY);
                    while (delayTimer.isActive()) QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                    qCDebug(LIBKOOKASCAN_LOG) << "retrying";
                    continue;
                }
            }

            stat = KScanDevice::OpenDevice;
            qCDebug(LIBKOOKASCAN_LOG) << "sane_start() error" << lastSaneErrorMessage();
        }

        if (stat==KScanDevice::Ok && mScanningState==KScanDevice::ScanStarting)
        {						// first time through loop
            // Create image to receive scan, based on real SANE parameters
            stat = createNewImage(&mSaneParameters);

            // Create/reinitialise buffer for scanning one line
            if (stat==KScanDevice::Ok)
            {
                if (mScanBuf!=nullptr) delete [] mScanBuf;
                mScanBuf = new SANE_Byte[mSaneParameters.bytes_per_line+4];
                if (mScanBuf==nullptr) stat = KScanDevice::NoMemory;
            }						// can this ever happen?

            if (stat==KScanDevice::Ok)
            {
                int fd = 0;
                // Don't assume that sane_get_select_fd() will succeed even if
                // sane_set_io_mode() successfully sets I/O mode to noblocking -
                // bug 159300
                if (sane_set_io_mode(mScannerHandle, SANE_TRUE)==SANE_STATUS_GOOD)
                {
                    if (sane_get_select_fd(mScannerHandle, &fd)==SANE_STATUS_GOOD)
                    {
                        qCDebug(LIBKOOKASCAN_LOG) << "using read socket notifier";
                        mSocketNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
                        connect(mSocketNotifier, &QSocketNotifier::activated, this, &KScanDevice::doProcessABlock);
                    }
                    else qCDebug(LIBKOOKASCAN_LOG) << "not using socket notifier (sane_get_select_fd() failed)";
                }
                else qCDebug(LIBKOOKASCAN_LOG) << "not using socket notifier (sane_set_io_mode() failed)";
            }
        }

        if (stat!=KScanDevice::Ok)			// some problem getting started
        {
            // Scanning could not start - give up now
            qCDebug(LIBKOOKASCAN_LOG) << "Scanning failed to start, status" << stat;
            break;
        }

        if (mScanningState==KScanDevice::ScanStarting)	// first time through loop
        {
            QApplication::setOverrideCursor(Qt::BusyCursor);
            emit sigAcquireStart();
        }

        emit sigScanProgress(0);			// signal the progress dialog
        qApp->processEvents();				// update the progress window

        mPixelX = 0;
        mPixelY = 0;
        mBytesUsed = 0;

        // Set internal status to indicate scanning in progress.
        // This status might be changed, e.g. by pressing Stop on a
        // GUI dialog displayed during scanning.
        mScanningState = KScanDevice::ScanInProgress;

        // As originally coded, if using the socket notifier
        // sane_get_parameters() was only called once at the beginning of
        // the scan - just after sane_start() above.  If not using the socket
        // notifier on the other hand, sane_get_parameters() was called after
        // each doProcessABlock() in the loop below.
        //
        // According to the SANE documentation text, sane_get_parameters()
        // needs to be called once after sane_start() to get the exact
        // parameters, but not necessarily in the reading loop that just
        // needs to call sane_read() repeatedly.  The diagram above, though,
        // seems to imply that sane_get_parameters() should be done in the
        // reading loop.
        //
        // Doing the sane_get_parameters() just once seems to work with all
        // of the scanners that I have available to test, both using the
        // socket notifier and not.  So that is what we now do, in this
        // much simpler loop.

        while (true)					// loop for one scanned frame
        {
            if (mSocketNotifier!=nullptr)		// using the socket notifier
            {
                qApp->processEvents();			// let it fire away
            }
            else					// not using socket notifier
            {
                doProcessABlock();			// may block while reading
            }
							// exit loop when frame done
            if (mScanningState==KScanDevice::ScanIdle ||
                mScanningState==KScanDevice::ScanStopNow ||
                mScanningState==KScanDevice::ScanNextFrame) break;
        }

        ++frameCount;					// count up this frame
							// no more frames to do, exit loop
        if (mScanningState!=KScanDevice::ScanNextFrame) break;
    }
    ScanDevices::self()->reactivateNetworkProxy();

    if (mScanningState==KScanDevice::ScanStopNow)
    {
        stat = KScanDevice::Cancelled;
    }
    else if (mScanningState==KScanDevice::ScanStopAdfEmpty)
    {
        stat = KScanDevice::AdfNoDoc;
    }
    else
    {
        if (mSaneStatus!=SANE_STATUS_GOOD && mSaneStatus!=SANE_STATUS_EOF)
        {
            stat = KScanDevice::ScanError;
        }
    }

    qCDebug(LIBKOOKASCAN_LOG) << "Scan read" << mBytesRead << "bytes in"
                              << mBlocksRead << "blocks," << frameCount << "frames, status" << stat;

finish:
    return (stat);
}


/* This function calls at least sane_read and converts the read data from the scanner
 * to the qimage.
 * The function needs:
 * QImage img valid
 * the data-buffer  set to a appropriate size
 **/

// TODO: probably needs to be extended for 16-bit scanner support

void KScanDevice::doProcessABlock()
{
    int val,i;
    QRgb col, newCol;

    SANE_Byte *rptr = nullptr;
    SANE_Int bytes_read = 0;
    int chan = 0;
    mSaneStatus = SANE_STATUS_GOOD;
    uchar eight_pix = 0;

    if (mScanningState==KScanDevice::ScanIdle) return;	// scan finished, no more to do
							// block notifications while working
    if (mSocketNotifier!=nullptr) mSocketNotifier->setEnabled(false);

    while (true)
    {
        mSaneStatus = sane_read(mScannerHandle,
                                (mScanBuf+mBytesUsed),
                                mSaneParameters.bytes_per_line,
                                &bytes_read);
        if (mSaneStatus!=SANE_STATUS_GOOD)
        {
            if (mSaneStatus!=SANE_STATUS_EOF)		// this is OK, just stop
            {						// any other error
                qCDebug(LIBKOOKASCAN_LOG) << "sane_read() error" << lastSaneErrorMessage()
                                          << "bytes read" << bytes_read;
            }
            break;
        }

        if (bytes_read<1) break;			// no data, finish loop

        ++mBlocksRead;
	mBytesRead += bytes_read;

        int red = 0;
        int green = 0;
        int blue = 0;

	rptr = mScanBuf;				// start of scan data
	switch (mSaneParameters.format)
	{
case SANE_FRAME_RGB:
            if (mSaneParameters.lines<1) break;
            bytes_read += mBytesUsed;			// die übergebliebenen Bytes dazu
            mBytesUsed = bytes_read % 3;

            for (val = 0; val<((bytes_read-mBytesUsed)/3); val++)
            {
                red   = *rptr++;
                green = *rptr++;
                blue  = *rptr++;

                if (mPixelX>=mSaneParameters.pixels_per_line)
                {					// reached end of a row
                    mPixelX = 0;
                    mPixelY++;
                }
                if (mPixelY<mScanImage->height())	// within image height
                {
		    mScanImage->setPixel(mPixelX, mPixelY, qRgb(red, green, blue));
                }
                mPixelX++;
            }

            for (val = 0; val<mBytesUsed; val++)	// Copy the remaining bytes down
            {						// to the beginning of the block
                *(mScanBuf+val) = *rptr++;
            }
            break;

case SANE_FRAME_GRAY:
            for (val = 0; val<bytes_read ; val++)
            {
                if (mPixelY>=mSaneParameters.lines) break;
                if (mSaneParameters.depth==8)		// Greyscale
                {
		    if (mPixelX>=mSaneParameters.pixels_per_line)
                    {					// reached end of a row
                        mPixelX = 0;
                        mPixelY++;
                    }
		    mScanImage->setPixel(mPixelX, mPixelY, *rptr++);
		    mPixelX++;
                }
                else					// Lineart (bitmap)
                {					// needs to be converted to byte
		    eight_pix = *rptr++;
		    for (i = 0; i<8; i++)
		    {
                        if (mPixelY<mSaneParameters.lines)
                        {
                            chan = (eight_pix & 0x80)>0 ? 0 : 1;
                            eight_pix = eight_pix << 1;
                            mScanImage->setPixel(mPixelX, mPixelY, chan);
                            mPixelX++;
                            if( mPixelX>=mSaneParameters.pixels_per_line)
                            {
                                mPixelX = 0;
                                mPixelY++;
                                break;
                            }
                        }
		    }
                }
            }
            break;

case SANE_FRAME_RED:
case SANE_FRAME_GREEN:
case SANE_FRAME_BLUE:
            for (val = 0; val<bytes_read ; val++)
            {
                if (mPixelX>=mSaneParameters.pixels_per_line)
                {					// reached end of a row
                    mPixelX = 0;
		    mPixelY++;
                }

                if (mPixelY<mSaneParameters.lines)
                {
		    col = mScanImage->pixel(mPixelX, mPixelY);

		    red   = qRed(col);
		    green = qGreen(col);
		    blue  = qBlue(col);
		    chan  = *rptr++;

		    switch (mSaneParameters.format)
		    {
case SANE_FRAME_RED:    newCol = qRgba(chan, green, blue, 0xFF);
                        break;

case SANE_FRAME_GREEN:  newCol = qRgba(red, chan, blue, 0xFF);
                        break;

case SANE_FRAME_BLUE:   newCol = qRgba(red, green, chan, 0xFF);
                        break;

default:                newCol = qRgba(0xFF, 0xFF, 0xFF, 0xFF);
                        break;
		    }
		    mScanImage->setPixel(mPixelX, mPixelY, newCol);
		    mPixelX++;
                }
            }
            break;

default:    qCWarning(LIBKOOKASCAN_LOG) << "Undefined SANE format" << mSaneParameters.format;
            break;
	}						// switch of scan format

	if ((mSaneParameters.lines>0) && ((mSaneParameters.lines*mPixelY)>0))
	{
            int progress =  int((double(MAX_PROGRESS)/mSaneParameters.lines)*mPixelY);
            if (progress<MAX_PROGRESS) emit sigScanProgress(progress);
	}

        if (mScanningState==KScanDevice::ScanStopNow)
        {
            // The "Stop" button in the progress display calls slotStopScanning()
            // which sets mScanningState to ScanStopNow.
            //
            // Not sure how it can ever happen, but an old comment here said and did:
            //
            // This is also hit after the normal finish of the scan. Most probably,
            // the QSocketNotifier fires for a few times after the scan has been
            // cancelled.  Does it matter?  To see it, just uncomment the qDebug msg.
            // mScanningState = KScanDevice::ScanIdle;

            qCDebug(LIBKOOKASCAN_LOG) << "Calling sane_cancel() to stop scan";
            sane_cancel(mScannerHandle);

            // The SANE documentation says that we should now not do anything else
            // until the SANE call in progress or the next to be called, which
            // in this case will be the sane_read() at the top of the loop,
            // returns with a SANE_STATUS_CANCELLED error code.  So just continue
            // the loop.
        }
    }							// end of main loop

    // Here when scanning is finished or has had an error
    if (mSaneStatus==SANE_STATUS_EOF)			// end of scan pass
    {
        if (mSaneParameters.last_frame)			// end of scanning run
        {
            /** Everything is okay, the picture is ready **/
            qCDebug(LIBKOOKASCAN_LOG) << "Last frame reached, scan successful";
            mScanningState = KScanDevice::ScanIdle;
        }
        else
        {
            /** EOF und nicht letzter Frame -> Parameter neu belegen und neu starten **/
            mScanningState = KScanDevice::ScanNextFrame;
            qCDebug(LIBKOOKASCAN_LOG) << "EOF, but another frame to scan";
        }
    }
    else if (mSaneStatus==SANE_STATUS_CANCELLED)
    {
        // In debug messages in this file, always refer to a SANE status code
        // as "SANE status" in order to distinguish it from a KScanDevice::Status
        // value referred to as "status".
        qCDebug(LIBKOOKASCAN_LOG) << "Scan cancelled, SANE status" << mSaneStatus;

        // Set his explicitly in case the "scan cancelled" state has not been
        // noted already - for example, if the scanner has a physical "stop"
        // button which generates an immediate SANE_STATUS_CANCELLED.  The
        // caller will translate the state ScanStopNow into Cancelled.
        mScanningState = KScanDevice::ScanStopNow;
    }
    else if (mSaneStatus!=SANE_STATUS_GOOD)
    {
        qCDebug(LIBKOOKASCAN_LOG) << "Scan error, SANE status" << mSaneStatus;
        mScanningState = KScanDevice::ScanIdle;
    }

    if (mSocketNotifier!=nullptr) mSocketNotifier->setEnabled(true);
}


KScanDevice::Status KScanDevice::scanFinished(KScanDevice::Status stat)
{
    qCDebug(LIBKOOKASCAN_LOG) << "status" << stat;

    emit sigScanProgress(MAX_PROGRESS);
    QApplication::restoreOverrideCursor();

    if (mSocketNotifier!=nullptr)			// clean up if in use
    {
	delete mSocketNotifier;
	mSocketNotifier = nullptr;
    }

    if (mScanBuf!=nullptr)
    {
	delete[] mScanBuf;
	mScanBuf = nullptr;
    }

    // If the scan succeeded, finish off the result image and tell the
    // application that it is ready.
    if (stat==KScanDevice::Ok && !mScanImage.isNull())
    {
	mScanImage->setXResolution(mCurrScanResolutionX);
	mScanImage->setYResolution(mCurrScanResolutionY);
	mScanImage->setScannerName(mScannerName);

	if (mScanningPreview)
	{
	    savePreviewImage(*mScanImage);
	    emit sigNewPreview(mScanImage);
	}
	else
	{
	    emit sigNewImage(mScanImage);
	}

        // The signal above will have been delivered to the destination plugin's
        // imageScanned() via KookaView::slotNewImageScanned().  If the plugin
        // cannot accept the image, or if there is a user interaction there which
        // they cancelled, this will have called our slotStopScanning() which will
        // have set mScanningState to KScanDevice::ScanStopNow.  If this happened
        // then treat it as if the scan was cancelled, but first call sane_cancel()
        // below because nothing else will have done.
        if (mScanningState==KScanDevice::ScanStopNow)
        {
            qCDebug(LIBKOOKASCAN_LOG) << "destination did not accept image";
            stat = KScanDevice::NotSaved;
        }
    }

    if (stat!=KScanDevice::Ok && stat!=KScanDevice::Cancelled)
    {
        // sane_cancel() was originally called unconditionally here, even for
        // normal scan termination.  However, it seems to have side effects,
        // such as feeding through anything remaining in the ADF even if only
        // one page has been requested to be scanned.  So do not call it if
        // the scanning status was 'Ok' or if it is 'Cancelled' - in the second
        // case sane_cancel() will have already been called by doProcessABlock().
        // But do call it if there has been any other error, or if the ADF has
        // fed through all its documents and is empty.
        ScanDevices::self()->deactivateNetworkProxy();
        qCDebug(LIBKOOKASCAN_LOG) << "calling sane_cancel() for status" << stat;
        sane_cancel(mScannerHandle);
        ScanDevices::self()->reactivateNetworkProxy();

    }

    // This status is never reported to the outside.
    if (stat==KScanDevice::NotSaved) stat = KScanDevice::Cancelled;

    // Tell the application that the scan has finished.
    emit sigScanFinished(stat);

    // Nothing needs to be done to clean up the image that was allocated
    // in createNewImage(), the QSharedPointer will take care of reference
    // counting and deletion.

    mScanningState = KScanDevice::ScanIdle;
    return (stat);					// may have been updated
}


//  Configuration
//  -------------

static void writeFlag(KConfigGroup &grp, const KConfigSkeletonItem *item, MultiScanOptions::Flags value)
{
    // Ensure that flag bits are written as Boolean values.
    grp.writeEntry(item->key(), static_cast<bool>(value));
}


/* private */ void KScanDevice::saveStartupConfig()
{
    if (mScannerName.isEmpty()) return;			// do not save for no scanner

    saveOptions(KScanOptSet::Params);			// save the SANE parameters
							// save the multiple scan options
    KConfigGroup grp = KScanOptSet::configGroup(mScannerName, KScanOptSet::Options);

    const MultiScanOptions::Flags f = mMultiScanOptions.flags();
    writeFlag(grp, ScanSettings::self()->multiScanItem(), (f & MultiScanOptions::MultiScan));
    writeFlag(grp, ScanSettings::self()->multiRotateOddItem(), (f & MultiScanOptions::RotateOdd));
    writeFlag(grp, ScanSettings::self()->multiRotateEvenItem(), (f & MultiScanOptions::RotateEven));
    // RotateBoth can be derived from the above two
    // AdfAvailable does not need to be saved
    writeFlag(grp, ScanSettings::self()->multiManualWaitItem(), (f & MultiScanOptions::ManualWait));
    writeFlag(grp, ScanSettings::self()->multiDelayWaitItem(), (f & MultiScanOptions::DelayWait));
    writeFlag(grp, ScanSettings::self()->multiBatchMultipleItem(), (f & MultiScanOptions::BatchMultiple));
    writeFlag(grp, ScanSettings::self()->multiAutoGenerateItem(), (f & MultiScanOptions::AutoGenerate));
    // Source does not need to be saved, the SANE parameter mirrors it

    const KConfigSkeletonItem *item = ScanSettings::self()->multiScanDelayItem();
    grp.writeEntry(item->key(), mMultiScanOptions.delay());
    grp.sync();
}


static bool readFlag(const KConfigGroup &grp, const KConfigSkeletonItem *item)
{
    return (grp.readEntry(item->key(), item->getDefault().toBool()));
}


bool KScanDevice::loadStartupConfig()
{
    // Load the startup options applicable to the current scanner
    qCDebug(LIBKOOKASCAN_LOG) << "looking for startup options";
    if (!loadOptions(KScanOptSet::Params))
    {
        qCDebug(LIBKOOKASCAN_LOG) << "no startup options to load";
        return (false);
    }

    const KConfigGroup grp = KScanOptSet::configGroup(mScannerName, KScanOptSet::Options);

    mMultiScanOptions.setFlags(MultiScanOptions::MultiScan, readFlag(grp, ScanSettings::self()->multiScanItem()));
    mMultiScanOptions.setFlags(MultiScanOptions::RotateOdd, readFlag(grp, ScanSettings::self()->multiRotateOddItem()));
    mMultiScanOptions.setFlags(MultiScanOptions::RotateEven, readFlag(grp, ScanSettings::self()->multiRotateEvenItem()));
    // RotateBoth can be derived from the above two
    // AdfAvailable does not need to be loaded
    mMultiScanOptions.setFlags(MultiScanOptions::ManualWait, readFlag(grp, ScanSettings::self()->multiManualWaitItem()));
    mMultiScanOptions.setFlags(MultiScanOptions::DelayWait, readFlag(grp, ScanSettings::self()->multiDelayWaitItem()));
    mMultiScanOptions.setFlags(MultiScanOptions::BatchMultiple, readFlag(grp, ScanSettings::self()->multiBatchMultipleItem()));
    mMultiScanOptions.setFlags(MultiScanOptions::AutoGenerate, readFlag(grp, ScanSettings::self()->multiAutoGenerateItem()));
    // Source does not need to be loaded, the SANE parameter mirrors it

    const KConfigSkeletonItem *item = ScanSettings::self()->multiScanDelayItem();
    mMultiScanOptions.setDelay(grp.readEntry(item->key(), item->getDefault().toUInt()));

    qCDebug(LIBKOOKASCAN_LOG) << "loaded startup options";
    return (true);
}


/* private */ void KScanDevice::loadOptionSetInternal(const KScanOptSet *optSet, bool prio)
{
    for (KScanOptSet::const_iterator it = optSet->constBegin();
         it!=optSet->constEnd(); ++it)
    {
        const QByteArray name = it.key();
        if (!optionExists(name)) continue;		// only for options that exist

        KScanOption *so = getOption(name, false);
        if (so==nullptr) continue;			// we don't have this option
        if (so->isGroup()) continue;			// nothing to do here
        if (so->isPriorityOption() ^ prio) continue;	// check whether requested priority

        so->set(it.value());
        if (so->isInitialised() && so->isSoftwareSettable() && so->isActive()) so->apply();
        if (so->getName()==SANE_NAME_SCAN_SOURCE) updateAdfState(so);
    }
}


/* private */ void KScanDevice::loadOptionSet(const KScanOptSet *optSet)
{
    if (optSet==nullptr) return;

    qCDebug(LIBKOOKASCAN_LOG) << "Loading set" << optSet->getSetName() << "with" << optSet->count() << "options";
    loadOptionSetInternal(optSet, true);
    loadOptionSetInternal(optSet, false);
}


bool KScanDevice::loadOptions(KScanOptSet::Category category, const QString &setName)
{
    KScanOptSet optSet(!setName.isEmpty() ? setName : mScannerName);

    if (!optSet.loadConfig(category, mScannerName)) return (false);
    loadOptionSet(&optSet);
    return (true);
}


bool KScanDevice::saveOptions(KScanOptSet::Category category, const QString &setName) const
{
    KScanOptSet optSet(!setName.isEmpty() ? setName : mScannerName);
    getCurrentOptions(&optSet);
    // No need for I18N, this is not a user visible string
    optSet.setDescription("Default startup configuration");
    return (optSet.saveConfig(category, mScannerName));
}


// Retrieve the current options from the scanner, i.e. all of those that
// have an associated GUI element and also any others (e.g. the TL_X and
// other scan area settings) that have been apply()'ed but do not have
// a GUI.

void KScanDevice::getCurrentOptions(KScanOptSet *optSet) const
{
    if (optSet==nullptr) return;

    for (OptionHash::const_iterator it = mCreatedOptions.constBegin();
         it!=mCreatedOptions.constEnd(); ++it)
    {
        KScanOption *so = it.value();
        if (!so->isReadable()) continue;

        if (so->isGuiElement() || so->isApplied())
        {
            if (so->isActive()) optSet->backupOption(so);
            so->setApplied(false);
        }
    }
}


//  SANE Authentication
//  -------------------
//
//  According to the SANE documentation, this may be requested for any use of
//  sane_open(), sane_control_option() or sane_start() on a scanner device
//  that requires authentication.
//
//  The only uses of sane_open() and sane_start() are here in this file, and
//  they set the current scanner using setScanDevice() before performing the
//  SANE operation.
//
//  This does not happen for all uses of sane_control_option(), either here or
//  in KScanOption, so there is a slight possibility that if authentication is
//  needed for those (and has not been previously requested by sane_open() or
//  sane_start()) then it will use the wrong scanner device or will not prompt
//  at all.  However, Kooka only supports one scanner open at a time, and does
//  sane_open() before any use of sane_control_option().  So hopefully this
//  will not be a problem.

bool KScanDevice::authenticate(QByteArray *retuser, QByteArray *retpass)
{
    qCDebug(LIBKOOKASCAN_LOG) << "for" << mScannerName;

    // TODO: use KWallet for username/password?
    KConfigGroup grp = KScanOptSet::configGroup(mScannerName, KScanOptSet::Options);
    QByteArray user = QByteArray::fromBase64(grp.readEntry("user", QString()).toLocal8Bit());
    QByteArray pass = QByteArray::fromBase64(grp.readEntry("pass", QString()).toLocal8Bit());

    if (!user.isEmpty() && !pass.isEmpty())
    {
        qCDebug(LIBKOOKASCAN_LOG) << "have saved username/password";
    }
    else
    {
        qCDebug(LIBKOOKASCAN_LOG) << "asking for username/password";

        KPasswordDialog dlg(nullptr, KPasswordDialog::ShowKeepPassword|KPasswordDialog::ShowUsernameLine);
        dlg.setPrompt(xi18nc("@info", "The scanner<nl/><icode>%1</icode><nl/>requires authentication.", mScannerName.constData()));
        dlg.setWindowTitle(i18n("Scanner Authentication"));

        if (!user.isEmpty()) dlg.setUsername(user);
        if (!pass.isEmpty()) dlg.setPassword(pass);

        if (!dlg.exec()) return (false);

        user = dlg.username().toLocal8Bit();
        pass = dlg.password().toLocal8Bit();
        if (dlg.keepPassword())
        {
            grp.writeEntry("user", user.toBase64());
            grp.writeEntry("pass", pass.toBase64());
        }
    }

    *retuser = user;
    *retpass = pass;
    return (true);
}


void KScanDevice::clearSavedAuth()
{
    KConfigGroup grp = KScanOptSet::configGroup(mScannerName, KScanOptSet::Options);
    grp.deleteEntry("user");
    grp.deleteEntry("pass");
    grp.sync();
}


//  Error reporting
//  ---------------

QString KScanDevice::lastSaneErrorMessage() const
{
    return (sane_strstatus(mSaneStatus));
}


QString KScanDevice::statusMessage(KScanDevice::Status stat)
{
    switch (stat)
    {
case KScanDevice::Ok:			return (i18n("OK"));		// shouldn't be reported
case KScanDevice::NoDevice:		return (i18n("No device"));	// never during scanning
case KScanDevice::ParamError:		return (i18n("Bad parameter"));
case KScanDevice::OpenDevice:		return (i18n("Cannot open device"));
case KScanDevice::ControlError:		return (i18n("sane_control_option() failed"));
case KScanDevice::ScanError:		return (i18n("Error while scanning"));
case KScanDevice::EmptyPic:		return (i18n("Empty picture"));
case KScanDevice::NoMemory:		return (i18n("Out of memory"));
case KScanDevice::Reload:		return (i18n("Needs reload"));	// never during scanning
case KScanDevice::Cancelled:		return (i18n("Cancelled"));	// shouldn't be reported
case KScanDevice::OptionNotActive:	return (i18n("Not active"));	// never during scanning
case KScanDevice::NotSupported:		return (i18n("Not supported"));
case KScanDevice::AdfNoDoc:		return (i18n("ADF no document loaded"));
case KScanDevice::AdfEmpty:		return (i18n("ADF empty"));	// shouldn't be reported
case KScanDevice::NotSaved:		return (i18n("Not accepted"));	// shouldn't be reported
default:				return (i18n("Unknown status %1", stat));
    }
}
