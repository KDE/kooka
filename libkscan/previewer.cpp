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
#include <kmenu.h>

#ifdef AUTOSEL_DEBUG
#include <qfile.h>
#include <qtextstream.h>
#endif

#include "img_canvas.h"
#include "sizeindicator.h"
#include "kscandevice.h"


/** Config tags for autoselection **/
#define CFG_AUTOSEL_DO        "doAutoselection"   /* do it or not */
#define CFG_AUTOSEL_THRESH    "autoselThreshold"  /* threshold    */
#define CFG_AUTOSEL_DUSTSIZE  "autoselDustsize"   /* dust size    */

/* tag if a scan of the empty scanner results in black or white image */
#define CFG_SCANNER_EMPTY_BG	"scannerBackground"
#define SCANNER_EMPTY_WHITE	"white"
#define SCANNER_EMPTY_BLACK	"black"

/* Defaultvalues for the threshold for the autodetection */
#define DEF_THRESH_BLACK "45"
#define DEF_THRESH_WHITE "240"

/* Items for the combobox to set the color of an empty scan */
#define BG_ITEM_BLACK 0
#define BG_ITEM_WHITE 1


class Previewer::PreviewerPrivate
{
public:
    PreviewerPrivate():
        m_doAutoSelection(false),
        m_autoSelThresh(0),
        m_dustsize(5),
        m_bgIsWhite(false),
        m_sliderThresh(0),
        m_sliderDust(0),
        m_cbBackground(0),
        m_autoSelGroup(0),
        m_scanner(0)
        {
        }
    bool         m_doAutoSelection;  /* switch auto-selection on and off */
    int          m_autoSelThresh;    /* threshold for auto selection     */
    int          m_dustsize;         /* dustsize for auto selection      */

    bool         m_bgIsWhite;        /* indicates if a scan without paper
                                      * results in black or white */
    QSlider     *m_sliderThresh;
    QSlider     *m_sliderDust;
    QComboBox   *m_cbBackground;
    QGroupBox   *m_autoSelGroup;
    KScanDevice *m_scanner;

    QVector<long> m_heightSum;
    QVector<long> m_widthSum;
};


