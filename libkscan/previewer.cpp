
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   $Id$
*/

#include <qlabel.h>
#include <qfontmetrics.h>
#include <qhbox.h>
#include <qtooltip.h>
#include <qpopupmenu.h>
#include <qregexp.h>
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
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <kconfig.h>
#include <qbuttongroup.h>
#include <qvbuttongroup.h>
#include <kmessagebox.h>

#define ID_CUSTOM 0
#define ID_A4     1
#define ID_A5     2
#define ID_A6     3
#define ID_9_13   4
#define ID_10_15  5
#define ID_LETTER 6

#define CFG_AUTOSEL_DO        "doAutoselection"
#define CFG_AUTOSEL_THRESH    "autoselThreshold"
#define CFG_AUTOSEL_DUSTSIZE  "autoselDustsize"

#define CFG_SCANNER_EMPTY_BG  "scannerBackgroundWhite"

Previewer::Previewer(QWidget *parent, const char *name )
    : QWidget(parent,name),
      m_sliderDust(0L),
      m_scanner(0L)
{
    QVBoxLayout *top = new QVBoxLayout( this, 10 );
    layout = new QHBoxLayout( 2 );
    top->addLayout( layout, 9 );
    QVBoxLayout *left = new QVBoxLayout( 3 );
    layout->addLayout( left, 2 );

    /* Load autoselection values from Config file */
    KConfig *cfg = KGlobal::config();
    cfg->setGroup( GROUP_STARTUP );
    m_doAutoSelection = cfg->readBoolEntry( CFG_AUTOSEL_DO, true );
    m_dustsize        = cfg->readNumEntry(  CFG_AUTOSEL_DUSTSIZE, 5 );

    int autoselThreshDefault = 240;
    if( ! m_bgIsWhite )
    {
        autoselThreshDefault = 12;
    }
    m_autoSelThresh = cfg->readNumEntry( CFG_AUTOSEL_THRESH, autoselThreshDefault );

    /* Units etc. TODO: get from Config */
    sizeUnit = KRuler::Millimetres;
    displayUnit = sizeUnit;
    m_autoSelThresh = 240;

    overallHeight = 295;  /* Default DIN A4 */
    overallWidth = 210;
    kdDebug(29000) << "Previewer: got Overallsize: " <<
        overallWidth << " x " << overallHeight << endl;
    img_canvas  = new ImageCanvas( this );

    img_canvas->setScaleFactor( 0 );
    img_canvas->enableContextMenu(true);

    layout->addWidget( img_canvas, 6 );

    /* Actions for the previewer zoom */
    KAction *act;
    act =  new KAction(i18n("Scale to W&idth"), "scaletowidth", CTRL+Key_I,
		       this, SLOT( slScaleToWidth()), this );
    act->plug( img_canvas->contextMenu());

    act = new KAction(i18n("Scale to &Height"), "scaletoheight", CTRL+Key_H,
		      this, SLOT( slScaleToHeight()), this );
    act->plug( img_canvas->contextMenu());

    /*Signals: Control the custom-field and show size of selection */
    connect( img_canvas, SIGNAL(newRect()),      this, SLOT(slCustomChange()));
    connect( img_canvas, SIGNAL(newRect(QRect)), this, SLOT(slNewDimen(QRect)));

    /* Stuff for the preview-Notification */
    left->addWidget( new QLabel( i18n("<B>Preview</B>"), this ), 1);

    // Create a button group to contain buttons for Portrait/Landscape
    bgroup = new QVButtonGroup( i18n("Scan Size"), this );

    // -----
    pre_format_combo = new QComboBox( bgroup, "PREVIEWFORMATCOMBO" );
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
    QGroupBox *grBox = new QGroupBox( 1, Horizontal, i18n("Autoselection"), this);
    m_cbAutoSel = new QCheckBox( i18n("active"), grBox );
    m_cbAutoSel->setChecked( m_doAutoSelection );
    connect( m_cbAutoSel, SIGNAL(toggled(bool) ), SLOT(slAutoSelToggled(bool)));

    QLabel *l1= new QLabel( i18n("Thresh&old:"), grBox );
    m_sliderThresh = new QSlider( 0, 254, 10, m_autoSelThresh,  Qt::Horizontal, grBox );
    connect( m_sliderThresh, SIGNAL(valueChanged(int)), SLOT(slAutoSelThresh(int)));
    l1->setBuddy(m_sliderThresh);

#if 0  /** Dustsize-Slider: No deep impact on result **/
    (void) new QLabel( i18n("Dustsize:"), grBox );
    m_sliderDust = new QSlider( 0, 50, 5, m_dustsize,  Qt::Horizontal, grBox );
    connect( m_sliderDust, SIGNAL(valueChanged(int)), SLOT(slAutoSelDustsize(int)));
#endif

    left->addWidget( grBox );

    /* Needed to apply the settings if active or not */
    slAutoSelToggled(m_doAutoSelection);

    /* Labels for the dimension */
    QGroupBox *gbox = new QGroupBox( 1, Horizontal, i18n("Selection"), this, "GROUPBOX" );

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
    KConfig *cfg = KGlobal::config();

    cfg->setGroup( GROUP_STARTUP );
    cfg->writeEntry( CFG_AUTOSEL_DO, m_doAutoSelection );
    cfg->writeEntry( CFG_AUTOSEL_DUSTSIZE, m_dustsize  );
}

bool Previewer::loadPreviewImage( const QString forScanner )
{
   const QString prevFile = previewFile( forScanner );
   kdDebug(29000) << "Loading preview for " << forScanner << " from file " << prevFile << endl;

   bool re = m_previewImage.load( prevFile );
   if( re )
   {
      img_canvas->newImage( &m_previewImage );
   }
   return re;
}

