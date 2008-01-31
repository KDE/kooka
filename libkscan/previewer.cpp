
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

#include <qlabel.h>
#include <qfontmetrics.h>
#include <qhbox.h>
#include <qtooltip.h>
#include <qpopupmenu.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qcombobox.h>
#include <qradiobutton.h>
#include <qgroupbox.h>
#include <qlayout.h>

#include <kdebug.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kaction.h>
#include <kstandarddirs.h>

#include "previewer.h"
#include "img_canvas.h"
#include "sizeindicator.h"
#include "devselector.h" /* for definition of config key :( */
#include "kscandevice.h"
#include <qslider.h>
#include <qcheckbox.h>
#include <kconfig.h>
#include <qbuttongroup.h>
#include <qvbuttongroup.h>
#include <kmessagebox.h>
#include <qvaluevector.h>

#define ID_CUSTOM 0
#define ID_A4     1
#define ID_A5     2
#define ID_A6     3
#define ID_9_13   4
#define ID_10_15  5
#define ID_LETTER 6

/** Config tags for autoselection **/
#define CFG_AUTOSEL_DO        "doAutoselection"   /* do it or not */
#define CFG_AUTOSEL_THRESH    "autoselThreshold"  /* threshold    */
#define CFG_AUTOSEL_DUSTSIZE  "autoselDustsize"   /* dust size    */
/* tag if a scan of the empty scanner results in black or white image */
#define CFG_SCANNER_EMPTY_BG  "scannerBackgroundWhite"

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

