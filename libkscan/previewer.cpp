/***************************************************************************
           previewer.cpp  -  class that implements a scan preview
                             -------------------                                         
    begin                : Thu Jun 22 2000                                           
    copyright            : (C) 2000 by Klaas Freitag                         
    email                : freitag@suse.de                                     
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/
#include <qlabel.h>
#include <qfontmetrics.h>

#include <kdebug.h>
#include <klocale.h>

#include "previewer.h"
#include "img_canvas.h"

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
   layout = new QHBoxLayout( this, 2 );
   QVBoxLayout *left = new QVBoxLayout( 3 );
   layout->addLayout( left, 2 );
	
	
   sizeUnit = KRuler::Millimetres;
   displayUnit = sizeUnit;
	
   overallHeight = 295;  /* Default DIN A4 */
   overallWidth = 210;
   kdDebug(29000) << "Previewer: got Overallsize: " << overallWidth << " x " << overallHeight << endl;
   img_canvas  = new ImageCanvas( this );
   img_canvas->setScaleFactor( 0 );
   layout->addWidget( img_canvas, 6 );
	
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
   left->addWidget( gbox, 1 );

   left->addStretch( 6 );

   layout->activate();

   /* Preset custom Cutting */
   pre_format_combo->setCurrentItem( ID_CUSTOM );
   slFormatChange( ID_CUSTOM);
}

Previewer::~Previewer()
{

}


void Previewer::newImage( QImage *ni )
{
   img_canvas->newImage( ni );
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

/* This slot is called with the new dimension for the selection
 * in values between 0..1000. It emits signals, that redraw the
 * size labels.
 */
void Previewer::slNewDimen(QRect r)
{
   int w_mm = 0;
   int h_mm = 0;

   if( r.height() > 0)
        w_mm = (int) (overallWidth / 1000 * r.width());
   if( r.width() > 0)
        h_mm = (int) (overallHeight / 1000 * r.height());

   QString s;
   s = i18n("width %1 mm").arg(w_mm);
   emit(setScanWidth(s));
   kdDebug(29000) << "Setting new Dimension " << s << endl;
   s = i18n("height %1 mm").arg(h_mm);
   emit(setScanHeight(s));
}


QPoint Previewer::calcPercent( int w_mm, int h_mm )
{
	QPoint p(0,0);
	if( overallWidth < 1.0 || overallHeight < 1.0 ) return( p );

 	if( sizeUnit == KRuler::Millimetres ) {
 		p.setX( 1000.0*w_mm / overallWidth );
 		p.setY( 1000.0*h_mm / overallHeight );
 	} else {
 		kdDebug(29000) << "ERROR: Only mm supported yet !" << endl;
 	}
 	return( p );

}
#include "previewer.moc"