Previewer::Previewer(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("Previewer");
    d = new PreviewerPrivate();

    QGridLayout *gl = new QGridLayout(this);
    gl->setMargin(0);

    /* Units etc. TODO: get from Config */
    displayUnit = KRuler::Millimetres;
    d->m_autoSelThresh = 240;

    bedHeight = 297;					// for most A4/Letter scanners
    bedWidth = 215;

    /* Stuff for the preview-Notification */
    gl->addWidget(new QLabel(i18n("<b>Preview</b>"), this), 0, 0);

    // Image viewer
    img_canvas  = new ImageCanvas(this);
    img_canvas->setDefaultScaleKind(ImageCanvas::DYNAMIC);
    img_canvas->enableContextMenu(true);
    img_canvas->repaint();
    gl->addWidget(img_canvas, 0, 1, -1, 1);
    gl->setColumnStretch(1, 1);

    /* Actions for the previewer zoom */
    KAction *act = new KAction(KIcon("scaletowidth"),i18n("Scale to Width"),this);
    act->setShortcut(Qt::CTRL+Qt::Key_I);
    connect(act, SIGNAL(triggered()), SLOT(slotScaleToWidth()));
    img_canvas->contextMenu()->addAction(act);

    act = new KAction(KIcon("scaletoheight"),i18n("Scale to Height"),this);
    act->setShortcut(Qt::CTRL+Qt::Key_H);
    connect(act, SIGNAL(triggered()), SLOT(slotScaleToHeight()));
    img_canvas->contextMenu()->addAction(act);

    /*Signals: Control the custom-field and show size of selection */
    connect(img_canvas,SIGNAL(newRect(QRect)),SLOT(slotNewAreaSelected(QRect)));

    QLabel *l;
    QVBoxLayout *vb;

    // Selected area group box
    QGroupBox *selGroup = new QGroupBox(i18n("Selection"), this);
    vb = new QVBoxLayout(selGroup);

    // Dimensions
    selSize1 = new QLabel(i18n("- mm" ), selGroup);
    vb->addWidget(selSize1, 0, Qt::AlignHCenter);
    selSize2 = new QLabel(i18n("- pix" ), selGroup);
    vb->addWidget(selSize2, 0, Qt::AlignHCenter);

    vb->addSpacing(KDialog::spacingHint());

    // File size indicator
    fileSize = new SizeIndicator(selGroup);
    fileSize->setToolTip(i18n("This size field shows how large the uncompressed image will be.\n"
                              "It tries to warn you if you try to produce too big an image by \n"
                              "changing its background color."));
    fileSize->setText("-");
    vb->addWidget(fileSize);

    gl->addWidget(selGroup, 1, 0);
    gl->setRowMinimumHeight(2, 2*KDialog::spacingHint());

    // Auto selection group box
    d->m_autoSelGroup = new QGroupBox(i18n("Auto Select"), this);
    d->m_autoSelGroup->setCheckable(true);
    d->m_autoSelGroup->setToolTip(i18n("Select this option to automatically detect\n"
                                       "the document scan area"));
    connect(d->m_autoSelGroup, SIGNAL(toggled(bool)), SLOT(slotAutoSelToggled(bool)));

    vb = new QVBoxLayout(d->m_autoSelGroup);

    l = new QLabel(i18n("Background:"), d->m_autoSelGroup);
    vb->addWidget(l);

    // combobox to select if black or white background
    d->m_cbBackground = new QComboBox(d->m_autoSelGroup);
    d->m_cbBackground->insertItem(BG_ITEM_BLACK, i18n("Black"));
    d->m_cbBackground->insertItem(BG_ITEM_WHITE, i18n("White"));
    d->m_cbBackground->setEnabled(false);
    d->m_cbBackground->setToolTip(i18n("Select whether a scan of the\n"
                                       "empty scanner glass results in\n"
                                       "a black or a white image."));
    connect(d->m_cbBackground, SIGNAL(activated(int)), SLOT(slotScanBackgroundChanged(int)));
    vb->addWidget(d->m_cbBackground);
    vb->addSpacing(2*KDialog::spacingHint());

    l= new QLabel(i18n("Threshold:"), d->m_autoSelGroup);
    vb->addWidget(l);

    // autodetect threshold slider
    d->m_sliderThresh = new QSlider(Qt::Horizontal, d->m_autoSelGroup);
    d->m_sliderThresh->setRange(0, 254);
    d->m_sliderThresh->setSingleStep(10);
    d->m_sliderThresh->setValue(d->m_autoSelThresh);
    d->m_sliderThresh->setToolTip(i18n("Threshold for autodetection.\n"
                                       "All pixels lighter (on a black background)\n"
                                       "or darker (on a white background)\n"
                                       "than this are considered to be part of the image."));
    d->m_sliderThresh->setTickPosition(QSlider::TicksBelow);
    d->m_sliderThresh->setTickInterval(25);

    connect( d->m_sliderThresh, SIGNAL(valueChanged(int)), SLOT(slotSetAutoSelThresh(int)));
    vb->addWidget(d->m_sliderThresh);

    l->setBuddy(d->m_sliderThresh);

#ifdef AUTOSEL_DUSTSIZE
    vb->addSpacing(KDialog::spacingHint());

    /** Dustsize-Slider: No deep impact on result **/
    l = new QLabel( i18n("Dust size:"), d->m_autoSelGroup);
    vb->addWidget(l);

    d->m_sliderDust = new QSlider(Qt::Horizontal, d->m_autoSelGroup);
    d->m_sliderDust ->setRange(0, 50);
    d->m_sliderDust ->setSingleStep(5);
    d->m_sliderDust ->setValue(d->m_dustsize);
    d->m_sliderDust->setTickPosition(QSlider::TicksBelow);
    d->m_sliderDust->setTickInterval(5);

    connect(d->m_sliderDust, SIGNAL(valueChanged(int)), SLOT(slotSetAutoSelDustsize(int)));
    vb->addWidget(d->m_sliderDust);
#endif

    /* disable Autoselbox as long as no scanner is connected */
    d->m_autoSelGroup->setEnabled(false);

    gl->addWidget(d->m_autoSelGroup, 3, 0);

    gl->setRowStretch(4, 1);

    scanResX = -1;
    scanResY = -1;
    m_bytesPerPix = -1;

    selectionWidthMm = bedWidth;
    selectionHeightMm = bedHeight;
    updateSelectionDims();
    slotAutoSelToggled(false);
}


Previewer::~Previewer()
{
    delete d;
}


bool Previewer::setPreviewImage( const QImage &image )
{
   if ( image.isNull() )
	return false;

   kDebug() << "setting new image, size" << image.size();
   m_previewImage = image;
   img_canvas->newImage( &m_previewImage );

   return true;
}


