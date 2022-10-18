/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
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

#include "previewer.h"

#include <qvector.h>
#include <qtimer.h>
#include <qlayout.h>

#include <kwidgetsaddons_version.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>

#ifdef DEBUG_AUTOSEL
#include <qfile.h>
#include <qtextstream.h>
#endif

#include "imagecanvas.h"
#include "kscandevice.h"
#include "autoselectbar.h"
#include "scansettings.h"
#include "libkookascan_logging.h"


Previewer::Previewer(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("Previewer");

    QVBoxLayout *vbl = new QVBoxLayout(this);
    vbl->setMargin(0);

    /* Units etc. TODO: get from Config */
    mDisplayUnit = KRuler::Millimetres;
    mAutoSelThresh = 25;				// not used until scanner connected

    mScanDevice = nullptr;                 // no scanner connected yet
    mBedHeight = 297;                   // for most A4/Letter scanners
    mBedWidth = 215;

    // Image viewer
    mCanvas  = new ImageCanvas(this);
    mCanvas->setDefaultScaleType(ImageCanvas::ScaleDynamic);
    vbl->addWidget(mCanvas);

    /*Signals: Control the custom-field and show size of selection */
    connect(mCanvas, QOverload<const QRectF &>::of(&ImageCanvas::newRect), this, &Previewer::slotNewAreaSelected);

    mAutoSelectBar = new AutoSelectBar(mAutoSelThresh, this);
    connect(mAutoSelectBar, &AutoSelectBar::thresholdChanged, this, &Previewer::slotSetAutoSelThresh);
    connect(mAutoSelectBar, &AutoSelectBar::advancedSettingsChanged, this, &Previewer::slotAutoSelectSettingsChanged);
    connect(mAutoSelectBar, &AutoSelectBar::performSelection, this, &Previewer::slotFindAutoSelection);
    vbl->addWidget(mAutoSelectBar);

    mScanResX = -1;
    mScanResY = -1;
    mBytesPerPix = -1;

    mSelectionWidthMm = mBedWidth;
    mSelectionHeightMm = mBedHeight;
    updateSelectionDims();
    setAutoSelection(false);				// initially disable, no scanner
}


bool Previewer::setPreviewImage(ScanImage::Ptr image)
{
    if (image.isNull()) return (false);

    qCDebug(LIBKOOKASCAN_LOG) << "setting new image, size" << image->size();
    mCanvas->newImage(image);
    return (true);
}


void Previewer::newImage(ScanImage::Ptr image)
{
    if (image.isNull()) return;

    resetAutoSelection();				// reset for new image
    mCanvas->newImage(image);				// set image on canvas
    slotFindAutoSelection();				// auto-select if required
    slotNotifyAutoSelectChanged();			// tell the GUI
}


void Previewer::setScannerBedSize(int w, int h)
{
    //qCDebug(LIBKOOKASCAN_LOG) << "to [" << w << "," << h << "]";

    mBedWidth = w;
    mBedHeight = h;

    slotNewCustomScanSize(QRect());			// reset selection and display
}

void Previewer::setDisplayUnit(KRuler::MetricStyle unit)
{
    // TODO: this is not used
    mDisplayUnit = unit;
}

// Signal sent from ScanParams, selection chosen by user
// from scan size combo (specified in integer millimetres)
void Previewer::slotNewCustomScanSize(const QRect &rect)
{
    //qCDebug(LIBKOOKASCAN_LOG) << "rect" << rect;

    QRect r = rect;
    QRectF newRect;
    if (r.isValid()) {
        mSelectionWidthMm = r.width();
        mSelectionHeightMm = r.height();

        newRect.setLeft(double(r.left()) / mBedWidth);  // convert mm -> bedsize factor
        newRect.setWidth(double(r.width()) / mBedWidth);
        newRect.setTop(double(r.top()) / mBedHeight);
        newRect.setHeight(double(r.height()) / mBedHeight);
    } else {
        mSelectionWidthMm = mBedWidth;
        mSelectionHeightMm = mBedHeight;
    }

    mCanvas->setSelectionRect(newRect);
    updateSelectionDims();
}

