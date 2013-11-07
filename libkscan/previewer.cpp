/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>

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

#include "previewer.h"
#include "previewer.moc"

#include <qcombobox.h>
#include <qgroupbox.h>
#include <qgridlayout.h>
#include <qslider.h>
#include <qvector.h>

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <kdialog.h>

#ifdef DEBUG_AUTOSEL
#include <qfile.h>
#include <qtextstream.h>
#endif

#include "imagecanvas.h"
#include "sizeindicator.h"
#include "kscandevice.h"


/** Config tags for autoselection **/
#define CFG_AUTOSEL_ON		"doAutoselection"   /* do it or not */
#define CFG_AUTOSEL_THRESHOLD	"autoselThreshold"  /* threshold    */
#define CFG_AUTOSEL_DUSTSIZE	"autoselDustsize"   /* dust size    */

/* tag if a scan of the empty scanner results in black or white image */
#define CFG_SCANNER_EMPTY_BG	"scannerBackground"
#define SCANNER_EMPTY_WHITE	"white"
#define SCANNER_EMPTY_BLACK	"black"

/* Defaultvalues for the threshold for the autodetection */
#define DEF_THRESHOLD		"25"
#define DEF_DUSTSIZE		"5"
#define MAX_THRESHOLD		50

/* Items for the combobox to set the color of an empty scan */
#define BG_ITEM_BLACK		0
#define BG_ITEM_WHITE		1