Previewer::Previewer(QWidget *parent, const char *name )
    : QWidget(parent,name)
{
    d = new PreviewerPrivate();

    // beautification to look like the left scanparams widget in the dialog
    QHBoxLayout *htop = new QHBoxLayout( this );
    QFrame *frame = new QFrame( this );
    frame->setFrameStyle( QFrame::Panel | QFrame::Raised );
    frame->setLineWidth( 1 );
    htop->addWidget( frame );

    QVBoxLayout *top = new QVBoxLayout( frame, KDialog::marginHint(), KDialog::spacingHint() );
    layout = new QHBoxLayout( KDialog::spacingHint() );
    top->addLayout( layout, 9 );
    QVBoxLayout *left = new QVBoxLayout( KDialog::spacingHint() );
    layout->addLayout( left, 2 );

    /* Load autoselection values from Config file */
    KConfig *cfg = KGlobal::config();
    cfg->setGroup( GROUP_STARTUP );

    /* Units etc. TODO: get from Config */
    sizeUnit = KRuler::Millimetres;
    displayUnit = sizeUnit;
    d->m_autoSelThresh = 240;

    overallHeight = 295;  /* Default DIN A4 */
    overallWidth = 210;
    kdDebug(29000) << "Previewer: got Overallsize: " <<
        overallWidth << " x " << overallHeight << endl;
    img_canvas  = new ImageCanvas( frame );

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
    connect( img_canvas, SIGNAL(newRect()),      this, SLOT(slCustomChange()));
    connect( img_canvas, SIGNAL(newRect(QRect)), this, SLOT(slNewDimen(QRect)));

    /* Stuff for the preview-Notification */
    left->addWidget( new QLabel( i18n("<B>Preview</B>"), frame ), 1);

    // Create a button group to contain buttons for Portrait/Landscape
    bgroup = new QVButtonGroup( i18n("Scan Size"), frame );

    // -----
    pre_format_combo = new QComboBox( frame, "PREVIEWFORMATCOMBO" );
    pre_format_combo->insertItem( i18n( "Custom" ), ID_CUSTOM);
    pre_format_combo->insertItem( i18n( "DIN A4" ), ID_A4);
    pre_format_combo->insertItem( i18n( "DIN A5" ), ID_A5);
    pre_format_combo->insertItem( i18n( "DIN A6" ), ID_A6);
    pre_format_combo->insertItem( i18n( "9x13 cm" ), ID_9_13 );
    pre_format_combo->insertItem( i18n( "10x15 cm" ), ID_10_15 );
    pre_format_combo->insertItem( i18n( "Letter" ), ID_LETTER);

    connect( pre_format_combo, SIGNAL(activated (int)),
             this, SLOT( slFormatChange(int)));

    left->addWidget( pre_format_combo, 1 );

    /** Potrait- and Landscape Selector **/
    QFontMetrics fm = bgroup->fontMetrics();
    int w = fm.width( (const QString)i18n(" Landscape " ) );
    int h = fm.height( );

    rb1 = new QRadioButton( i18n("&Landscape"), bgroup );
    landscape_id = bgroup->id( rb1 );
    rb2 = new QRadioButton( i18n("P&ortrait"),  bgroup );
    portrait_id = bgroup->id( rb2 );
    bgroup->setButton( portrait_id );

    connect(bgroup, SIGNAL(clicked(int)), this, SLOT(slOrientChange(int)));

    int rblen = 5+w+12;  // 12 for the button?
    rb1->setGeometry( 5, 6, rblen, h );
    rb2->setGeometry( 5, 1+h/2+h, rblen, h );

    left->addWidget( bgroup, 2 );


    /** Autoselection Box **/
    d->m_autoSelGroup = new QGroupBox( 1, Horizontal, i18n("Auto-Selection"), frame);

    QHBox *hbox       = new QHBox(d->m_autoSelGroup);
    d->m_cbAutoSel    = new QCheckBox( i18n("Active on"), hbox );
    QToolTip::add( d->m_cbAutoSel, i18n("Check here if you want autodetection\n"
                                        "of the document on the preview."));

    /* combobox to select if black or white background */
    d->m_cbBackground = new QComboBox( hbox );
    d->m_cbBackground->insertItem(i18n("Black"), BG_ITEM_BLACK );
    d->m_cbBackground->insertItem(i18n("White"), BG_ITEM_WHITE );
    connect( d->m_cbBackground, SIGNAL(activated(int) ),
             this, SLOT( slScanBackgroundChanged( int )));


    QToolTip::add( d->m_cbBackground,
                   i18n("Select whether a scan of the empty\n"
                        "scanner glass results in a\n"
                        "black or a white image."));
    connect( d->m_cbAutoSel, SIGNAL(toggled(bool) ), SLOT(slAutoSelToggled(bool)));

    (void) new QLabel( i18n("scanner background"), d->m_autoSelGroup );

    QLabel *l1= new QLabel( i18n("Thresh&old:"), d->m_autoSelGroup );
    d->m_sliderThresh = new QSlider( 0, 254, 10, d->m_autoSelThresh,  Qt::Horizontal,
                                     d->m_autoSelGroup );
    connect( d->m_sliderThresh, SIGNAL(valueChanged(int)), SLOT(slSetAutoSelThresh(int)));
    QToolTip::add( d->m_sliderThresh,
                   i18n("Threshold for autodetection.\n"
                        "All pixels higher (on black background)\n"
                        "or smaller (on white background)\n"
                        "than this are considered to be part of the image."));
    l1->setBuddy(d->m_sliderThresh);

#if 0  /** Dustsize-Slider: No deep impact on result **/
    (void) new QLabel( i18n("Dust size:"), grBox );
    d->m_sliderDust = new QSlider( 0, 50, 5, d->m_dustsize,  Qt::Horizontal, grBox );
    connect( d->m_sliderDust, SIGNAL(valueChanged(int)), SLOT(slSetAutoSelDustsize(int)));
#endif

    /* disable Autoselbox as long as no scanner is connected */
    d->m_autoSelGroup->setEnabled(false);

    left->addWidget(d->m_autoSelGroup);

    /* Labels for the dimension */
    QGroupBox *gbox = new QGroupBox( 1, Horizontal, i18n("Selection"), frame, "GROUPBOX" );

    QLabel *l2 = new QLabel( i18n("width - mm" ), gbox );
    QLabel *l3 = new QLabel( i18n("height - mm" ), gbox );

    connect( this, SIGNAL(setScanWidth(const QString&)),
             l2, SLOT(setText(const QString&)));
    connect( this, SIGNAL(setScanHeight(const QString&)),
             l3, SLOT(setText(const QString&)));

    /* size indicator */
    QHBox *hb = new QHBox( gbox );
    (void) new QLabel( i18n( "Size:"), hb );
    SizeIndicator *indi = new SizeIndicator( hb );
    QToolTip::add( indi, i18n( "This size field shows how large the uncompressed image will be.\n"
                               "It tries to warn you, if you try to produce huge images by \n"
                               "changing its background color." ));
    indi->setText( i18n("-") );

    connect( this, SIGNAL( setSelectionSize(long)),
             indi, SLOT(   setSizeInByte   (long)) );

    left->addWidget( gbox, 1 );

    left->addStretch( 6 );

    top->activate();

    /* Preset custom Cutting */
    pre_format_combo->setCurrentItem( ID_CUSTOM );
    slFormatChange( ID_CUSTOM);

    scanResX = -1;
    scanResY = -1;
    pix_per_byte = 1;

    selectionWidthMm = 0.0;
    selectionHeightMm = 0.0;
    recalcFileSize();
}