void Previewer::slotNewScanResolutions(int xres, int yres)
{
    qCDebug(LIBKOOKASCAN_LOG) << "resolution" << xres << "x" << yres;

    mScanResX = xres;
    mScanResY = yres;
    updateSelectionDims();
}

void Previewer::slotNewScanMode(int bytes_per_pix)
{
    qCDebug(LIBKOOKASCAN_LOG) << "bytes per pix" << bytes_per_pix;

    mBytesPerPix = bytes_per_pix;
    updateSelectionDims();
}

//  Signal sent from ImageCanvas, the user has drawn a new selection box
//  (specified relative to the preview image size).  This is converted
//  to millimetres and re-emitted as signal newPreviewRect().
void Previewer::slotNewAreaSelected(const QRectF &rect)
{
    qCDebug(LIBKOOKASCAN_LOG) << "rect" << rect << "width" << mBedWidth << "height" << mBedHeight;

    if (rect.isValid()) {
        // convert bedsize -> mm
        QRect r;
        r.setLeft(qRound(rect.left()*mBedWidth));
        r.setWidth(qRound(rect.width()*mBedWidth));
        r.setTop(qRound(rect.top()*mBedHeight));
        r.setHeight(qRound(rect.height()*mBedHeight));
        //qCDebug(LIBKOOKASCAN_LOG) << "new rect" << r;
        emit newPreviewRect(r);

        mSelectionWidthMm = r.width();
        mSelectionHeightMm = r.height();
    } else {
        emit newPreviewRect(QRect());           // full scan area

        mSelectionWidthMm = mBedWidth;          // for size display
        mSelectionHeightMm = mBedHeight;
    }

    updateSelectionDims();
}

static inline int mmToPixels(double mm, int res)
{
    return (qRound(mm / 25.4 * res));
}

void Previewer::updateSelectionDims()
{
    if (mScanDevice == nullptr) {
        return;    // no scanner connected
    }

    /* Calculate file size */
    if (mScanResX > 1 && mScanResY > 1) {   // if resolution available
        if (mBytesPerPix != -1) {           // depth of scan available?
            int wPix = mmToPixels(mSelectionWidthMm, mScanResX);
            int hPix = mmToPixels(mSelectionHeightMm, mScanResY);

            long size_in_byte;
            if (mBytesPerPix == 0) {        // bitmap scan
                size_in_byte = wPix * hPix / 8;
            } else {                // grey or colour scan
                size_in_byte = wPix * hPix * mBytesPerPix;
            }

            emit previewFileSizeChanged(size_in_byte);
        } else {
            emit previewFileSizeChanged(-1);    // depth not available
        }
    }

    QString result = previewInfoString(mSelectionWidthMm, mSelectionHeightMm, mScanResX, mScanResY);
    emit previewDimsChanged(result);
}

QString Previewer::previewInfoString(double widthMm, double heightMm, int resX, int resY)
{
    if (resX > 1 && resY > 1) {         // resolution available
        int wPix = mmToPixels(widthMm, resX);
        int hPix = mmToPixels(heightMm, resY);
        // xgettext:no-c-format
        return (i18nc("@info:status", "%1x%2mm, %3x%4pix", widthMm, heightMm, wPix, hPix));
    } else {                    // resolution not available
        // xgettext:no-c-format
        return (i18nc("@info:status", "%1x%2mm", widthMm, heightMm));
    }
}