Previewer::Previewer(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("Previewer");

    QGridLayout *gl = new QGridLayout(this);
    gl->setMargin(0);

    /* Units etc. TODO: get from Config */
    mDisplayUnit = KRuler::Millimetres;
    mAutoSelThresh = 240;

    mScanDevice = NULL;					// no scanner connected yet
    mBedHeight = 297;					// for most A4/Letter scanners
    mBedWidth = 215;

    /* Stuff for the preview-Notification */
    gl->addWidget(new QLabel(i18n("<b>Preview</b>"), this), 0, 0);

    // Image viewer
    mCanvas  = new ImageCanvas(this);
    mCanvas->setDefaultScaleType(ImageCanvas::ScaleDynamic);

    //mCanvas->repaint();
    gl->addWidget(mCanvas, 0, 1, -1, 1);
    gl->setColumnStretch(1, 1);

    /*Signals: Control the custom-field and show size of selection */
    connect(mCanvas, SIGNAL(newRect(const QRectF &)), SLOT(slotNewAreaSelected(const QRectF &)));

    QLabel *l;
    QVBoxLayout *vb;

    // Selected area group box
    QGroupBox *selGroup = new QGroupBox(i18n("Selection"), this);
    vb = new QVBoxLayout(selGroup);

    // Dimensions
    mSelSizeLabel1 = new QLabel(i18n("- mm" ), selGroup);
    vb->addWidget(mSelSizeLabel1, 0, Qt::AlignHCenter);
    mSelSizeLabel2 = new QLabel(i18n("- pix" ), selGroup);
    vb->addWidget(mSelSizeLabel2, 0, Qt::AlignHCenter);

    vb->addSpacing(KDialog::spacingHint());

    // File size indicator
    mFileSizeLabel = new SizeIndicator(selGroup);
    mFileSizeLabel->setToolTip(i18n("This size field shows how large the uncompressed image will be.\n"
                                    "It tries to warn you if you try to produce too big an image by \n"
                                    "changing its background color."));
    mFileSizeLabel->setText("-");
    vb->addWidget(mFileSizeLabel);

    gl->addWidget(selGroup, 1, 0);
    gl->setRowMinimumHeight(2, 2*KDialog::spacingHint());

    // Auto selection group box
    mAutoSelGroup = new QGroupBox(i18n("Auto Select"), this);
    mAutoSelGroup->setCheckable(true);
    mAutoSelGroup->setEnabled(false);			// initially, no scanner
    mAutoSelGroup->setToolTip(i18n("Select this option to automatically detect\n"
                                   "the document scan area"));
    connect(mAutoSelGroup, SIGNAL(toggled(bool)), SLOT(slotAutoSelToggled(bool)));

    vb = new QVBoxLayout(mAutoSelGroup);

    l = new QLabel(i18n("Background:"), mAutoSelGroup);
    vb->addWidget(l);

    // combobox to select if black or white background
    mBackgroundCombo = new QComboBox(mAutoSelGroup);
    mBackgroundCombo->insertItem(BG_ITEM_BLACK, i18n("Black"));
    mBackgroundCombo->insertItem(BG_ITEM_WHITE, i18n("White"));
    mBackgroundCombo->setToolTip(i18n("Select whether a scan of the\n"
                                       "empty scanner glass results in\n"
                                       "a black or a white image."));
    connect(mBackgroundCombo, SIGNAL(activated(int)), SLOT(slotScanBackgroundChanged(int)));
    vb->addWidget(mBackgroundCombo);
    vb->addSpacing(2*KDialog::spacingHint());

    l->setBuddy(mBackgroundCombo);

    l= new QLabel(i18n("Threshold:"), mAutoSelGroup);
    vb->addWidget(l);

    // autodetect threshold slider
    mSliderThresh = new QSlider(Qt::Horizontal, mAutoSelGroup);
    mSliderThresh->setRange(0, MAX_THRESHOLD);
    mSliderThresh->setSingleStep(5);
    mSliderThresh->setValue(mAutoSelThresh);
    mSliderThresh->setToolTip(i18n("Threshold for autodetection.\n"
                                   "All pixels lighter (on a black background)\n"
                                   "or darker (on a white background)\n"
                                   "than this are considered to be part of the image."));
    mSliderThresh->setTickPosition(QSlider::TicksBelow);
    mSliderThresh->setTickInterval(10);

    connect( mSliderThresh, SIGNAL(valueChanged(int)), SLOT(slotSetAutoSelThresh(int)));
    vb->addWidget(mSliderThresh);

    l->setBuddy(mSliderThresh);

#ifdef AUTOSEL_DUSTSIZE_GUI
    vb->addSpacing(KDialog::spacingHint());

    /** Dustsize-Slider: No deep impact on result **/
    l = new QLabel( i18n("Dust size:"), mAutoSelGroup);
    vb->addWidget(l);

    mSliderDust = new QSlider(Qt::Horizontal, mAutoSelGroup);
    mSliderDust ->setRange(0, 50);
    mSliderDust ->setSingleStep(5);
    mSliderDust ->setValue(mAutoSelDustsize);
    mSliderDust->setTickPosition(QSlider::TicksBelow);
    mSliderDust->setTickInterval(5);

    connect(mSliderDust, SIGNAL(valueChanged(int)), SLOT(slotSetAutoSelDustsize(int)));
    vb->addWidget(mSliderDust);
#endif

    gl->addWidget(mAutoSelGroup, 3, 0);

    gl->setRowStretch(4, 1);

    mScanResX = -1;
    mScanResY = -1;
    mBytesPerPix = -1;

    mSelectionWidthMm = mBedWidth;
    mSelectionHeightMm = mBedHeight;
    updateSelectionDims();
    setAutoSelection(false);				// initially disable, no scanner
}


Previewer::~Previewer()
{
}


bool Previewer::setPreviewImage( const QImage &image )
{
   if ( image.isNull() )
	return false;

   kDebug() << "setting new image, size" << image.size();
   mPreviewImage = image;
   mCanvas->newImage( &mPreviewImage );

   return true;
}


void Previewer::newImage(const QImage *image)
{
   kDebug() << "size" << image->size();

   /* image canvas does not copy the image, so we hold a copy here */
   mPreviewImage = *image;

   resetAutoSelection();				// reset for new image
   mCanvas->newImage(&mPreviewImage);			// set image on canvas
   findAutoSelection();					// auto-select if required
}


void Previewer::setScannerBedSize(int w,int h)
{
   kDebug() << "to [" << w << "," << h << "]";

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
    kDebug() << "rect" << rect;

    QRect r = rect;
    QRectF newRect;
    if (r.isValid())
    {
        mSelectionWidthMm = r.width();
        mSelectionHeightMm = r.height();

        newRect.setLeft(double(r.left())/mBedWidth);	// convert mm -> bedsize factor
        newRect.setRight(double(r.right())/mBedWidth);
        newRect.setTop(double(r.top())/mBedHeight);
        newRect.setBottom(double(r.bottom())/mBedHeight);
    }
    else
    {
        mSelectionWidthMm = mBedWidth;
        mSelectionHeightMm = mBedHeight;
    }

    mCanvas->setSelectionRect(newRect);
    updateSelectionDims();
}