/* extension needs to be added */
QString Previewer::previewFile( const QString& scanner )
{
   QString fname = galleryRoot() + QString::fromLatin1(".previews/");
   QRegExp rx( "/" );
   QString sname( scanner );
   sname.replace( rx, "_");

   fname += sname;

   kdDebug(29000) << "ImgSaver: returning preview file without extension: " << fname << endl;
   return( fname );
}

QString Previewer::galleryRoot()
{
   QString dir = (KGlobal::dirs())->saveLocation( "data", "ScanImages", true );

   if( dir.right(1) != "/" )
      dir += "/";

   return( dir );

}

void Previewer::newImage( QImage *ni )
{
   /* image canvas does not copy the image, so we hold a copy here */
   m_previewImage = *ni;
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


void Previewer::slSetScannerBgIsWhite( bool b )
{
    m_bgIsWhite = b;

    if( m_scanner )
    {
        m_scanner->slStoreConfig( CFG_SCANNER_EMPTY_BG, b ? QString("Yes") : QString("No"));
    }
}

/**
 * reads the scanner dependant config file through the m_scanner pointer.
 * If a value for the scanner is not yet known, the function starts up a
 * popup and asks the user.  The result is stored.
 */
void Previewer::checkForScannerBg()
{
    if( m_scanner ) /* Is the scan device already known? */
    {
        QString isWhite = m_scanner->getConfig( CFG_SCANNER_EMPTY_BG, "unknown" );
        if( isWhite == "unknown" )
        {
            /* not yet known, should ask the user. */
            kdDebug(29000) << "Dont know the scanner background yet!" << endl;

            if( KMessageBox::questionYesNo( this,
                                            i18n("The Autodection of images on the preview depends on the background color of the preview image (Think of a preview of an empty scanner).\nPlease select if the background of the preview image is black or white"),
                                            i18n("Image Autodetection"),
                                            i18n("White"), i18n("Black") ) == KMessageBox::Yes )
            {
                isWhite = "Yes";
            }
            kdDebug(29000) << "User said " << isWhite << endl;

        }

        /* remember value */
        slSetScannerBgIsWhite( (isWhite == QString("Yes")) );
    }
}

void Previewer::slAutoSelToggled(bool isOn )
{
    kdDebug(29000) << "Toggeln: " << isOn << endl;

    if( isOn )
        checkForScannerBg();

    if( m_cbAutoSel )
    {
        QRect r = img_canvas->sel();

        kdDebug(29000) << "The rect is " << r.width() << " x " << r.height() << endl;
        m_doAutoSelection = isOn;
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
    if( m_sliderThresh )
        m_sliderThresh->setEnabled(isOn);
    if( m_sliderDust )
        m_sliderDust->setEnabled(isOn);

}


void Previewer::slAutoSelThresh(int t)
{
    m_autoSelThresh = t;
    kdDebug(29000) << "Setting threshold to " << t << endl;
    findSelection();
}

void Previewer::slAutoSelDustsize(int d)
{
    m_dustsize = d;
    kdDebug(29000) << "Setting dustsize to " << d << endl;
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
    if( ! m_doAutoSelection ) return;
    long line;
    long x;
    const QImage *img = img_canvas->rootImage();
    if( ! img ) return;

    long iWidth  = img->width();
    long iHeight = img->height();

    QMemArray<long> heightSum( iHeight );
    QMemArray<long> widthSum( iWidth );
    heightSum.fill(0);
    widthSum.fill(0);

    for( line = 0; line < iHeight; line++ )
    {

        for( x = 0; x < iWidth; x++ )
        {
            int gray  = qGray( img->pixel( x, line ));
            // kdDebug(29000) << "Gray-Value: " << gray << endl;
            heightSum[line]   += gray;
            widthSum[x]       += gray;
        }
        heightSum[line] /= iWidth;
    }
    /* Divide by amount of pixels */
    for( x = 0; x < iWidth; x++ )
        widthSum[x] /= iHeight;

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
                str << x << '\t' << heightSum[x] << endl;
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
            str << x << '\t' << widthSum[x] << endl;

        fi1.close();
    }
#endif
    int start = 0;
    int end = 0;
    QRect r;

    /** scale to 0..1000 range **/
    start = 0;
    end   = 0;
    imagePiece( heightSum, start, end ); // , m_threshold, m_dustsize, false );

    r.setTop(  1000*start/iHeight );
    r.setBottom( 1000*end/iHeight);
    // r.setTop(  start );
    // r.setBottom( end );

    start = 0;
    end   = 0;
    imagePiece( widthSum, start, end ); // , m_threshold, m_dustsize, false );
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
        if( ! m_bgIsWhite )
        {
            /* pixelvalue needs to be higher than threshold, white background */
            if( src[x] > m_autoSelThresh )
            {
                /* Ok this pixel could be the start */
                int iStart = x;
                int iEnd = x;
                x++;
                while( x < src.size() && src[x] > m_autoSelThresh )
                {
                    x++;
                }
                iEnd = x;

                int delta = iEnd-iStart;

                if( delta > m_dustsize && end-start < delta )
                {
                    start = iStart;
                    end   = iEnd;
                }
            }
        }
        else
        {
            /* pixelvalue needs to be lower than threshold, black background */
            if( src[x] < m_autoSelThresh )
            {
                int iStart = x;
                int iEnd = x;
                x++;
                while( x < src.size() && src[x] < m_autoSelThresh )
                {
                    x++;
                }
                iEnd = x;

                int delta = iEnd-iStart;

                if( delta > m_dustsize && end-start < delta )
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
