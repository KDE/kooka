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

#include <klocalizedstring.h>
#include <kconfig.h>
#include <kpassworddialog.h>

#include "scanglobal.h"
#include "scandevices.h"
#include "kgammatable.h"
#include "kscancontrols.h"
#include "kscanoption.h"
#include "kscanoptset.h"
#include "deviceselector.h"
#include "scansettings.h"
#include "libkookascan_logging.h"

extern "C" {
#include <sane/saneopts.h>
}

#define MIN_PREVIEW_DPI		75
#define MAX_PROGRESS		100


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


KScanOption *KScanDevice::getOption(const QByteArray &name, bool create /* = true */)
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

    if (!optionExists(alias))
    {
#ifdef DEBUG_CREATE
        qCDebug(LIBKOOKASCAN_LOG) << "unknown option" << alias;
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
        if (mScanningState!=KScanDevice::ScanIdle)
        {
            qCDebug(LIBKOOKASCAN_LOG) << "Scanner is still active, calling sane_cancel()";
            sane_cancel(mScannerHandle);
        }
        sane_close(mScannerHandle);			// close the SANE device
        mScannerHandle = nullptr;			// scanner no longer open
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
    // This assumes that the TL_X and TL_Y options (defining the top left
    // of the scan area) have the same range as BR_X and BR_Y (defining the
    // bottom right).  There is also a general assumption everywhere that
    // the minimum range value for all four of these options is zero.  So
    // it only necessary to look at the maximum range values of these two
    // options.
    KScanOption *so_w = getOption(SANE_NAME_SCAN_BR_X);
    KScanOption *so_h = getOption(SANE_NAME_SCAN_BR_Y);

    // The built in "pnm" device does not support a settable scan area.
    if (so_w==nullptr || so_h==nullptr)
    {
        qCDebug(LIBKOOKASCAN_LOG) << "Scanner does not support area setting";
        return (QSize());
    }

    QSize s;
    double min, max;

    so_w->getRange(&min, &max);
    s.setWidth(qRound(max));

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
    }

    if (!reload)					// need to reload now?
    {
#ifdef DEBUG_RELOAD
        qCDebug(LIBKOOKASCAN_LOG) << "Reload of others not needed";
#endif // DEBUG_RELOAD
        return;
    }
							// reload of all others needed
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
static ScanImage::ImageType getScanFormat(const SANE_Parameters *p)
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


// Get the QImage format to be used for a scan image type.
static QImage::Format getImageFormat(ScanImage::ImageType stype)
{
    switch (stype)					// choose QImage option for that
    {
default:
case ScanImage::None:		return (QImage::Format_Invalid);
case ScanImage::BlackWhite:	return (QImage::Format_Mono);
case ScanImage::Greyscale:
case ScanImage::LowColour:	return (QImage::Format_Indexed8);
case ScanImage::HighColour:	return (QImage::Format_RGB32);
    }
}


//  Create a new image to receive the scan or preview.
KScanDevice::Status KScanDevice::createNewImage(const SANE_Parameters *p)
{
    const ScanImage::ImageType itype = getScanFormat(p);
    const QImage::Format fmt = getImageFormat(itype);
    if (fmt==QImage::Format_Invalid) return (KScanDevice::ParamError);

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
    loadOptionSet(&savedOptions);			// restore original scan settings
    return (status);
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
    }
}


/* Starts scanning
 *  depending on if a filename is given or not, the function tries to open
 *  the file using the Qt-Image-IO or really scans the image.
 */