Previewer::~Previewer()
{
    delete d;
}

bool Previewer::setPreviewImage( const QImage &image )
{
   if ( image.isNull() )
	return false;

   m_previewImage = image;
   img_canvas->newImage( &m_previewImage );

   return true;
}

QString Previewer::galleryRoot()
{
   QString dir = (KGlobal::dirs())->saveLocation( "data", "ScanImages", true );

   if( !dir.endsWith("/") )
      dir += "/";

   return( dir );

}

void Previewer::newImage( QImage *ni )
{
   /* image canvas does not copy the image, so we hold a copy here */
   m_previewImage = *ni;

   /* clear the auto detection arrays */
   d->m_heightSum.resize( 0 );
   d->m_widthSum.resize( 0 );

   img_canvas->newImage( &m_previewImage );
   findSelection( );
}

void Previewer::setScanSize( int w, int h, KRuler::MetricStyle unit )
{
   overallWidth = w;
   overallHeight = h;
   sizeUnit = unit;
}


void Previewer::slSetDisplayUnit( KRuler::MetricStyle unit )
{
   displayUnit = unit;
}


void Previewer::slOrientChange( int id )
{
   (void) id;
   /* Gets either portrait or landscape-id */
   /* Just read the format-selection and call slFormatChange */
   slFormatChange( pre_format_combo->currentItem() );
}

/** Slot called whenever the format selection combo changes. **/
void Previewer::slFormatChange( int id )
{
   QPoint p(0,0);
   bool lands_allowed;
   bool portr_allowed;
   bool setSelection = true;
   int s_long = 0;
   int s_short= 0;

   isCustom = false;

   switch( id )
   {
      case ID_LETTER:
	 s_long = 294;
	 s_short = 210;
	 lands_allowed = false;
	 portr_allowed = true;
	 break;
      case ID_CUSTOM:
	 lands_allowed = false;
	 portr_allowed = false;
	 setSelection = false;
	 isCustom = true;
	 break;
      case ID_A4:
	 s_long = 297;
	 s_short = 210;
	 lands_allowed = false;
	 portr_allowed = true;
	 break;
      case ID_A5:
	 s_long = 210;
	 s_short = 148;
	 lands_allowed = true;
	 portr_allowed = true;
	 break;
      case ID_A6:
	 s_long = 148;
	 s_short = 105;
	 lands_allowed = true;
	 portr_allowed = true;
	 break;
      case ID_9_13:
	 s_long = 130;
	 s_short = 90;
	 lands_allowed = true;
	 portr_allowed = true;
	 break;
      case ID_10_15:
	 s_long = 150;
	 s_short = 100;
	 lands_allowed = true;
	 portr_allowed = true;
	 break;
      default:
	 lands_allowed = true;
	 portr_allowed = true;
	 setSelection = false;
	 break;
   }

   rb1->setEnabled( lands_allowed );
   rb2->setEnabled( portr_allowed );

   int format_id = bgroup->id( bgroup->selected() );
   if( !lands_allowed && format_id == landscape_id )
   {
      bgroup->setButton( portrait_id );
      format_id = portrait_id;
   }
   /* Convert the new dimension to a new QRect and call slot in canvas */
   if( setSelection )
   {
      QRect newrect;
      newrect.setRect( 0,0, p.y(), p.x() );

      if( format_id == portrait_id )
      {   /* Portrait Mode */
	 p = calcPercent( s_short, s_long );
	 kdDebug(29000) << "Now is portrait-mode" << endl;
      }
      else
      {   /* Landscape-Mode */
	 p = calcPercent( s_long, s_short );
      }

      newrect.setWidth( p.x() );
      newrect.setHeight( p.y() );

      img_canvas->newRectSlot( newrect );
   }
}

/* This is called when the user fiddles around in the image.
 * This makes the selection custom-sized immediately.
 */
void Previewer::slCustomChange( void )
{
   if( isCustom )return;
   pre_format_combo->setCurrentItem(ID_CUSTOM);
   slFormatChange( ID_CUSTOM );
}


void Previewer::slNewScanResolutions( int x, int y )
{
   kdDebug(29000) << "got new Scan Resolutions: " << x << "|" << y << endl;
   scanResX = x;
   scanResY = y;

   recalcFileSize();
}


/* This slot is called with the new dimension for the selection
 * in values between 0..1000. It emits signals, that redraw the
 * size labels.
 */
