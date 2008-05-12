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

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kaction.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kdialog.h>

#include <qhbox.h>
#include <qtooltip.h>
#include <qpopupmenu.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qslider.h>
#include <qcheckbox.h>

#ifdef AUTOSEL_DEBUG
#include <qfile.h>
#include <qtextstream.h>
#endif

#include "img_canvas.h"
#include "sizeindicator.h"
#include "kscandevice.h"

#include "previewer.moc"
#include "previewer.h"


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
        m_cbAutoSel(0),
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
    QCheckBox   *m_cbAutoSel;
    QComboBox   *m_cbBackground;
    QGroupBox   *m_autoSelGroup;
    KScanDevice *m_scanner;

    QMemArray<long> m_heightSum;
    QMemArray<long> m_widthSum;
};


Previewer::Previewer(QWidget *parent)
    : QWidget(parent)
{
    d = new PreviewerPrivate();

    QVBoxLayout *top = new QVBoxLayout( this, 10 );
    QHBoxLayout *layout = new QHBoxLayout( 2 );
    top->addLayout( layout, 9 );
    QVBoxLayout *left = new QVBoxLayout( 2*KDialog::spacingHint() );
    layout->addLayout( left, 2 );

    /* Units etc. TODO: get from Config */
    displayUnit = KRuler::Millimetres;
    d->m_autoSelThresh = 240;

    bedHeight = 297;					// for most A4/Letter scanners
    bedWidth = 215;

    img_canvas  = new ImageCanvas( this );
    img_canvas->setDefaultScaleKind( ImageCanvas::DYNAMIC );
    img_canvas->enableContextMenu(true);
    img_canvas->repaint();
    layout->addWidget( img_canvas, 6 );

    /* Actions for the previewer zoom */
    KAction *act;
    act =  new KAction(i18n("Scale to W&idth"), "scaletowidth", CTRL+Key_I,
		       this, SLOT( slScaleToWidth()), this, "preview_scaletowidth" );
    act->plug( img_canvas->contextMenu());

    act = new KAction(i18n("Scale to &Height"), "scaletoheight", CTRL+Key_H,
		      this, SLOT( slScaleToHeight()), this, "preview_scaletoheight" );
    act->plug( img_canvas->contextMenu());

    /*Signals: Control the custom-field and show size of selection */
    connect(img_canvas,SIGNAL(newRect(QRect)),SLOT(slotNewAreaSelected(QRect)));

    /* Stuff for the preview-Notification */
    left->addWidget( new QLabel( i18n("<B>Preview</B>"), this ), 1);

    /** Autoselection Box **/
    d->m_autoSelGroup = new QGroupBox( 1, Horizontal, i18n("Auto-Select"), this);

    QHBox *hbox       = new QHBox(d->m_autoSelGroup);
    d->m_cbAutoSel    = new QCheckBox( i18n("Active on"), hbox );
    QToolTip::add( d->m_cbAutoSel, i18n("Check here if you want autodetection\n"
                                        "of the document on the preview."));

    /* combobox to select if black or white background */
    d->m_cbBackground = new QComboBox( hbox );
    d->m_cbBackground->insertItem(i18n("Black"), BG_ITEM_BLACK );
    d->m_cbBackground->insertItem(i18n("White"), BG_ITEM_WHITE );
    d->m_cbBackground->setEnabled(false);
    connect( d->m_cbBackground, SIGNAL(activated(int) ),
             this, SLOT( slScanBackgroundChanged( int )));

    QToolTip::add( d->m_cbBackground,
                   i18n("Select whether a scan of the empty\n"
                        "scanner glass results in a\n"
                        "black or a white image."));
    connect( d->m_cbAutoSel, SIGNAL(toggled(bool) ), SLOT(slAutoSelToggled(bool)));

    (void) new QLabel( i18n("scanner background"), d->m_autoSelGroup );
    d->m_autoSelGroup->addSpace(2*KDialog::spacingHint());

    QLabel *l1= new QLabel( i18n("Threshold:"), d->m_autoSelGroup );
    d->m_sliderThresh = new QSlider( 0, 254, 10, d->m_autoSelThresh,  Qt::Horizontal,
                                     d->m_autoSelGroup );
    connect( d->m_sliderThresh, SIGNAL(valueChanged(int)), SLOT(slSetAutoSelThresh(int)));
    QToolTip::add( d->m_sliderThresh,
                   i18n("Threshold for autodetection.\n"
                        "All pixels higher (on black background)\n"
                        "or smaller (on white background)\n"
                        "than this are considered to be part of the image."));
    l1->setBuddy(d->m_sliderThresh);
    d->m_sliderThresh->setTickmarks(QSlider::Below);
    d->m_sliderThresh->setTickInterval(25);

#ifdef AUTOSEL_DUSTSIZE
    /** Dustsize-Slider: No deep impact on result **/
    (void) new QLabel( i18n("Dust size:"), d->m_autoSelGroup);
    d->m_sliderDust = new QSlider( 0, 50, 5, d->m_dustsize,  Qt::Horizontal, d->m_autoSelGroup);
    connect( d->m_sliderDust, SIGNAL(valueChanged(int)), SLOT(slSetAutoSelDustsize(int)));
#endif

    /* disable Autoselbox as long as no scanner is connected */
    d->m_autoSelGroup->setEnabled(false);

    left->addWidget(d->m_autoSelGroup);

    /* Labels for the dimension */
    QGroupBox *gbox = new QGroupBox( 1, Horizontal, i18n("Selection"), this, "GROUPBOX" );

    selSize1 = new QLabel( i18n("- mm" ), gbox );
    selSize2 = new QLabel( i18n("- pix" ), gbox );

    gbox->addSpace(KDialog::spacingHint());

    /* size indicator */
    QHBox *hb = new QHBox( gbox );
    (void) new QLabel( i18n( "Size  "), hb );
    fileSize = new SizeIndicator( hb );
    QToolTip::add( fileSize, i18n( "This size field shows how large the uncompressed image will be.\n"
                               "It tries to warn you, if you try to produce huge images by \n"
                               "changing its background color." ));
    fileSize->setText( i18n("-") );

    left->addWidget( gbox, 1 );
    left->addStretch( 6 );

    top->activate();

    scanResX = -1;
    scanResY = -1;
    m_bytesPerPix = -1;

    selectionWidthMm = bedWidth;
    selectionHeightMm = bedHeight;
    updateSelectionDims();
}


