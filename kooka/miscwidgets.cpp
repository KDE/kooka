/*
 *   kscan - a scanning program
 *   Copyright (C) 1998 Ivan Shvedunov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <iostream.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qbutton.h>
#include <qradiobutton.h>

#include "miscwidgets.h"
#include "resource.h"


void ImageHistogram::count()
{
  if( ! img ) return;

  bzero(hits,256*sizeof(int));
  for(int y = 0; y<img->height(); y++)
    for(int x = 0; x<img->width(); x++) {
      QColor c = img->pixel(x,y);
      int I;
      switch(color) {
      case 0:
	I = (c.red()+c.green()+c.blue())/3;
	break;
      case 1:
	I = c.red();
	break;
      case 2:
	I = c.green();
	break;
      default:
	I = c.blue();
      }
      hits[I]++;
    }
  maxhits = hits[1];
  for(int i = 2; i<255; i++)
    if(hits[i]>maxhits) maxhits = hits[i];
}

void ImageHistogram::drawContents(QPainter *p)
{
	if(!maxhits) return;
	for(int i = 0; i<width()-8; i++) {
		p->moveTo(i+4,height()-4);
		int a = (hits[(i*256)/(width()-8)]*(height()-8))/maxhits;
		p->lineTo(i+4,height()-4-a);
	}
}





ImgScaleDialog::ImgScaleDialog( QWidget *parent, int curr_sel,
				int last_custom,
				const char *name )
: QDialog( parent, name, true )
{
   setCaption (I18N ("Image Zoom"));
   selected = curr_sel;   
   int        one_is_selected = false;
   
   QLabel *l1 = new QLabel( I18N("Select image zoom:"), this);
   QSize s1 = l1->sizeHint();
   l1->setGeometry( 5, 10-s1.height()/2 , 220, s1.height());
   // 
   QButtonGroup *radios = new QButtonGroup ( this );
   connect( radios, SIGNAL( clicked( int )),
	    this, SLOT( setSelValue( int )));
   
   radios->setFrameStyle( QFrame::Panel | QFrame::Raised );
   // radios->setFrameWidth( 3 );
   radios->setLineWidth( 1 );
   radios->setGeometry( 5, 20+s1.height()/2, 220, 170 );
   
   const int br = 89;
   // left gap: smaller Image
   QRadioButton *rb25 = new QRadioButton (I18N ("25 %"), radios);
   QSize s = rb25->sizeHint();
   rb25->setGeometry( 25, 20, br, s.height());
   if( curr_sel == 25 ){
      rb25->setChecked( true ); one_is_selected = true; }

   QRadioButton *rb50 = new QRadioButton (I18N ("50 %"), radios );
   rb50->setGeometry( 25, 45, br, s.height());
   if( curr_sel == 50 ){
      rb50->setChecked( true );
      one_is_selected = true;
   }

   QRadioButton *rb75 = new QRadioButton (I18N ("75 %"), radios );
   rb75->setGeometry( 25, 70, br, s.height());
   if( curr_sel == 75 ) {
      rb75->setChecked( true );
      one_is_selected = true;
   }
   
   QRadioButton *rb100 = new QRadioButton (I18N ("100 %"), radios);
   rb100->setGeometry( 25, 95, br, s.height());
   if( curr_sel == 100 ) {
      rb100->setChecked( true );
      one_is_selected = true;
   }
   
   int gy = 130;
   // right gap: Enlarge Image
   QRadioButton *rb150 = new QRadioButton (I18N ("150 %"), radios);
   rb150->setGeometry( gy, 20, br, s.height());
   if( curr_sel == 150 ) {
      rb150->setChecked( true );
      one_is_selected = true;
   }
   
   QRadioButton *rb200 = new QRadioButton (I18N ("200 %"), radios );
   rb200->setGeometry( gy, 45, br, s.height());
   if( curr_sel == 200 ) {
      rb200->setChecked( true );
      one_is_selected = true;
   }
   
   QRadioButton *rb300 = new QRadioButton (I18N ("300 %"), radios );
   rb300->setGeometry( gy, 70, br, s.height());
   if( curr_sel == 300 ) {
      rb300->setChecked( true );
      one_is_selected = true;
   }
   
   QRadioButton *rb400 = new QRadioButton (I18N ("400 %"), radios);
   rb400->setGeometry( gy, 95, br, s.height());
   if( curr_sel == 400 ) {
      rb400->setChecked( true );
      one_is_selected = true;
   }
   
   // Custom Scaler at the bottom
   QRadioButton *rbCust = new QRadioButton (I18N ("Custom Scalefactor:"),
					    radios);
   rbCust->setGeometry( 25, 130, 180, s.height());
   if( ! one_is_selected ) rbCust->setChecked( true );
   
   leCust = new QLineEdit( radios );
   leCust->setGeometry( 170,128, 40, s.height()+4);
   QString sn;
   sn.setNum( last_custom );
   leCust->setText(sn );
   connect( leCust, SIGNAL( textChanged( const QString& )),
	    this, SLOT( customChanged( const QString& )));
   connect( rbCust, SIGNAL( toggled( bool )),
	    this, SLOT(enableAndFocus(bool)));
   leCust->setEnabled( rbCust->isChecked());
   
   
   QPushButton *ok, *cancel;
   ok = new QPushButton( "Ok", this );
   ok->setDefault( true );
   ok->setGeometry( 15, 200+s1.height()/2, 85, 30 );
   connect( ok, SIGNAL(clicked()), SLOT(accept()) );
   cancel = new QPushButton( "Cancel", this );
   cancel->setGeometry( 130, 200+s1.height()/2, 85,30 );
   connect( cancel, SIGNAL(clicked()), SLOT(reject()) );

   // no show - done by exec()
   setFixedSize( 230, 250 );
}

void ImgScaleDialog::customChanged( const QString& s )
{
   bool ok;
   int  okval = s.toInt( &ok );
   if( ok && okval > 5 && okval < 1000 ) {
      selected = okval;
      emit( customScaleChange( okval ));
   } else {
      debug( "ERR: To large, to smale, or whatever shitty !" );
   }
}

// This slot is called, when the user changes the Scale-Selection
// in the button group. The value val is the index of the active
// button which is translated to the Scale-Size in percent.
// If custom size is selected, the ScaleSize is read from the
// QLineedit.
//
void ImgScaleDialog::setSelValue( int val )
{
   const int translator[]={ 25, 50, 75, 100, 150,200,300, 400, -1 };
   const size_t translator_size = sizeof( translator ) / sizeof(int);
   int   old_sel = selected;

   // Check if value is in Range
   if( val >= 0 && val < (int) translator_size ) {
      selected = translator[val];
   
      // Custom size selected
      if( selected == -1 ) {
	 QString s = leCust->text();
      
	 bool ok;
	 int  okval = s.toInt( &ok );
	 if( ok ) {
	    selected = okval;
	    emit( customScaleChange( okval ));
	 } else {
	    selected = old_sel;
	 }
      } // Selection is not custom
   } else {
      debug( "ERR: Invalid size selected !" );
   }
   // debug( "SetValue: Selected Scale is %d", selected );
}