void Previewer::newImage(const QImage *image)
{
   kDebug() << "size" << image->size();

   /* image canvas does not copy the image, so we hold a copy here */
   m_previewImage = *image;

   /* clear the auto detection arrays */
   d->m_heightSum.resize(0);
   d->m_widthSum.resize(0);

   img_canvas->newImage(&m_previewImage);
   findSelection();
}


void Previewer::setScannerBedSize(int w,int h)
{
   kDebug() << "to [" << w << "," << h << "]";

   bedWidth = w;
   bedHeight = h;

   slotNewCustomScanSize(QRect());			// reset selection and display
}


void Previewer::setDisplayUnit(KRuler::MetricStyle unit)
{
    displayUnit = unit;
}


// sent from ScanParams, selection chosen by user
void Previewer::slotNewCustomScanSize(QRect rect)
{
    kDebug() << "rect" << rect;

    if (rect.isValid())
    {
        selectionWidthMm = rect.width();
        selectionHeightMm = rect.height();
							// convert mm -> bedsize/1000
        rect.setLeft(static_cast<int>(1000.0*rect.left()/bedWidth+0.5));
        rect.setRight(static_cast<int>(1000.0*rect.right()/bedWidth+0.5));
        rect.setTop(static_cast<int>(1000.0*rect.top()/bedHeight+0.5));
        rect.setBottom(static_cast<int>(1000.0*rect.bottom()/bedHeight+0.5));
    }
    else
    {
        selectionWidthMm = bedWidth;
        selectionHeightMm = bedHeight;
    }

    img_canvas->setSelectionRect(rect);
    updateSelectionDims();
}


void Previewer::slotNewScanResolutions(int xres,int yres)
{
    kDebug() << "resolution" << xres << " x " << yres;

    scanResX = xres;
    scanResY = yres;
    updateSelectionDims();
}


void Previewer::slotNewScanMode(int bytes_per_pix)
{
    kDebug() << "bytes per pix" << bytes_per_pix;

    m_bytesPerPix = bytes_per_pix;
    updateSelectionDims();
}


/* This slot is called with the new dimension for the selection
 * in values between 0..1000. It emits signals, that redraw the
 * size labels.
 */

void Previewer::slotNewAreaSelected(QRect rect)
{
    kDebug() << "rect" << rect << "width" << bedWidth << "height" << bedHeight;

    if (rect.isValid())
    {							// convert bedsize/1000 -> mm
        rect.setLeft(static_cast<int>(rect.left()/1000.0*bedWidth+0.5));
        rect.setRight(static_cast<int>(rect.right()/1000.0*bedWidth+0.5));
        rect.setTop(static_cast<int>(rect.top()/1000.0*bedHeight+0.5));
        rect.setBottom(static_cast<int>(rect.bottom()/1000.0*bedHeight+0.5));
        kDebug() << "new rect" << rect;
        emit newPreviewRect(rect);

        selectionWidthMm = rect.width();
        selectionHeightMm = rect.height();
    }
    else
    {
        emit newPreviewRect(QRect());			// full scan area

        selectionWidthMm = bedWidth;			// for size display
        selectionHeightMm = bedHeight;
    }

    updateSelectionDims();
}


void Previewer::updateSelectionDims()
{
    if (d->m_scanner==NULL) return;			// no scanner connected

    selSize1->setText(i18n("%1 x %2 mm", selectionWidthMm, selectionHeightMm));

    /* Calculate file size */
    long size_in_byte = 0;
    if (scanResX>1 && scanResY>1)			// if resolution available
    {
        double w_inch = ((double) selectionWidthMm)/25.4;
        double h_inch = ((double) selectionHeightMm)/25.4;

        int pix_w = static_cast<int>(w_inch*scanResX+0.5);
        int pix_h = static_cast<int>(h_inch*scanResY+0.5);

        selSize2->setText(i18n("%1 x %2 pix", pix_w, pix_h));

        if (m_bytesPerPix!=-1)				// depth of scan available
        {
            if (m_bytesPerPix==0)			// bitmap scan
            {
                size_in_byte = pix_w*pix_h/8;
            }
            else					// grey or colour scan
            {
                size_in_byte = pix_w*pix_h*m_bytesPerPix;
            }

            fileSize->setSizeInByte(size_in_byte);
        }
        else fileSize->setSizeInByte(-1);		// depth not available
    }
    else selSize2->setText(QString::null);		// resolution not available
}


