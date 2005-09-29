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

#include <qlayout.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qslider.h>
#include <qlineedit.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>
#include "kscanslider.h"

KScanSlider::KScanSlider( QWidget *parent, const QString& text,
			  double min, double max, bool haveStdButt,
			  int stdValue )
   : QFrame( parent ),
     m_stdValue( stdValue ),
     m_stdButt(0)
{
    QHBoxLayout *hb = new QHBoxLayout( this );
    l1 = new QLabel( text, this, "AUTO_SLIDER_LABEL" );
    hb->addWidget( l1,20 );

    if( haveStdButt )
    {
       KIconLoader *loader = KGlobal::iconLoader();
       m_stdButt = new QPushButton( this );
       m_stdButt->setPixmap( loader->loadIcon( "undo",KIcon::Small ));

       /* connect the button click to setting the value */
       connect( m_stdButt, SIGNAL(clicked()),
		this, SLOT(slRevertValue()));

       QToolTip::add( m_stdButt,
		      i18n( "Revert value back to its standard value %1" ).arg( stdValue ));
       hb->addWidget( m_stdButt, 0 );
       hb->addSpacing( 4 );
    }

    slider = new QSlider( (int) min, (int)max, 1, (int)min, QSlider::Horizontal, this, "AUTO_SLIDER_" );
    slider->setTickmarks( QSlider::Below );
    slider->setTickInterval( int(QMAX( (max-min)/10, 1 )) );
    slider->setSteps( int(QMAX( (max-min)/20, 1) ), int(QMAX( (max-min)/10, 1) ) );
    slider->setMinimumWidth( 140 );
    /* set a buddy */
    l1->setBuddy( slider );

    /* create a spinbox for displaying the values */
    m_spin = new QSpinBox( (int) min, (int) max,
			   1, // step
			   this );


    /* make spin box changes change the slider */
    connect( m_spin, SIGNAL(valueChanged(int)), this, SLOT(slSliderChange(int)));

    /* Handle internal number display */
    // connect(slider, SIGNAL(valueChanged(int)), numdisp, SLOT( setNum(int) ));
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT( slSliderChange(int) ));

    /* set Value 0 to the widget */
    slider->setValue( (int) min -1 );

    /* Add to layout widget and activate */
    hb->addWidget( slider, 36 );
    hb->addSpacing( 4 );
    hb->addWidget( m_spin, 0 );

    hb->activate();

}

void KScanSlider::setEnabled( bool b )
{
    if( slider )
	slider->setEnabled( b );
    if( l1 )
	l1->setEnabled( b );
    if( m_spin )
	m_spin->setEnabled( b );
    if( m_stdButt )
       m_stdButt->setEnabled( b );
}

void KScanSlider::slSetSlider( int value )
{
    /* Important to check value to avoid recursive signals ;) */
    // debug( "Slider val: %d -> %d", value, slider_val );
    kdDebug(29000) << "Setting Slider with " << value << endl;

    if( value == slider->value() )
    {
      kdDebug(29000) << "Returning because slider value is already == " << value << endl;
      return;
    }
    slider->setValue( value );
    slSliderChange( value );

}

void KScanSlider::slSliderChange( int v )
{
   // kdDebug(29000) << "Got slider val: " << v << endl;
    // slider_val = v;
    int spin = m_spin->value();
    if( v != spin )
       m_spin->setValue(v);
    int slid = slider->value();
    if( v != slid )
       slider->setValue(v);

    emit( valueChanged( v ));
}

void KScanSlider::slRevertValue()
{
   if( m_stdButt )
   {
      /* Only if stdButt is non-zero, the default value is valid */
      slSetSlider( m_stdValue );
   }
}


KScanSlider::~KScanSlider()
{
}

/* ====================================================================== */