void Previewer::slotNewScanResolutions(int xres,int yres)
{
    kDebug() << "resolution" << xres << " x " << yres;

    mScanResX = xres;
    mScanResY = yres;
    updateSelectionDims();
}


void Previewer::slotNewScanMode(int bytes_per_pix)
{
    kDebug() << "bytes per pix" << bytes_per_pix;

    mBytesPerPix = bytes_per_pix;
    updateSelectionDims();
}


//  Signal sent from ImageCanvas, the user has drawn a new selection box
//  (specified relative to the preview image size).  This is converted
//  to millimetres and re-emitted as signal newPreviewRect().
void Previewer::slotNewAreaSelected(const QRectF &rect)
{
    kDebug() << "rect" << rect << "width" << mBedWidth << "height" << mBedHeight;

    if (rect.isValid())
    {							// convert bedsize -> mm
        QRect r;
        r.setLeft(int(rect.left()*mBedWidth+0.5));
        r.setRight(int(rect.right()*mBedWidth+0.5));
        r.setTop(int(rect.top()*mBedHeight+0.5));
        r.setBottom(int(rect.bottom()*mBedHeight+0.5));
        kDebug() << "new rect" << r;
        emit newPreviewRect(r);

        mSelectionWidthMm = r.width();
        mSelectionHeightMm = r.height();
    }
    else
    {
        emit newPreviewRect(QRect());			// full scan area

        mSelectionWidthMm = mBedWidth;			// for size display
        mSelectionHeightMm = mBedHeight;
    }

    updateSelectionDims();
}


void Previewer::updateSelectionDims()
{
    if (mScanDevice==NULL) return;			// no scanner connected

    mSelSizeLabel1->setText(i18n("%1 x %2 mm", mSelectionWidthMm, mSelectionHeightMm));

    /* Calculate file size */
    long size_in_byte = 0;
    if (mScanResX>1 && mScanResY>1)			// if resolution available
    {
        double w_inch = ((double) mSelectionWidthMm)/25.4;
        double h_inch = ((double) mSelectionHeightMm)/25.4;

        int pix_w = static_cast<int>(w_inch*mScanResX+0.5);
        int pix_h = static_cast<int>(h_inch*mScanResY+0.5);

        mSelSizeLabel2->setText(i18n("%1 x %2 pixel", pix_w, pix_h));

        if (mBytesPerPix!=-1)				// depth of scan available
        {
            if (mBytesPerPix==0)			// bitmap scan
            {
                size_in_byte = pix_w*pix_h/8;
            }
            else					// grey or colour scan
            {
                size_in_byte = pix_w*pix_h*mBytesPerPix;
            }

            mFileSizeLabel->setSizeInByte(size_in_byte);
        }
        else mFileSizeLabel->setSizeInByte(-1);		// depth not available
    }
    else mSelSizeLabel2->setText(QString::null);	// resolution not available
}


void Previewer::connectScanner(KScanDevice *scan)
{
    kDebug();
    mScanDevice = scan;

    if (scan!=NULL)
    {
        mAutoSelGroup->setEnabled(true);		// enable the auto-select group
        setAutoSelection(false);			// initially off, disregard config
							// but get other saved values
        bool isWhite = scan->getConfig(CFG_SCANNER_EMPTY_BG, "")!=SCANNER_EMPTY_BLACK;
        mBackgroundCombo->setCurrentIndex(isWhite ? BG_ITEM_WHITE : BG_ITEM_BLACK);
        mBgIsWhite = isWhite;

        int val = scan->getConfig(CFG_AUTOSEL_DUSTSIZE, DEF_DUSTSIZE).toInt();
#ifdef AUTOSEL_DUSTSIZE_GUI
        mSliderDust->setValue(val);
#else
        mAutoSelDustsize = val;
#endif
        val = scan->getConfig(CFG_AUTOSEL_THRESHOLD, DEF_THRESHOLD).toInt();
        mSliderThresh->setValue(val);

        updateSelectionDims();
    }
}