KScanDevice::Status KScanDevice::acquireScan(const QString &filename)
{
    if (filename.isEmpty())				// real scan
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

        return (acquireData(false));			// perform the scan
    }
    else						// virtual scan from image file
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

        mScanImage.clear();				// remove reference to previous
        mScanImage.reset(newImage);			// capture the new image pointer

        // Copy the resolution from the original image
        mScanImage->setXResolution(DPM_TO_DPI(img.dotsPerMeterX()));
        mScanImage->setYResolution(DPM_TO_DPI(img.dotsPerMeterY()));
        // Set the original image file name as the scanner name
        mScanImage->setScannerName(QFile::encodeName(filename));
        emit sigNewImage(mScanImage);
        return (KScanDevice::Ok);
    }
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

    qCDebug(LIBKOOKASCAN_LOG) << msg.toLatin1().constData();
    qCDebug(LIBKOOKASCAN_LOG) << "  format:          " << p->format << "=" << formatName.constData();
    qCDebug(LIBKOOKASCAN_LOG) << "  last_frame:      " << p->last_frame;
    qCDebug(LIBKOOKASCAN_LOG) << "  lines:           " << p->lines;
    qCDebug(LIBKOOKASCAN_LOG) << "  depth:           " << p->depth;
    qCDebug(LIBKOOKASCAN_LOG) << "  pixels_per_line: " << p->pixels_per_line;
    qCDebug(LIBKOOKASCAN_LOG) << "  bytes_per_line:  " << p->bytes_per_line;
}
#endif // DEBUG_PARAMS