Previewer::~Previewer()
{
    delete d;
}


bool Previewer::setPreviewImage( const QImage &image )
{
   if ( image.isNull() )
	return false;

   kdDebug(29000) << k_funcinfo << "setting new image, size=" << image.size() << endl;
   m_previewImage = image;
   img_canvas->newImage( &m_previewImage );

   return true;
}


// TODO: this doesn't belong here at all - move to kookapref?
QString Previewer::galleryRoot()
{
    QString dir = KGlobal::dirs()->saveLocation("data","ScanImages",true);

    if (!dir.endsWith("/")) dir += "/";
    return (dir);
}


void Previewer::newImage(const QImage *image)
{
   kdDebug(29000) << k_funcinfo << "size=" << image->size() << endl;

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
   kdDebug(29000) << k_funcinfo << "to [" << w << "," << h << "]" << endl;

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
    kdDebug(29000) << k_funcinfo << "rect=" << rect << endl;

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


void Previewer::slNewScanResolutions(int xres,int yres)
{
    kdDebug(29000) << k_funcinfo << xres << " x " << yres << endl;

    scanResX = xres;
    scanResY = yres;
    updateSelectionDims();
}


void Previewer::slNewScanMode(int bytes_per_pix)
{
    kdDebug(29000) << k_funcinfo << bytes_per_pix << endl;

    m_bytesPerPix = bytes_per_pix;
    updateSelectionDims();
}


/* This slot is called with the new dimension for the selection
 * in values between 0..1000. It emits signals, that redraw the
 * size labels.
 */

void Previewer::slotNewAreaSelected(QRect rect)
{
    kdDebug(29000) << k_funcinfo << "rect=" << rect << " width=" << bedWidth << " height=" << bedHeight << endl;

    if (rect.isValid())
    {							// convert bedsize/1000 -> mm
        rect.setLeft(static_cast<int>(rect.left()/1000.0*bedWidth+0.5));
        rect.setRight(static_cast<int>(rect.right()/1000.0*bedWidth+0.5));
        rect.setTop(static_cast<int>(rect.top()/1000.0*bedHeight+0.5));
        rect.setBottom(static_cast<int>(rect.bottom()/1000.0*bedHeight+0.5));
        kdDebug(29000) << k_funcinfo << "new rect=" << rect << endl;
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
    selSize1->setText(i18n("%1 x %1 mm").arg(selectionWidthMm).arg(selectionHeightMm));

    /* Calculate file size */
    long size_in_byte = 0;
    if (scanResX>1 && scanResY>1)			// if resolution available
    {
        double w_inch = ((double) selectionWidthMm)/25.4;
        double h_inch = ((double) selectionHeightMm)/25.4;

        int pix_w = static_cast<int>(w_inch*scanResX+0.5);
        int pix_h = static_cast<int>(h_inch*scanResY+0.5);

        selSize2->setText(i18n("%1 x %1 pix").arg(pix_w).arg(pix_h));

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


void Previewer::slScaleToWidth()
{
   if( img_canvas )
   {
      img_canvas->handle_popup( ImageCanvas::ID_FIT_WIDTH );
   }
}

void Previewer::slScaleToHeight()
{
   if( img_canvas )
   {
      img_canvas->handle_popup( ImageCanvas::ID_FIT_HEIGHT);
   }
}


void Previewer::connectScanner(KScanDevice *scan)
{
    kdDebug(29000) << k_funcinfo << endl;
    d->m_scanner = scan;

    if (scan!=NULL)
    {
        d->m_autoSelGroup->setEnabled(true);		// enable the auto-select group
							// set parameters from config
        d->m_cbAutoSel->setChecked(scan->getConfig(CFG_AUTOSEL_DO,QString::null)=="on");
							// "white" is the best default
        bool isWhite = scan->getConfig(CFG_SCANNER_EMPTY_BG,"")!=SCANNER_EMPTY_BLACK;
        d->m_cbBackground->setCurrentItem(isWhite ? BG_ITEM_WHITE : BG_ITEM_BLACK);

        QString h = scan->getConfig(CFG_AUTOSEL_DUSTSIZE,"5");
        d->m_dustsize = h.toInt();

        h = scan->getConfig(CFG_AUTOSEL_THRESH,(isWhite ? DEF_THRESH_WHITE : DEF_THRESH_BLACK));
        d->m_sliderThresh->setValue(h.toInt());
    }
}


void Previewer::setScannerBgIsWhite(bool isWhite)
{
    d->m_bgIsWhite = isWhite;
    if (d->m_scanner!=NULL)
    {
        d->m_cbBackground->setCurrentItem(isWhite ? BG_ITEM_WHITE : BG_ITEM_BLACK);
        d->m_cbBackground->setEnabled(true);

        d->m_scanner->slStoreConfig(CFG_SCANNER_EMPTY_BG,(isWhite ? SCANNER_EMPTY_WHITE : SCANNER_EMPTY_BLACK));
    }
}


/**
 * reads the scanner dependant config file through the m_scanner pointer.
 * If a value for the scanner is not yet known, the function starts up a
 * popup and asks the user.  The result is stored.
 */

void Previewer::checkForScannerBg()
{
    if (d->m_scanner!=NULL)				// scan device already known?
    {
        QString curWhite = d->m_scanner->getConfig(CFG_SCANNER_EMPTY_BG,QString::null);
        bool goWhite = false;

        if (curWhite.isNull())				// not yet known, should ask the user
        {
            kdDebug(29000) << "Dont know the scanner background yet!" << endl;

            goWhite = KMessageBox::questionYesNo(this,
                                                 i18n("The autodetection of images on the preview depends on the background color of the preview image (a preview of an empty scanner).\nPlease select whether the background of the preview image is black or white"),
                                                 i18n("Image Autodetection"),
                                                 i18n("White"),i18n("Black"))==KMessageBox::Yes;
        }
        else
        {
            if (curWhite==SCANNER_EMPTY_WHITE) goWhite = true;
        }

        setScannerBgIsWhite(goWhite);			// set and save that value
    }
}


void Previewer::slScanBackgroundChanged(int indx)
{
    setScannerBgIsWhite(indx==BG_ITEM_WHITE);
}


void Previewer::slAutoSelToggled(bool isOn )
{
    if( isOn )
        checkForScannerBg();

    if( d->m_cbAutoSel )
    {
        QRect r = img_canvas->sel();

        kdDebug(29000) << "The rect is " << r.width() << " x " << r.height() << endl;
        d->m_doAutoSelection = isOn;

        /* Store configuration */
        if( d->m_scanner )
        {
            d->m_scanner->slStoreConfig( CFG_AUTOSEL_DO,
                                         isOn ? "on" : "off" );
        }

        if( isOn && r.width() < 2 && r.height() < 2)  /* There is no selection yet */
        {
            /* if there is already an image, check, if the bg-color is set already */
            if( img_canvas->rootImage() )
            {
                kdDebug(29000) << "No selection -> try to find one!" << endl;

                findSelection();
            }

        }
    }
    if( d->m_sliderThresh )
        d->m_sliderThresh->setEnabled(isOn);
    if( d->m_sliderDust )
        d->m_sliderDust->setEnabled(isOn);
    if( d->m_cbBackground )
        d->m_cbBackground->setEnabled(isOn);

}


void Previewer::slSetAutoSelThresh(int t)
{
    d->m_autoSelThresh = t;
    kdDebug(29000) << "Setting threshold to " << t << endl;
    if( d->m_scanner )
        d->m_scanner->slStoreConfig( CFG_AUTOSEL_THRESH, QString::number(t) );
    findSelection();
}

void Previewer::slSetAutoSelDustsize(int dSize)
{
    d->m_dustsize = dSize;
    kdDebug(29000) << "Setting dustsize to " << dSize << endl;
    findSelection();
}

/**
 * This method tries to find a selection on the preview image automatically.
 * It uses the image of the preview image canvas, the previewer global
 * threshold setting and a dustsize.
 **/
void  Previewer::findSelection( )
{
    kdDebug(29000) << "Searching Selection" << endl;

    kdDebug(29000) << "Threshold: " << d->m_autoSelThresh << endl;
    kdDebug(29000) << "dustsize: " << d->m_dustsize << endl;
    kdDebug(29000) << "isWhite: " << d->m_bgIsWhite << endl;


    if( ! d->m_doAutoSelection ) return;
    int line;
    int x;
    const QImage *img = img_canvas->rootImage();
    if( ! img ) return;

    long iWidth  = img->width();
    long iHeight = img->height();

    QMemArray<long> heightSum;
    QMemArray<long> widthSum;

    kdDebug(29000)<< "Preview size is " << iWidth << "x" << iHeight << endl;

    if( (d->m_heightSum).size() == 0 && (iHeight>0) )
    {
        kdDebug(29000) << "Starting to fill Array " << endl;
        QMemArray<long> heightSum(iHeight);
        QMemArray<long> widthSum(iWidth);
        heightSum.fill(0);
        widthSum.fill(0);

        kdDebug(29000) << "filled  Array with zero " << endl;

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
        kdDebug(29000) << "Resizing now" << endl;
        for( x = 0; x < iWidth; x++ )
            widthSum[x] = widthSum[x]/iHeight;

        kdDebug(29000) << "Filled Arrays successfully" << endl;
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

    kdDebug(29000) << " -- Autodetection -- " <<  endl;
    kdDebug(29000) << "Area top " << r.top() <<  endl;
    kdDebug(29000) << "Area left" << r.left() << endl;
    kdDebug(29000) << "Area bottom " << r.bottom() << endl;
    kdDebug(29000) << "Area right " << r.right() << endl;
    kdDebug(29000) << "Area width " << r.width() << endl;
    kdDebug(29000) << "Area height " << r.height() << endl;

    img_canvas->setSelectionRect(r);
    slotNewAreaSelected(r);
}


/*
 * returns an Array containing the
 */
bool Previewer::imagePiece( QMemArray<long> src, int& start, int& end )
{
    for( uint x = 0; x < src.size(); x++ )
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
