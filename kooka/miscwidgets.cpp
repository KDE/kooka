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
#include <qvbox.h>
#include <qcheckbox.h>
#include <qbutton.h>
#include <qradiobutton.h>
#include <qlabel.h>

#include <kdebug.h>
#include <klocale.h>

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



/* ############################################################################## */


ImgScaleDialog::ImgScaleDialog( QWidget *parent, int curr_sel,
				const char *name )
   :KDialogBase( parent,  name , true, i18n("Zoom"),
                 Ok|Cancel, Ok, true )
{
   // setCaption (i18n ("Image Zoom"));
   selected = curr_sel;
   int        one_is_selected = false;
   enableButtonSeparator( false );
   QVBox *main = makeVBoxMainWidget( );

   // (void) new QLabel( , main, "Page");
   //
   QButtonGroup *radios = new QButtonGroup ( 2, Qt::Horizontal, main );
   CHECK_PTR(radios);
   radios->setTitle( i18n("Select image zoom:") );

   connect( radios, SIGNAL( clicked( int )),
	    this, SLOT( setSelValue( int )));

   // left gap: smaller Image
   QRadioButton *rb25 = new QRadioButton (i18n ("25 %"), radios);
   if( curr_sel == 25 ){
      rb25->setChecked( true );
      one_is_selected = true;
   }

   QRadioButton *rb50 = new QRadioButton (i18n ("50 %"), radios );
   if( curr_sel == 50 ){
      rb50->setChecked( true );
      one_is_selected = true;
   }

   QRadioButton *rb75 = new QRadioButton (i18n ("75 %"), radios );
   if( curr_sel == 75 ) {
      rb75->setChecked( true );
      one_is_selected = true;
   }

   QRadioButton *rb100 = new QRadioButton (i18n ("100 %"), radios);
   if( curr_sel == 100 ) {
      rb100->setChecked( true );
      one_is_selected = true;
   }

   QRadioButton *rb150 = new QRadioButton (i18n ("150 %"), radios);
   if( curr_sel == 150 ) {
      rb150->setChecked( true );
      one_is_selected = true;
   }

   QRadioButton *rb200 = new QRadioButton (i18n ("200 %"), radios );
   if( curr_sel == 200 ) {
      rb200->setChecked( true );
      one_is_selected = true;
   }

   QRadioButton *rb300 = new QRadioButton (i18n ("300 %"), radios );
   if( curr_sel == 300 ) {
      rb300->setChecked( true );
      one_is_selected = true;
   }

   QRadioButton *rb400 = new QRadioButton (i18n ("400 %"), radios);
   if( curr_sel == 400 ) {
      rb400->setChecked( true );
      one_is_selected = true;
   }

   // Custom Scaler at the bottom
   QRadioButton *rbCust = new QRadioButton (i18n ("Custom scale factor:"),
					    radios);
   if( ! one_is_selected )
      rbCust->setChecked( true );
      
   
   leCust = new QLineEdit( radios );
   QString sn;
   sn.setNum(curr_sel );
   
   leCust->setText(sn );
   connect( leCust, SIGNAL( textChanged( const QString& )),
	    this, SLOT( customChanged( const QString& )));
   connect( rbCust, SIGNAL( toggled( bool )),
	    this, SLOT(enableAndFocus(bool)));
   leCust->setEnabled( rbCust->isChecked());
   

}

void ImgScaleDialog::customChanged( const QString& s )
{
   bool ok;
   int  okval = s.toInt( &ok );
   if( ok && okval > 5 && okval < 1000 ) {
      selected = okval;
      emit( customScaleChange( okval ));
   } else {
      kdDebug(28000) << "ERR: To large, to smale, or whatever shitty !" << endl;
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
      kdDebug(28000) << "ERR: Invalid size selected !" << endl;
   }
   // debug( "SetValue: Selected Scale is %d", selected );
}



/* ############################################################################## */

EntryDialog::EntryDialog( QWidget *parent, QString caption, const QString text)
: KDialogBase( parent, "ENTRY_DIALOG", true, caption, Ok|Cancel, Ok, true )
{
   
   QVBox *vbox = makeVBoxMainWidget();
   (void) new  QLabel( text, vbox, "LABEL" );

   entry = new QLineEdit( vbox );

   entry->setFocus();
}

QString EntryDialog::getText ( void )
{
   if( entry )
      return( entry->text());

   return( "" );
}

EntryDialog::~EntryDialog()
{
}


#include "miscwidgets.moc"
