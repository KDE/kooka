/***************************************************************************
                          kscanslider.cpp  -  description
                             -------------------
    begin                : Wed Jan 5 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : Klaas.Freitag@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlayout.h>
#include "kscanslider.h"

KScanSlider::KScanSlider( QWidget *parent, const char *text, double min, double max )
 : QFrame( parent )
{
    QHBoxLayout *hb = new QHBoxLayout( this );
    l1 = new QLabel( text, this, "AUTO_SLIDER_LABEL" + QString(text) );
    hb->addWidget( l1,20 );
    numdisp = new QLabel( "MMM", this, "AUTO_SLIDER_NUMDISP" + QString(text) );
    numdisp->setAlignment( AlignRight );
    numdisp->setMinimumHeight( numdisp->sizeHint().height());
    numdisp->setMaximumHeight( numdisp->sizeHint().height());
    hb->addWidget( numdisp, 8 );
    hb->addStretch( 1);
    
    slider = new QSlider( min, max, 1, min, QSlider::Horizontal, this, "AUTO_SLIDER_" );
    slider->setTickmarks( QSlider::Below );
    slider->setTickInterval( (max-min) / 10 );
    slider->setSteps( (max-min)/20, (max-min)/10 );
    slider->setMinimumHeight( slider->sizeHint().height());
    slider->setMaximumHeight( slider->sizeHint().height());

    /* Handle internal number display */
    // connect(slider, SIGNAL(valueChanged(int)), numdisp, SLOT( setNum(int) ));
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT( slSliderChange(int) ));
		
    /* set Value 0 to the widget */
    slider_val = (int)min - 1;

    /* Add to layout widget and activate */
    hb->addWidget( slider, 36 );
    hb->activate();

}

void KScanSlider::setEnabled( bool b )
{
    if( slider )
	slider->setEnabled( b );
    if( l1 )
	l1->setEnabled( b );
    if( numdisp )
	numdisp->setEnabled( b );
}

void KScanSlider::slSetSlider( int value )
{
    /* Important to check value to avoid recursive signals ;) */
    // debug( "Slider val: %d -> %d", value, slider_val );
    debug("Setting Slider with %d", value );

    if( value == slider_val )
    {
      debug( "Returning because slider value is already == %d", value );
      return;
    }
    slider->setValue( value );
    slSliderChange( value );

}

void KScanSlider::slSliderChange( int v )
{
    debug( "Got slider val: %d",v );
    slider_val = v;
    numdisp->setNum(v);
    emit( valueChanged( v ));
}

KScanSlider::~KScanSlider()
{
}

KScanEntry::KScanEntry( QWidget *parent, const char *text )
 : QFrame( parent )
{
    QHBoxLayout *hb = new QHBoxLayout( this );
    
    QLabel *l1 = new QLabel( text, this, "AUTO_ENTRYFIELD" );
    hb->addWidget( l1,1 );
 	
    entry = new QLineEdit( this, "AUTO_ENTRYFIELD_E" );
    connect( entry, SIGNAL( textChanged(const QString& )), this,
		    SLOT( slEntryChange(const QString&)));
 			
    hb->addWidget( entry,3 );
    entry->setMinimumHeight( entry->sizeHint().height() );
    hb->activate();
}

void KScanEntry::slSetEntry( const char *t )
{
    if( ! t || QString(t) == entry->text()) 	return;
    /* Important to check value to avoid recursive signals ;) */
		
    entry->setText( t );
}

void KScanEntry::slEntryChange( const QString& t )
{
    emit( valueChanged( t ));
}



KScanCombo::KScanCombo( QWidget *parent, const QString text, const QStrList& list )
   : QHBox( parent )
{
   setSpacing( 12 );
   setMargin( 2 );

   combolist = list;
 
   (void) new QLabel( text, this, "AUTO_COMBOLABEL" );
 	
    combo = new QComboBox( this, "AUTO_COMBO" );
    combo->insertStrList( &combolist);

    connect( combo, SIGNAL(activated( const QString &)), this,
		    SLOT( slComboChange( const QString &)));
    connect( combo, SIGNAL(activated( int )),
	     this,  SLOT(slFireActivated(int)));
}

void KScanCombo::slSetEntry( const QString &t )
{
    if( t.isNull() ) 	return;
    int i = combolist.find( t );

    /* Important to check value to avoid recursive signals ;) */
    if( i == combo->currentItem() )
	return;
		
    if( i > -1 )
	combo->setCurrentItem( i );
    else
	debug( "Combo item not in list !");
}

void KScanCombo::slComboChange( const QString &t )
{
    emit( valueChanged( t ));
    debug( "Combo: valueChanged emitted!" );
}

void KScanCombo::slSetIcon( const QPixmap& pix, const QString str)
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

#include "kscanslider.h"
#include "kscanslider.moc"