void Previewer::slNewDimen(QRect r)
{
   if( r.height() > 0)
        selectionWidthMm = (overallWidth / 1000 * r.width());
   if( r.width() > 0)
        selectionHeightMm = (overallHeight / 1000 * r.height());

   QString s;
   s = i18n("width %1 mm").arg( int(selectionWidthMm));
   emit(setScanWidth(s));

   kdDebug(29000) << "Setting new Dimension " << s << endl;
   s = i18n("height %1 mm").arg(int(selectionHeightMm));
   emit(setScanHeight(s));

   recalcFileSize( );

}

void Previewer::recalcFileSize( void )
{
   /* Calculate file size */
   long size_in_byte = 0;
   if( scanResY > -1 && scanResX > -1 )
   {
      double w_inch = ((double) selectionWidthMm) / 25.4;
      double h_inch = ((double) selectionHeightMm) / 25.4;

      int pix_w = int( w_inch * double( scanResX ));
      int pix_h = int( h_inch * double( scanResY ));

      size_in_byte = pix_w * pix_h / pix_per_byte;
   }

   emit( setSelectionSize( size_in_byte ));
}


QPoint Previewer::calcPercent( int w_mm, int h_mm )
{
	QPoint p(0,0);
	if( overallWidth < 1.0 || overallHeight < 1.0 ) return( p );

 	if( sizeUnit == KRuler::Millimetres ) {
 		p.setX( static_cast<int>(1000.0*w_mm / overallWidth) );
 		p.setY( static_cast<int>(1000.0*h_mm / overallHeight) );
 	} else {
 		kdDebug(29000) << "ERROR: Only mm supported yet !" << endl;
 	}
 	return( p );

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

void Previewer::slConnectScanner( KScanDevice *scan )
{
    kdDebug(29000) << "Connecting scan device!" << endl;
    d->m_scanner = scan;

    if( scan )
    {
        /* Enable the by-default disabled autoselection group */
        d->m_autoSelGroup->setEnabled(true);
        QString h;

        h = scan->getConfig( CFG_AUTOSEL_DO, QString("unknown") );
        if( h == QString("on") )
            d->m_cbAutoSel->setChecked(true);
        else
            d->m_cbAutoSel->setChecked(false);

        QString isWhite = d->m_scanner->getConfig( CFG_SCANNER_EMPTY_BG, "unknown" );

        h = scan->getConfig( CFG_AUTOSEL_DUSTSIZE, QString("5") );
        d->m_dustsize = h.toInt();

        QString  thresh = DEF_THRESH_BLACK; /* for black */
        if( isWhite.lower() == "yes" )
            thresh = DEF_THRESH_WHITE;

        h = scan->getConfig( CFG_AUTOSEL_THRESH, thresh );
        d->m_sliderThresh->setValue( h.toInt() );
    }
}

void Previewer::slSetScannerBgIsWhite( bool b )
{
    d->m_bgIsWhite = b;

    if( d->m_scanner )
    {
        if( b )  // The background _is_ white
        {
            d->m_cbBackground->setCurrentItem( BG_ITEM_WHITE );
        }
        else
        {
            d->m_cbBackground->setCurrentItem( BG_ITEM_BLACK );
        }

        d->m_scanner->slStoreConfig( CFG_SCANNER_EMPTY_BG, b ? QString("Yes") : QString("No"));
    }
}

/**
 * reads the scanner dependant config file through the m_scanner pointer.
 * If a value for the scanner is not yet known, the function starts up a
 * popup and asks the user.  The result is stored.
 */
void Previewer::checkForScannerBg()
{
    if( d->m_scanner ) /* Is the scan device already known? */
    {
        QString isWhite = d->m_scanner->getConfig( CFG_SCANNER_EMPTY_BG, "unknown" );
        bool goWhite = false;
        if( isWhite == "unknown" )
        {
            /* not yet known, should ask the user. */
            kdDebug(29000) << "Dont know the scanner background yet!" << endl;

            goWhite = ( KMessageBox::questionYesNo( this,
                                                    i18n("The autodetection of images on the preview depends on the background color of the preview image (Think of a preview of an empty scanner).\nPlease select whether the background of the preview image is black or white"),
                                                    i18n("Image Autodetection"),
                                                    i18n("White"), i18n("Black") ) == KMessageBox::Yes );
            kdDebug(29000) << "User said " << isWhite << endl;

        }
        else
        {
            if( isWhite.lower() == "yes" )
                goWhite = true;
        }

        /* remember value */
        slSetScannerBgIsWhite( goWhite );
    }
}

void Previewer::slScanBackgroundChanged( int indx )
{
    slSetScannerBgIsWhite( indx == BG_ITEM_WHITE );
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
#if 0
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

    img_canvas->newRectSlot( r );
    slCustomChange();
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

#include "previewer.moc"
