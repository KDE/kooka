/***************************************************************************
                   imgscaledialog.cpp - small dialog to scale the image
                             -------------------                                         
    begin                : 04.04.2001
    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de

    $Id$ 
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
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


#include <klocale.h>
#include <kdebug.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

#include "imgscaledialog.h"

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
   
   // (void) new QLabel( , main, "Page");
   //
   // makeMainWidget();
   QButtonGroup *radios = new QButtonGroup ( 2, Qt::Horizontal, this );
   setMainWidget(radios);
   Q_CHECK_PTR(radios);
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
   QRadioButton *rbCust = new QRadioButton (i18n ("Custom Scale Factor:"),
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


int ImgScaleDialog::getSelected() const 
{
   return( selected );
}


/* ############################################################################## */


#include "imgscaledialog.moc"
