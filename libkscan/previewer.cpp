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

#include <kdebug.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kaction.h>
#include <kstandarddirs.h>

#include "previewer.h"
#include "img_canvas.h"
#include "sizeindicator.h"

#define ID_CUSTOM 0
#define ID_A4     1
#define ID_A5     2
#define ID_A6     3
#define ID_9_13   4
#define ID_10_15  5
#define ID_LETTER 6



Previewer::Previewer(QWidget *parent, const char *name )
   : QWidget(parent,name)
{
   QVBoxLayout *top = new QVBoxLayout( this, 10 );
   layout = new QHBoxLayout( 2 );
   top->addLayout( layout, 9 );
   QVBoxLayout *left = new QVBoxLayout( 3 );
   layout->addLayout( left, 2 );
	
	
   sizeUnit = KRuler::Millimetres;
   displayUnit = sizeUnit;
	
   overallHeight = 295;  /* Default DIN A4 */
   overallWidth = 210;
   kdDebug(29000) << "Previewer: got Overallsize: " << overallWidth << " x " << overallHeight << endl;
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
   connect( img_canvas, SIGNAL(newRect()), this, SLOT(slCustomChange()));
   connect( img_canvas, SIGNAL(newRect(QRect)), this, SLOT(slNewDimen(QRect)));
	
   /* Stuff for the preview-Notification */
   left->addWidget( new QLabel( i18n("<B>Preview</B>"), this ), 1);

   pre_format_combo = new QComboBox( this, "PREVIEWFORMATCOMBO" );
   pre_format_combo->insertItem( i18n( "Custom" ), ID_CUSTOM);
   pre_format_combo->insertItem( i18n( "DIN A4" ), ID_A4);
   pre_format_combo->insertItem( i18n( "DIN A5" ), ID_A5);
   pre_format_combo->insertItem( i18n( "DIN A6" ), ID_A6);
   pre_format_combo->insertItem( i18n( "9x13 cm" ), ID_9_13 );
   pre_format_combo->insertItem( i18n( "10x15 cm" ), ID_10_15 );
   pre_format_combo->insertItem( i18n( "Letter" ), ID_LETTER);


   connect( pre_format_combo, SIGNAL(activated (int)),
	    this, SLOT( slFormatChange(int)));

   QLabel *l1 = new QLabel( i18n( "Select s&can size:" ), this );
   l1->setBuddy( pre_format_combo );
   left->addWidget( l1, 1 );
   left->addWidget( pre_format_combo, 1 );


   // Create a button group to contain buttons for Portrait/Landscape
   bgroup = new QButtonGroup( 2, Vertical, i18n("Orientation"), this );

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

}

bool Previewer::loadPreviewImage( const QString forScanner )
{
   const QString prevFile = previewFile( forScanner );
   kdDebug(28000) << "Loading preview for " << forScanner << " from file " << prevFile << endl;
   
   if( m_previewImage.load( prevFile ))
   {
      img_canvas->newImage( &m_previewImage );
   }
}

/* extension needs to be added */
QString Previewer::previewFile( const QString& scanner )
{
   QString fname = galleryRoot() + QString::fromLatin1(".previews/");
   QRegExp rx( "/" );
   QString sname( scanner );
   sname.replace( rx, "_");
 
   fname += sname;

   kdDebug(28000) << "ImgSaver: returning preview file without extension: " << fname << endl;
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

#include "previewer.moc"