void Previewer::slotScaleToWidth()
{
   if( img_canvas )
   {
      img_canvas->handlePopup( ImageCanvas::ID_FIT_WIDTH );
   }
}

void Previewer::slotScaleToHeight()
{
   if( img_canvas )
   {
      img_canvas->handlePopup( ImageCanvas::ID_FIT_HEIGHT);
   }
}


void Previewer::connectScanner(KScanDevice *scan)
{
    kDebug();
    d->m_scanner = scan;

    if (scan!=NULL)
    {
        d->m_autoSelGroup->setEnabled(true);		// enable the auto-select group
							// set parameters from config

        bool asel = (scan->getConfig(CFG_AUTOSEL_DO, QString::null)=="on");
							// "white" is the best default
        if (d->m_autoSelGroup!=NULL) d->m_autoSelGroup->setChecked(asel);

        bool isWhite = scan->getConfig(CFG_SCANNER_EMPTY_BG,"")!=SCANNER_EMPTY_BLACK;
        d->m_cbBackground->setCurrentIndex(isWhite ? BG_ITEM_WHITE : BG_ITEM_BLACK);

        QString h = scan->getConfig(CFG_AUTOSEL_DUSTSIZE,"5");
        d->m_dustsize = h.toInt();

        h = scan->getConfig(CFG_AUTOSEL_THRESH,(isWhite ? DEF_THRESH_WHITE : DEF_THRESH_BLACK));
        d->m_sliderThresh->setValue(h.toInt());

        updateSelectionDims();
    }
}


void Previewer::setScannerBgIsWhite(bool isWhite)
{
    d->m_bgIsWhite = isWhite;
    if (d->m_scanner!=NULL)
    {
        d->m_cbBackground->setCurrentIndex(isWhite ? BG_ITEM_WHITE : BG_ITEM_BLACK);
        d->m_cbBackground->setEnabled(true);

        d->m_scanner->slotStoreConfig(CFG_SCANNER_EMPTY_BG,(isWhite ? SCANNER_EMPTY_WHITE : SCANNER_EMPTY_BLACK));
    }
}


/**
 * reads the scanner dependant config file through the m_scanner pointer.
 * If a value for the scanner is not yet known, the function starts up a
 * popup and asks the user.  The result is stored.
 */