KScanEntry::KScanEntry( QWidget *parent, const QString& text )
 : QFrame( parent )
{
    QHBoxLayout *hb = new QHBoxLayout( this );

    QLabel *l1 = new QLabel( text, this, "AUTO_ENTRYFIELD" );
    hb->addWidget( l1,1 );

    entry = new QLineEdit( this, "AUTO_ENTRYFIELD_E" );
    l1->setBuddy( entry );
    connect( entry, SIGNAL( textChanged(const QString& )),
	     this, SLOT( slEntryChange(const QString&)));
    connect( entry, SIGNAL( returnPressed()),
	     this,  SLOT( slReturnPressed()));

    hb->addWidget( entry,3 );
    hb->activate();
}

QString  KScanEntry::text( void ) const
{
   QString str = QString::null;
   // kdDebug(29000) << "entry is "<< entry << endl;
   if(entry)
   {
       str = entry->text();
      if( ! str.isNull() && ! str.isEmpty())
      {
	 kdDebug(29000) << "KScanEntry returns <" << str << ">" << endl;
      }
      else
      {
	 kdDebug(29000) << "KScanEntry:  nothing entered !" << endl;
      }
   }
   else
   {
      kdDebug(29000) << "KScanEntry ERR: member var entry not defined!" << endl;
   }
   return ( str );
}

void KScanEntry::slSetEntry( const QString& t )
{
    if( t == entry->text() )
	return;
    /* Important to check value to avoid recursive signals ;) */

    entry->setText( t );
}

void KScanEntry::slEntryChange( const QString& t )
{
    emit valueChanged( QCString( t.latin1() ) );
}

void KScanEntry::slReturnPressed( void )
{
   QString t = text();
   emit returnPressed( QCString( t.latin1()));
}



KScanCombo::KScanCombo( QWidget *parent, const QString& text,
			const QStrList& list )
    : QHBox( parent ),
      combo(0)
{
    createCombo( text );
    if( combo )
        combo->insertStrList( list);
    combolist = list;
}

KScanCombo::KScanCombo( QWidget *parent, const QString& text,
			const QStringList& list )
    : QHBox( parent ),
      combo(0)
{
    createCombo( text );
    if( combo )
        combo->insertStringList( list );

    for ( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
        combolist.append( (*it).local8Bit() );
    }
}


void KScanCombo::createCombo( const QString& text )
{
    setSpacing( 12 );
    setMargin( 2 );


    (void) new QLabel( text, this, "AUTO_COMBOLABEL" );

    combo = new QComboBox( this, "AUTO_COMBO" );

    connect( combo, SIGNAL(activated( const QString &)), this,
             SLOT( slComboChange( const QString &)));
    connect( combo, SIGNAL(activated( int )),
	     this,  SLOT(slFireActivated(int)));

}


void KScanCombo::slSetEntry( const QString &t )
{
    if( t.isNull() ) 	return;
    int i = combolist.find( t.local8Bit() );

    /* Important to check value to avoid recursive signals ;) */
    if( i == combo->currentItem() )
	return;

    if( i > -1 )
	combo->setCurrentItem( i );
    else
	kdDebug(29000) << "Combo item not in list !" << endl;
}

void KScanCombo::slComboChange( const QString &t )
{
    emit valueChanged( QCString( t.latin1() ) );
    kdDebug(29000) << "Combo: valueChanged emitted!" << endl;
}

void KScanCombo::slSetIcon( const QPixmap& pix, const QString& str)
{
   for( int i=0; i < combo->count(); i++ )
   {
      if( combo->text(i) == str )
      {
	 combo->changeItem( pix, str, i );
	 break;
      }
   }
}

QString KScanCombo::currentText( void ) const
{
   return( combo->currentText() );
}


QString KScanCombo::text( int i ) const
{
   return( combo->text(i) );
}

void    KScanCombo::setCurrentItem( int i )
{
   combo->setCurrentItem( i );
}

int KScanCombo::count( void ) const
{
   return( combo->count() );
}

void KScanCombo::slFireActivated( int i )
{
   emit( activated( i ));
}

#include "kscanslider.moc"