void Previewer::connectScanner(KScanDevice *scan)
{
    qCDebug(LIBKOOKASCAN_LOG) << scan->scannerBackendName();
    mScanDevice = scan;
    mCanvas->newImage(nullptr);				// remove previous preview

    if (scan != nullptr) {
        setAutoSelection(false);			// initially off, disregard config
							// but get other saved values
        const KConfigSkeletonItem *item = ScanSettings::self()->previewAutoselBackgroundItem();
        mBgIsWhite = (scan->getConfig<int>(item)!=ScanSettings::BackgroundBlack);

        item = ScanSettings::self()->previewAutoselThresholdItem();
        int val = scan->getConfig<int>(item);
        mAutoSelThresh = val;
        mAutoSelectBar->setThreshold(val);

        item = ScanSettings::self()->previewAutoselDustsizeItem();
        val = scan->getConfig<int>(item);
        mAutoSelDustsize = val;

        item = ScanSettings::self()->previewAutoselMarginItem();
        val = scan->getConfig<int>(item);
        mAutoSelMargin = val;

        //qCDebug(LIBKOOKASCAN_LOG) << "margin" << mAutoSelMargin << "white?" << mBgIsWhite << "dust" << mAutoSelDustsize;
        mAutoSelectBar->setAdvancedSettings(mAutoSelMargin, mBgIsWhite, mAutoSelDustsize);

        updateSelectionDims();
    }
}

void Previewer::slotNotifyAutoSelectChanged()
{
    const bool isAvailable = mCanvas->hasImage();
    const bool isOn = mDoAutoSelection;
    emit autoSelectStateChanged(isAvailable, isOn);
}

void Previewer::resetAutoSelection()
{
    mHeightSum.clear();
    mWidthSum.clear();
}

void Previewer::setAutoSelection(bool isOn)
{
    qCDebug(LIBKOOKASCAN_LOG) << "to" << isOn;

    if (isOn && mScanDevice == nullptr) {			// no scanner connected yet
        qCWarning(LIBKOOKASCAN_LOG) << "no scanner!";
        isOn = false;
    }

    mDoAutoSelection = isOn;
    if (mAutoSelectBar != nullptr) {
        mAutoSelectBar->setVisible(isOn);
    }
    if (mScanDevice != nullptr) {
        const KConfigSkeletonItem *item = ScanSettings::self()->previewAutoselOnItem();
        mScanDevice->storeConfig<bool>(item, isOn);
    }
							// tell the GUI
    QTimer::singleShot(0, this, &Previewer::slotNotifyAutoSelectChanged);
}

/**
 * reads the scanner dependent config file through the mScanDevice pointer.
 * If a value for the scanner is not yet known, the function starts up a
 * popup and asks the user.  The result is stored.
 */