bool Previewer::checkForScannerBg()
{
    if (d->m_scanner!=NULL)				// scan device already known?
    {
        QString curWhite = d->m_scanner->getConfig(CFG_SCANNER_EMPTY_BG, "");
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
}


void Previewer::slotAutoSelToggled(bool isOn )
{
    if (isOn)
    {
        if (!checkForScannerBg())
        {
            if (d->m_autoSelGroup!=NULL) d->m_autoSelGroup->setChecked(false);
            return;
        }
    }

    QRect r = img_canvas->sel();
    kDebug() << "rect is" << r;

    /* Store configuration */
    d->m_doAutoSelection = isOn;
    if (d->m_scanner!=NULL) d->m_scanner->slotStoreConfig(CFG_AUTOSEL_DO,
                                                          (isOn ? "on" : "off"));

    if (isOn && r.width()<2 && r.height()<2)	/* There is no selection yet */
    {
            /* if there is already an image, check, if the bg-color is set already */
            if (img_canvas->rootImage())
            {
                kDebug() << "No selection, try to find one";
                findSelection();
            }
    }

    if (d->m_sliderThresh!=NULL) d->m_sliderThresh->setEnabled(isOn);
    if (d->m_sliderDust!=NULL) d->m_sliderDust->setEnabled(isOn);
    if (d->m_cbBackground!=NULL) d->m_cbBackground->setEnabled(isOn);
}


void Previewer::slotSetAutoSelThresh(int t)
{
    d->m_autoSelThresh = t;
    kDebug() << "Setting threshold to" << t;
    if( d->m_scanner )
        d->m_scanner->slotStoreConfig( CFG_AUTOSEL_THRESH, QString::number(t) );
    findSelection();
}

void Previewer::slotSetAutoSelDustsize(int dSize)
{
    d->m_dustsize = dSize;
    kDebug() << "Setting dustsize to" << dSize;
    findSelection();
}

/**
 * This method tries to find a selection on the preview image automatically.
 * It uses the image of the preview image canvas, the previewer global
 * threshold setting and a dustsize.
 **/
void  Previewer::findSelection( )
{
    kDebug() << "threshold:" << d->m_autoSelThresh
             << "dustsize:" << d->m_dustsize
             << "isWhite:" << d->m_bgIsWhite;

    if( ! d->m_doAutoSelection ) return;
    int line;
    int x;
    const QImage *img = img_canvas->rootImage();
    if( ! img ) return;

    long iWidth  = img->width();
    long iHeight = img->height();

    //QVector<long> heightSum;
    //QVector<long> widthSum;

    kDebug()<< "Preview size is" << img->size();

    if( (d->m_heightSum).size() == 0 && (iHeight>0) )
    {
        kDebug() << "Starting to fill Array";
        QVector<long> heightSum(iHeight);
        QVector<long> widthSum(iWidth);
        heightSum.fill(0);
        widthSum.fill(0);

        kDebug() << "filled Array with zero";

        for( line = 0; line < iHeight; line++ )
        {

            for( x = 0; x < iWidth; x++ )
            {
                int gray  = qGray( img->pixel( x, line ));
                // kdDebug(29000) << "Gray-Value at line " << gray << endl;
                Q_ASSERT( line < iHeight );
                Q_ASSERT( x < iWidth );
                int hsum = heightSum.at(line);
                int wsum = widthSum.at(x);

                heightSum[line] = hsum+gray;
                widthSum [x]    = wsum+gray;
            }
            heightSum[line] = heightSum[line]/iWidth;
        }
        /* Divide by amount of pixels */
        kDebug(29000) << "Resizing...";
        for( x = 0; x < iWidth; x++ )
            widthSum[x] = widthSum[x]/iHeight;

        kDebug() << "Filled Arrays successfully";
        d->m_widthSum  = widthSum;
        d->m_heightSum = heightSum;
    }
    /* Now try to find values in arrays that have grayAdds higher or lower
     * than threshold */
#ifdef AUTOSEL_DEBUG
    /* debug output */
    {
        QFile fi( "/tmp/thheight.dat");
        if( fi.open( IO_ReadWrite ) ) {
            QTextStream str( &fi );

            str << "# height ##################" << endl;
            for( x = 0; x < iHeight; x++ )
                str << x << '\t' << d->m_heightSum[x] << endl;
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
            str << x << '\t' << d->m_widthSum[x] << endl;

        fi1.close();
    }
#endif
    int start = 0;
    int end = 0;
    QRect r;

    /** scale to 0..1000 range **/
    start = 0;
    end   = 0;
    imagePiece( d->m_heightSum, start, end ); // , d->m_threshold, d->m_dustsize, false );

    r.setTop(  1000*start/iHeight );
    r.setBottom( 1000*end/iHeight);
    // r.setTop(  start );
    // r.setBottom( end );

    start = 0;
    end   = 0;
    imagePiece( d->m_widthSum, start, end ); // , d->m_threshold, d->m_dustsize, false );
    r.setLeft( 1000*start/iWidth );
    r.setRight( 1000*end/iWidth );
    // r.setLeft( start );
    // r.setRight( end );

    kDebug() << "Autodetection results:"
             << "top " << r.top()
             << "left" << r.left()
             << "bottom " << r.bottom()
             << "right " << r.right()
             << "width " << r.width()
             << "height " << r.height();

    img_canvas->setSelectionRect(r);
    slotNewAreaSelected(r);
}


/*
 * returns an Array containing the
 */
bool Previewer::imagePiece( QVector<long> src, int& start, int& end )
{
    for( int x = 0; x < src.size(); x++ )
    {
        if( !d->m_bgIsWhite )
        {
            /* pixelvalue needs to be higher than threshold, white background */
            if( src[x] > d->m_autoSelThresh )
            {
                /* Ok this pixel could be the start */
                int iStart = x;
                int iEnd = x;
                x++;
                while( x < src.size() && src[x] > d->m_autoSelThresh )
                {
                    x++;
                }
                iEnd = x;

                int delta = iEnd-iStart;

                if( delta > d->m_dustsize && end-start < delta )
                {
                    start = iStart;
                    end   = iEnd;
                }
            }
        }
        else
        {
            /* pixelvalue needs to be lower than threshold, black background */
            if( src[x] < d->m_autoSelThresh )
            {
                int iStart = x;
                int iEnd = x;
                x++;
                while( x < src.size() && src[x] < d->m_autoSelThresh )
                {
                    x++;
                }
                iEnd = x;

                int delta = iEnd-iStart;

                if( delta > d->m_dustsize && end-start < delta )
                {
                    start = iStart;
                    end   = iEnd;
                }
            }
        }
    }
    return (end-start)>0;
}