void Previewer::setScannerBgIsWhite(bool isWhite)
{
    mBgIsWhite = isWhite;
    resetAutoSelection();				// invalidate image data

    if (mScanDevice!=NULL)
    {
        mBackgroundCombo->setCurrentIndex(isWhite ? BG_ITEM_WHITE : BG_ITEM_BLACK);
        mBackgroundCombo->setEnabled(true);

        mScanDevice->storeConfig(CFG_SCANNER_EMPTY_BG,(isWhite ? SCANNER_EMPTY_WHITE : SCANNER_EMPTY_BLACK));
    }
}


void Previewer::resetAutoSelection()
{
   mHeightSum.clear();
   mWidthSum.clear();
}


void Previewer::setAutoSelection(bool isOn)
{
    kDebug() << "to" << isOn;

    if (isOn && mScanDevice==NULL)			// no scanner connected yet
    {
        kDebug() << "no scanner!";
        isOn = false;
    }

    mDoAutoSelection = isOn;
    if (mAutoSelGroup!=NULL) mAutoSelGroup->setChecked(isOn);
    if (mSliderThresh!=NULL) mSliderThresh->setEnabled(isOn);
#ifdef AUTOSEL_DUSTSIZE_GUI
    if (mSliderDust!=NULL) mSliderDust->setEnabled(isOn);
#endif
    if (mBackgroundCombo!=NULL) mBackgroundCombo->setEnabled(isOn);

    if (mScanDevice!=NULL) mScanDevice->storeConfig(CFG_AUTOSEL_ON,
                                                    (isOn ? "on" : "off"));
}


/**
 * reads the scanner dependant config file through the mScanDevice pointer.
 * If a value for the scanner is not yet known, the function starts up a
 * popup and asks the user.  The result is stored.
 */

bool Previewer::checkForScannerBg()
{
    if (mScanDevice!=NULL)				// scan device already known?
    {
        QString curWhite = mScanDevice->getConfig(CFG_SCANNER_EMPTY_BG, "");
        bool goWhite = false;

        if (curWhite.isEmpty())				// not yet known, should ask the user
        {
            kDebug() << "Don't know the scanner background yet!";

            int res = KMessageBox::questionYesNoCancel(this,
                                                       i18n("The autodetection of images on the preview depends on the background color of the preview image (the result of scanning with no document loaded).\n\nPlease select whether the background of the preview image is black or white."),
                                                       i18n("Autodetection Background"),
                                                       KGuiItem(i18n("White")), KGuiItem(i18n("Black")));
            if (res==KMessageBox::Cancel) return (false);
            goWhite = (res==KMessageBox::Yes);
        }
        else
        {
            if (curWhite==SCANNER_EMPTY_WHITE) goWhite = true;
        }

        setScannerBgIsWhite(goWhite);			// set and save that value
    }

    return (true);
}


void Previewer::slotScanBackgroundChanged(int indx)
{
    setScannerBgIsWhite(indx==BG_ITEM_WHITE);
    findAutoSelection();				// with new setting
}


void Previewer::slotAutoSelToggled(bool isOn)
{
    if (isOn)						// turning the option on?
    {
        if (!checkForScannerBg())			// check or ask for background
        {						// if couldn't resolve it
            setAutoSelection(false);			// then force option off
            return;					// and give up here
        }
    }

    /* Store configuration */
    mDoAutoSelection = isOn;
    if (mScanDevice!=NULL) mScanDevice->storeConfig(CFG_AUTOSEL_ON,
                                                    (isOn ? "on" : "off"));
    if (isOn && !mCanvas->hasSelectedRect())		// no selection yet?
    {
            /* if there is already an image, check, if the bg-color is set already */
            if (mCanvas->hasImage())
            {
                kDebug() << "No selection, try to find one";
                findAutoSelection();
            }
    }
}


void Previewer::slotSetAutoSelThresh(int t)
{
    mAutoSelThresh = t;
    kDebug() << "Setting threshold to" << t;
    // TODO: make storeConfig/getConfig templates, then no need for string conversion
    if (mScanDevice!=NULL) mScanDevice->storeConfig(CFG_AUTOSEL_THRESHOLD, QString::number(t));
    findAutoSelection();				// with new setting
}


