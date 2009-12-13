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

#include <qbuttongroup.h>
#include <qgroupbox.h>
#include <qlayout.h>
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

   QGroupBox * radios = new QGroupBox(i18n("Select Image Zoom"), this);
   setMainWidget(radios);

   QButtonGroup * radiosGroup = new QButtonGroup(radios);
   connect( radiosGroup, SIGNAL( buttonClicked( int )), this, SLOT( slotSetSelValue( int )));

   QVBoxLayout * radiosLayout = new QVBoxLayout(this);

      // left column: smaller Image
      QHBoxLayout * hbox = new QHBoxLayout();
      QVBoxLayout * vbox = new QVBoxLayout();

      QRadioButton *rb25 = new QRadioButton (i18n ("25 %"));
      if( curr_sel == 25 ){
         rb25->setChecked( true );
         one_is_selected = true;
      }
      vbox->addWidget(rb25);
      radiosGroup->addButton(rb25);

      QRadioButton *rb50 = new QRadioButton (i18n ("50 %"));
      if( curr_sel == 50 ){
         rb50->setChecked( true );
         one_is_selected = true;
      }
      vbox->addWidget(rb50);
      radiosGroup->addButton(rb50);

      QRadioButton *rb75 = new QRadioButton (i18n ("75 %"));
      if( curr_sel == 75 ) {
         rb75->setChecked( true );
         one_is_selected = true;
      }
      vbox->addWidget(rb75);
      radiosGroup->addButton(rb75);

      QRadioButton *rb100 = new QRadioButton (i18n ("100 %"));
      if( curr_sel == 100 ) {
         rb100->setChecked( true );
         one_is_selected = true;
      }
      vbox->addWidget(rb100);
      radiosGroup->addButton(rb100);

      hbox->addLayout(vbox);

      // right column: bigger image:
      vbox = new QVBoxLayout();

      QRadioButton *rb150 = new QRadioButton (i18n ("150 %"));
      if( curr_sel == 150 ) {
         rb150->setChecked( true );
         one_is_selected = true;
      }
      vbox->addWidget(rb150);
      radiosGroup->addButton(rb150);

      QRadioButton *rb200 = new QRadioButton (i18n ("200 %"));
      if( curr_sel == 200 ) {
         rb200->setChecked( true );
         one_is_selected = true;
      }
      vbox->addWidget(rb200);
      radiosGroup->addButton(rb200);

      QRadioButton *rb300 = new QRadioButton (i18n ("300 %"));
      if( curr_sel == 300 ) {
         rb300->setChecked( true );
         one_is_selected = true;
      }
      vbox->addWidget(rb300);
      radiosGroup->addButton(rb300);

      QRadioButton *rb400 = new QRadioButton (i18n ("400 %"));
      if( curr_sel == 400 ) {
         rb400->setChecked( true );
         one_is_selected = true;
      }
      vbox->addWidget(rb400);
      radiosGroup->addButton(rb400);

      hbox->addLayout(vbox);
      radiosLayout->addLayout(hbox);


      // Custom Scaler at the bottom:
      hbox = new QHBoxLayout();

         QRadioButton *rbCust = new QRadioButton (i18n ("Custom scale factor:"));
         if( ! one_is_selected )
            rbCust->setChecked( true );
         connect( rbCust, SIGNAL( toggled( bool )), this, SLOT(slotEnableAndFocus(bool)));

         hbox->addWidget(rbCust);
         radiosGroup->addButton(rbCust);

         leCust = new KLineEdit();
         QString sn;
         sn.setNum(curr_sel );
         leCust->setValidator( new KIntValidator( leCust ) );
         leCust->setText(sn );
         leCust->setEnabled( rbCust->isChecked());
         connect( leCust, SIGNAL( textChanged( const QString& )), this, SLOT( slotCustomChanged( const QString& )));
         hbox->addWidget(leCust);

      radiosLayout->addLayout(hbox);
      radiosLayout->addStretch();

   radios->setLayout(radiosLayout);
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