KScanDevice::Status KScanDevice::acquireData(bool isPreview)
{
    KScanDevice::Status stat = KScanDevice::Ok;
    int frames = 0;

#ifdef DEBUG_OPTIONS
    showOptions();					// dump the current noptions
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
        if (mSaneStatus==SANE_STATUS_GOOD)		// get pre-scan parameters
        {
#ifdef DEBUG_PARAMS
            dumpParams("Before scan:", &mSaneParameters);
#endif // DEBUG_PARAMS

            if (mSaneParameters.lines>=1 && mSaneParameters.pixels_per_line>0)
            {						// check for a plausible image
                fmt = getScanFormat(&mSaneParameters);	// find format it will have
                if (fmt==ScanImage::None)		// scan format not recognised?
                {
                    stat = KScanDevice::ParamError;	// no point starting scan
                    goto finish2;;			// clean up anything started
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
    emit sigScanStart(fmt);				// now tell the application

    ScanDevices::self()->deactivateNetworkProxy();

    // The application may have prompted for a file name.
    // If the user cancelled that, it will have called our
    // slotStopScanning() which sets mScanningState to
    // KScanDevice::ScanStopNow.  If that is the case,
    // then finish here.
    if (mScanningState==KScanDevice::ScanStopNow)
    {							// user cancelled save dialogue
        qCDebug(LIBKOOKASCAN_LOG) << "user cancelled before start";
        stat = KScanDevice::Cancelled;
        goto finish;					// clean up anything started
    }

    while (true)					// loop while frames available
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
							// potential lengthy operation
        mSaneStatus = sane_start(mScannerHandle);
        if (mSaneStatus==SANE_STATUS_ACCESS_DENIED)	// authentication failed?
        {
            qCDebug(LIBKOOKASCAN_LOG) << "retrying authentication";
            clearSavedAuth();				// clear any saved password
            mSaneStatus = sane_start(mScannerHandle);	// try again once more
        }

        if (mSaneStatus==SANE_STATUS_GOOD)
        {
            mSaneStatus = sane_get_parameters(mScannerHandle, &mSaneParameters);
            if (mSaneStatus==SANE_STATUS_GOOD)
            {
#ifdef DEBUG_PARAMS
                dumpParams(QString("For frame %1:").arg(frames+1), &mSaneParameters);
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
            stat = KScanDevice::OpenDevice;
            qCDebug(LIBKOOKASCAN_LOG) << "sane_start() error" << lastSaneErrorMessage();
        }
        QApplication::restoreOverrideCursor();

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
            goto finish;;				// clean up anything started
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

        ++frames;					// count up this frame
							// more frames to do
        if (mScanningState==KScanDevice::ScanNextFrame) continue;
        break;						// scan done, exit loop
    }

    if (mScanningState==KScanDevice::ScanStopNow)
    {
        stat = KScanDevice::Cancelled;
    }
    else
    {
        if (mSaneStatus!=SANE_STATUS_GOOD && mSaneStatus!=SANE_STATUS_EOF)
        {
            stat = KScanDevice::ScanError;
        }
    }

    qCDebug(LIBKOOKASCAN_LOG) << "Scan read" << mBytesRead << "bytes in"
                              << mBlocksRead << "blocks," << frames << "frames, status" << stat;

finish:
    ScanDevices::self()->reactivateNetworkProxy();
finish2:
    scanFinished(stat);					// scan is now finished
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
            int progress =  int((double(MAX_PROGRESS))/mSaneParameters.lines*mPixelY);
            if (progress<MAX_PROGRESS) emit sigScanProgress(progress);
	}

        // cannot get here, bytes_read and EOF tested above
	//if( bytes_read == 0 || mSaneStatus == SANE_STATUS_EOF )
	//{
	//   //qCDebug(LIBKOOKASCAN_LOG) << "mSaneStatus not OK:" << sane_stat;
	//   break;
	//}

        if (mScanningState==KScanDevice::ScanStopNow)
        {
            /* mScanningState is set to ScanStopNow due to hitting slStopScanning   */
            /* Mostly that one is fired by the STOP-Button in the progress dialog. */

            /* This is also hit after the normal finish of the scan. Most probably,
             * the QSocketnotifier fires for a few times after the scan has been
             * cancelled.  Does it matter ? To see it, just uncomment the qDebug msg.
             */
            //qCDebug(LIBKOOKASCAN_LOG) << "Stopping the scan progress";
            //mScanningState = KScanDevice::ScanIdle;
            break;
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
    else if (mSaneStatus!=SANE_STATUS_GOOD)
    {
        mScanningState = KScanDevice::ScanIdle;
        qCDebug(LIBKOOKASCAN_LOG) << "Scan error or cancelled, status" << mSaneStatus;
    }

    if (mSocketNotifier!=nullptr) mSocketNotifier->setEnabled(true);
}


void KScanDevice::scanFinished(KScanDevice::Status status)
{
    qCDebug(LIBKOOKASCAN_LOG) << "status" << status;

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
    if (status==KScanDevice::Ok && !mScanImage.isNull())
    {
        if (mTestFormat!=ScanImage::None)		// force an image format
        {
            const QImage::Format newFmt = getImageFormat(mTestFormat);
            if (newFmt!=QImage::Format_Invalid && newFmt!=mScanImage->format())
            {
                qCDebug(LIBKOOKASCAN_LOG) << "force converting from format" << mScanImage->format() << "->" << newFmt;
                mScanImage->convertTo(newFmt, Qt::NoOpaqueDetection);
                mScanImage->setImageType(mTestFormat);
            }
        }

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
    }

    // TODO: Should this be called here, even for normal scan termination?
    // It seems to have side effects, such as feeding through anything remaining
    // in the ADF even if only one page has been requested to be scanned.
    ScanDevices::self()->deactivateNetworkProxy();
    sane_cancel(mScannerHandle);
    ScanDevices::self()->reactivateNetworkProxy();

    // Tell the application that the scan has finished.
    emit sigScanFinished(status);

    // Nothing needs to be done to clean up the image that was allocated
    // in createNewImage(), the QSharedPointer will take care of reference
    // counting and deletion.

    mScanningState = KScanDevice::ScanIdle;
}


//  Configuration
//  -------------

void KScanDevice::saveStartupConfig()
{
    if (mScannerName.isNull()) return;			// do not save for no scanner

    KScanOptSet optSet(KScanOptSet::startupSetName());
    getCurrentOptions(&optSet);
    optSet.saveConfig(mScannerName, i18n("Default startup configuration"));
}


void KScanDevice::loadOptionSetInternal(const KScanOptSet *optSet, bool prio)
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
    }
}


void KScanDevice::loadOptionSet(const KScanOptSet *optSet)
{
    if (optSet==nullptr) return;

    qCDebug(LIBKOOKASCAN_LOG) << "Loading set" << optSet->getSetName() << "with" << optSet->count() << "options";
    loadOptionSetInternal(optSet, true);
    loadOptionSetInternal(optSet, false);
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


KConfigGroup KScanDevice::configGroup(const QString &groupName)
{
    Q_ASSERT(!groupName.isEmpty());
    return (ScanSettings::self()->config()->group(groupName));
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
    KConfigGroup grp = configGroup(mScannerName);
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
    KConfigGroup grp = configGroup(mScannerName);
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
default:				return (i18n("Unknown status %1", stat));
    }
}