bool Previewer::checkForScannerBg()
{
    if (mScanDevice == nullptr) {
        return (true);    // no scan device
    }

    KConfigSkeletonItem *item = ScanSettings::self()->previewAutoselBackgroundItem();
    int curWhite = mScanDevice->getConfig<int>(item);
    bool goWhite = false;

    if (curWhite==ScanSettings::BackgroundUnknown)	// not yet known, ask the user
    {
        qCDebug(LIBKOOKASCAN_LOG) << "Don't know the scanner background yet";
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
        int res = KMessageBox::questionTwoActionsCancel(this,
#else
        int res = KMessageBox::questionYesNoCancel(this,
#endif
                                                   i18n("The autodetection of images on the preview depends on the background color of the preview image (the result of scanning with no document loaded).\n\nPlease select whether the background of the preview image is black or white."),
                                                   i18nc("@title:window", "Autodetection Background"),
                                                   KGuiItem(i18nc("@action:button Name of colour", "White")),
                                                   KGuiItem(i18nc("@action:button Name of colour", "Black")));
        if (res==KMessageBox::Cancel) return (false);
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
        goWhite = (res==KMessageBox::PrimaryAction);
#else
        goWhite = (res==KMessageBox::Yes);
#endif
    }
    else if (curWhite==ScanSettings::BackgroundWhite) goWhite = true;

    mBgIsWhite = goWhite;				// set and save that value
    mScanDevice->storeConfig<int>(item, (goWhite ? ScanSettings::BackgroundWhite : ScanSettings::BackgroundBlack));

    resetAutoSelection();				// invalidate image data
    return (true);
}

void Previewer::slotAutoSelToggled(bool isOn)
{
    if (isOn) {                     // turning the option on?
        if (!checkForScannerBg()) {         // check or ask for background
            // if couldn't resolve it
            setAutoSelection(false);            // then force option off
            return;                 // and give up here
        }
    }

    setAutoSelection(isOn);             // set and store setting
    if (isOn && !mCanvas->hasSelectedRect()) {      // no selection yet?
        if (mCanvas->hasImage()) {          // is there a preview?
            qCDebug(LIBKOOKASCAN_LOG) << "No selection, try to find one";
            slotFindAutoSelection();
        }
    }
}

void Previewer::slotSetAutoSelThresh(int t)
{
    mAutoSelThresh = t;
    qCDebug(LIBKOOKASCAN_LOG) << "Setting threshold to" << t;

    if (mScanDevice!=nullptr)
    {
        const KConfigSkeletonItem *item = ScanSettings::self()->previewAutoselThresholdItem();
        mScanDevice->storeConfig<int>(item, t);
    }

    slotFindAutoSelection();				// with new setting
}

void Previewer::slotAutoSelectSettingsChanged(int margin, bool bgIsWhite, int dustsize)
{
    qCDebug(LIBKOOKASCAN_LOG) << "margin" << margin << "white?" << bgIsWhite << "dust" << dustsize;

    if (mScanDevice!=nullptr)				// save settings for scanner
    {
        const KConfigSkeletonItem *item = ScanSettings::self()->previewAutoselMarginItem();
        mScanDevice->storeConfig<int>(item, margin);

        item = ScanSettings::self()->previewAutoselBackgroundItem();
        mScanDevice->storeConfig<int>(item, (bgIsWhite ? ScanSettings::BackgroundWhite : ScanSettings::BackgroundBlack));

        item = ScanSettings::self()->previewAutoselDustsizeItem();
        mScanDevice->storeConfig<int>(item, dustsize);
    }

    mAutoSelMargin = margin;                // set area margin
    mAutoSelDustsize = dustsize;            // set dust size

    if (bgIsWhite != mBgIsWhite) {          // changing this setting?
        mBgIsWhite = bgIsWhite;             // set background colour
        resetAutoSelection();               // invalidate image data
    }

    slotFindAutoSelection();                // find with new settings
}

//  Try to automatically find a selection on the preview image.
//  It uses the image of the preview image canvas, the threshold
//  setting and a dust size.
//
//  Each individual scan line and column of the image is separately
//  averaged into a greyscale pixel value.  Each of these sets is
//  then scanned by imagePiece() to identify runs of above-threshold
//  (for a black scanner background) or below-threshold (for white
//  background) areas which are longer than the dust size setting.
//  The longest of those found becomes the auto-selected area.

void Previewer::slotFindAutoSelection()
{
    if (!mDoAutoSelection) return;			// not doing auto selection

    const QImage *img = mCanvas->rootImage().data();
    if (img==nullptr || img->isNull()) return;		// must have an image

    qCDebug(LIBKOOKASCAN_LOG) << "image size" << img->size()
                              << "threshold" << mAutoSelThresh
                              << "dustsize" << mAutoSelDustsize
                              << "isWhite" << mBgIsWhite;

    const long iWidth  = img->width();          // cheap copies
    const long iHeight = img->height();

    if (mHeightSum.isEmpty()) {             // need to initialise arrays
        //qCDebug(LIBKOOKASCAN_LOG) << "Summing image data";
        QVector<long> heightSum(iHeight);
        QVector<long> widthSum(iWidth);
        heightSum.fill(0);
        widthSum.fill(0);

        // Sum scan lines in both directions
        for (int y = 0; y < iHeight; ++y) {
            for (int x = 0; x < iWidth; ++x) {
                int pixgray = qGray(img->pixel(x, y));
                widthSum[x] += pixgray;
                heightSum[y] += pixgray;
            }
        }

        // Normalize sums (divide summed values by the total pixels)
        // to get an average for each scan line.
        // If the background is white, then also invert the values here.
        for (int x = 0; x < iWidth; ++x) {
            long sumval = widthSum[x];
            sumval /= iHeight;
            if (mBgIsWhite) {
                sumval = 255 - sumval;
            }
            widthSum[x] = sumval;
        }
        for (int y = 0; y < iHeight; ++y) {
            long sumval = heightSum[y];
            sumval /= iWidth;
            if (mBgIsWhite) {
                sumval = 255 - sumval;
            }
            heightSum[y] = sumval;
        }

        mWidthSum  = widthSum;              // also resizes them
        mHeightSum = heightSum;
    }

    /* Now try to find values in arrays that have grayAdds higher or lower
     * than threshold */
#ifdef DEBUG_AUTOSEL
    /* debug output */
    {
        QFile fi("/tmp/thheight.dat");
        if (fi.open(IO_ReadWrite)) {
            QTextStream str(&fi);

            str << "# height ##################" << endl;
            for (x = 0; x < iHeight; x++) {
                str << x << '\t' << mHeightSum[x] << endl;
            }
            fi.close();
        }
    }
    QFile fi1("/tmp/thwidth.dat");
    if (fi1.open(IO_ReadWrite)) {
        QTextStream str(&fi1);
        str << "# width ##################" << endl;
        str << "# " << iWidth << " points" << endl;
        for (x = 0; x < iWidth; x++) {
            str << x << '\t' << mWidthSum[x] << endl;
        }

        fi1.close();
    }
#endif
    int start;
    int end;
    QRectF r;

    if (imagePiece(mHeightSum, &start, &end)) {
        double margin = double(mAutoSelMargin) / mBedHeight;
        r.setTop(qMax(((double(start) / iHeight) - margin), 0.0));
        r.setBottom(qMin(((double(end) / iHeight) + margin), 0.999999));
    }

    if (imagePiece(mWidthSum, &start, &end)) {
        double margin = double(mAutoSelMargin) / mBedWidth;
        r.setLeft(qMax(((double(start) / iWidth) - margin), 0.0));
        r.setRight(qMin(((double(end) / iWidth) + margin), 0.999999));;
    }

    qCDebug(LIBKOOKASCAN_LOG) << "Autodetection result" << r;
    mCanvas->setSelectionRect(r);
    slotNewAreaSelected(r);
}

//  Analyse the specified array of scanline averages and try to identify
//  a run of more than <dustsize> pixels above the <threshold>.  If one
//  can be found, record its start and end as the selection boundaries.
//  The longest such run is returned as the result.
//
//  If the scanner background is white, then the pixel value will already
//  have been inverted by the caller.  The logic here is therefore the
//  same for a black or white background.

bool Previewer::imagePiece(const QVector<long> &src, int *startp, int *endp)
{
    int foundStart = 0;
    int foundEnd = 0;

    for (int x = 0; x < src.size(); ++x) {
        if (src[x] > mAutoSelThresh) {
            int thisStart = x;              // record as possible start
            ++x;                    // step on to next
            while (x < src.size() && src[x] > mAutoSelThresh) {
                ++x;
            }
            int thisEnd = x;                // find end and record that

            int delta = thisEnd - thisStart;    // length of this run

            if (delta > mAutoSelDustsize) {     // bigger than dust size?
                if (delta > (foundEnd - foundStart)) { // bigger than previously found?
                    foundStart = thisStart;     // record as result so far
                    foundEnd = thisEnd;
                }
            }
        }
    }

    *startp = foundStart;
    *endp = foundEnd;
    return ((foundEnd - foundStart) > 0);
}
