/* This file is part of the KDE Project
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>

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

#include "imgscaledialog.h"
#include "imgscaledialog.moc"

#include <q3buttongroup.h>
#include <qradiobutton.h>

#include <klocale.h>
#include <kdebug.h>
#include <knumvalidator.h>
#include <klineedit.h>

/* ############################################################################## */


ImgScaleDialog::ImgScaleDialog( QWidget *parent, int curr_sel)
   : KDialog(parent)
{
    setObjectName("ImgScaleDialog");

    setButtons(KDialog::Ok|KDialog::Cancel);
    setCaption(i18n("Zoom"));
    setModal(true);
    showButtonSeparator(false);

   selected = curr_sel;
   int        one_is_selected = false;

   Q3ButtonGroup *radios = new Q3ButtonGroup ( 2, Qt::Horizontal, this );
   setMainWidget(radios);
   radios->setTitle( i18n("Select Image Zoom") );

   connect( radios, SIGNAL( clicked( int )),
	    this, SLOT( slotSetSelValue( int )));

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
   connect( rbCust, SIGNAL( toggled( bool )),
	    this, SLOT(slotEnableAndFocus(bool)));
   if( ! one_is_selected )
      rbCust->setChecked( true );

   leCust = new KLineEdit( radios );
   QString sn;
   sn.setNum(curr_sel );
   leCust->setValidator( new KIntValidator( leCust ) );
   leCust->setText(sn );
   connect( leCust, SIGNAL( textChanged( const QString& )),
	    this, SLOT( slotCustomChanged( const QString& )));
   leCust->setEnabled( rbCust->isChecked());


}


void ImgScaleDialog::slotCustomChanged( const QString& s )
{
   bool ok;
   int  okval = s.toInt( &ok );
   if( ok && okval > 5 && okval < 1000 ) {
      selected = okval;
      emit( customScaleChange( okval ));
   } else {
      kDebug() << "Error: Out of range!";
   }
}


// This slot is called, when the user changes the Scale-Selection
// in the button group. The value val is the index of the active
// button which is translated to the Scale-Size in percent.
// If custom size is selected, the ScaleSize is read from the
// QLineedit.
//
void ImgScaleDialog::slotSetSelValue( int val )
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
      kDebug() << "Error: Invalid size selected!";
   }
}


int ImgScaleDialog::getSelected() const
{
   return( selected );
}


void ImgScaleDialog::slotEnableAndFocus( bool b )
{
    leCust->setEnabled( b ); leCust->setFocus();
}