void Previewer::slotSetAutoSelDustsize(int dSize)
{
    mAutoSelDustsize = dSize;
    kDebug() << "Setting dustsize to" << dSize;
#ifdef AUTOSEL_DUSTSIZE_GUI
    if (mScanDevice!=NULL) mScanDevice->storeConfig(CFG_AUTOSEL_DUSTSIZE, QString::number(dSize));
    findAutoSelection();				// with new setting
#endif
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

void Previewer::findAutoSelection()
{
    if (!mDoAutoSelection) return;			// not doing auto selection

    const QImage *img = mCanvas->rootImage();
    if (img==NULL || img->isNull()) return;		// must have an image

    kDebug()<< "image size" << img->size()
            << "threshold" << mAutoSelThresh
            << "dustsize" << mAutoSelDustsize
            << "isWhite" << mBgIsWhite;

    const long iWidth  = img->width();			// cheap copies
    const long iHeight = img->height();

    if (mHeightSum.isEmpty())				// need to initialise arrays 
    {
        kDebug() << "Summing image data";
        QVector<long> heightSum(iHeight);
        QVector<long> widthSum(iWidth);
        heightSum.fill(0);
        widthSum.fill(0);

        // Sum scan lines in both directions
        for (int y = 0; y<iHeight; ++y)
        {
            for (int x = 0; x<iWidth; ++x)
            {
                int pixgray = qGray(img->pixel(x, y));
                widthSum[x] += pixgray;
                heightSum[y] += pixgray;
            }
        }

        // Normalize sums (divide summed values by the total pixels)
        // to get an average for each scan line.
        // If the background is white, then also invert the values here.
        for (int x = 0; x<iWidth; ++x)
        {
            long sumval = widthSum[x];
            sumval /= iHeight;
            if (mBgIsWhite) sumval = 255-sumval;
            widthSum[x] = sumval;
        }
        for (int y = 0; y<iHeight; ++y)
        {
            long sumval = heightSum[y];
            sumval /= iWidth;
            if (mBgIsWhite) sumval = 255-sumval;
            heightSum[y] = sumval;
        }

        mWidthSum  = widthSum;				// also resizes them
        mHeightSum = heightSum;
    }

    /* Now try to find values in arrays that have grayAdds higher or lower
     * than threshold */
#ifdef DEBUG_AUTOSEL
    /* debug output */
    {
        QFile fi( "/tmp/thheight.dat");
        if( fi.open( IO_ReadWrite ) ) {
            QTextStream str( &fi );

            str << "# height ##################" << endl;
            for( x = 0; x < iHeight; x++ )
                str << x << '\t' << mHeightSum[x] << endl;
            fi.close();
        }
    }
    QFile fi1( "/tmp/thwidth.dat");
    if( fi1.open( IO_ReadWrite ))
    {
        QTextStream str( &fi1 );
        str << "# width ##################" << endl;
        str << "# " << iWidth << " points" << endl;
        for( x = 0; x < iWidth; x++ )
            str << x << '\t' << mWidthSum[x] << endl;

        fi1.close();
    }
#endif
    int start;
    int end;
    QRectF r;

    if (imagePiece(mHeightSum, &start, &end))
    {
        r.setTop(double(start)/iHeight);
        r.setBottom(double(end)/iHeight);
    }

    if (imagePiece(mWidthSum, &start, &end))
    {
        r.setLeft(double(start)/iWidth);
        r.setRight(double(end)/iWidth);
    }

    kDebug() << "Autodetection result" << r;
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

    for (int x = 0; x<src.size(); ++x)
    {
        if (src[x]>mAutoSelThresh)
        {
            int thisStart = x;				// record as possible start
            ++x;					// step on to next
            while (x<src.size() && src[x]>mAutoSelThresh) ++x;
            int thisEnd = x;				// find end and record that

            int delta = thisEnd-thisStart;		// length of this run

            if (delta>mAutoSelDustsize)			// bigger than dust size?
            {
                if (delta>(foundEnd-foundStart))	// bigger than previously found?
                {
                    foundStart = thisStart;		// record as result so far
                    foundEnd = thisEnd;
                }
            }
        }
    }

    *startp = foundStart;
    *endp = foundEnd;
    return ((foundEnd-foundStart)>0);
}
