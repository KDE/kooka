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

#include "kscancontrols.h"
#include "kscancontrols.moc"

#include <qlayout.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qslider.h>
#include <qlineedit.h>

#include <klocale.h>
#include <kdebug.h>
#include <kurlrequester.h>
#include <kimageio.h>



// TODO: a better way to implement these would be as a subclass of an abstract base
// class.  May eliminate some of the switch'es in kscanoption.cpp!


KScanSlider::KScanSlider( QWidget *parent, const QString& text,
			  double min, double max, bool haveStdButt,
			  int stdValue )
   : QFrame( parent ),
     m_stdValue( stdValue ),
     m_stdButt(0)
{
    QHBoxLayout *hb = new QHBoxLayout( this );

    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(((int) min),((int) max));
    slider->setTickPosition(QSlider::TicksBelow );
    slider->setTickInterval(qMax(((int)((max-min)/10)),1));
    slider->setSingleStep(qMax(((int)((max-min)/20)),1));
    slider->setPageStep(qMax(((int)((max-min)/10)),1));
    slider->setMinimumWidth( 140 );

    /* create a spinbox for displaying the values */
    m_spin = new QSpinBox(this);
    m_spin->setRange((int) min, (int) max);
    m_spin->setSingleStep(1);

    /* make spin box changes change the slider */
    connect( m_spin, SIGNAL(valueChanged(int)), this, SLOT(slotSliderChange(int)));

    /* Handle internal number display */
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT( slotSliderChange(int) ));

    /* set Value 0 to the widget */
    slider->setValue( (int) min -1 );

    /* Add to layout widget and activate */
    hb->addWidget( slider, 36 );
    hb->addSpacing( 4 );
    hb->addWidget( m_spin, 0 );

    if( haveStdButt )
    {
       m_stdButt = new QPushButton(this);
       m_stdButt->setIcon(KIcon("edit-undo"));

       /* connect the button click to setting the value */
       connect( m_stdButt, SIGNAL(clicked()),
		this, SLOT(slotRevertValue()));

       m_stdButt->setToolTip(i18n("Reset this setting to its standard value, %1",stdValue ));
       hb->addSpacing( 4 );
       hb->addWidget( m_stdButt, 0 );
    }

    hb->activate();
}

void KScanSlider::setEnabled( bool b )
{
    if( slider )
	slider->setEnabled( b );
    if( m_spin )
	m_spin->setEnabled( b );
    if( m_stdButt )
       m_stdButt->setEnabled( b );
}

void KScanSlider::slotSetSlider( int value )
{
    kDebug() << "setting slider to" << value;
    /* Important to check value to avoid recursive signals ;) */
    if (value == slider->value()) return;

    slider->setValue( value );
    slotSliderChange( value );
}

void KScanSlider::slotSliderChange( int v )
{
    // slider_val = v;
    int spin = m_spin->value();
    if( v != spin )
       m_spin->setValue(v);
    int slid = slider->value();
    if( v != slid )
       slider->setValue(v);

    emit( valueChanged( v ));
}

void KScanSlider::slotRevertValue()
{
   if( m_stdButt )
   {
      /* Only if stdButt is non-zero, the default value is valid */
      slotSetSlider( m_stdValue );
   }
}


int KScanSlider::value( ) const
{
    return( slider->value());
}



KScanSlider::~KScanSlider()
{
}

/* ====================================================================== */

KScanEntry::KScanEntry( QWidget *parent, const QString& text )
 : KHBox( parent )
{
    entry = new QLineEdit( this);
    connect( entry, SIGNAL( textChanged(const QString& )),
	     this, SLOT( slotEntryChange(const QString&)));
    connect( entry, SIGNAL( returnPressed()),
	     this,  SLOT( slotReturnPressed()));
}

QString  KScanEntry::text( void ) const
{
   QString str = QString::null;
   if(entry) str = entry->text();
   return ( str );
}

void KScanEntry::slotSetEntry( const QString& t )
{
    if( t == entry->text() )
	return;
    /* Important to check value to avoid recursive signals ;) */

    entry->setText( t );
}


void KScanEntry::slotEntryChange(const QString &t)
{
    emit valueChanged(t.toLatin1());
}


void KScanEntry::slotReturnPressed()
{
   QString t = text();
   emit returnPressed(t.toLatin1());
}

void KScanEntry::setEnabled( bool b )
{
    if( entry) entry->setEnabled( b );
}






KScanCombo::KScanCombo(QWidget *parent, const QString &text,
                       const QList<QByteArray> &list)
    : KHBox(parent),
      combo(NULL)
{
    createCombo( text );
    if (combo!=NULL)
    {
	for ( QList<QByteArray>::const_iterator it = list.constBegin();
              it != list.constEnd(); ++it )
	{
	    combo->addItem(i18n(*it));
	}
    }

    combolist = list;
    setFocusProxy(combo);
}


KScanCombo::KScanCombo(QWidget *parent, const QString &text,
                       const QStringList &list)
    : KHBox(parent),
      combo(NULL)
{
    createCombo( text );

    for (QStringList::ConstIterator it = list.constBegin();
         it != list.constEnd(); ++it )
    {
	if (combo!=NULL) combo->addItem(i18n( (*it).toUtf8()));
        combolist.append((*it).toLocal8Bit());
    }

    setFocusProxy(combo);
}


void KScanCombo::createCombo( const QString& text )
{
    combo = new QComboBox(this);

    connect( combo, SIGNAL(activated( const QString &)), this,
             SLOT(slotComboChange( const QString &)));
    connect( combo, SIGNAL(activated( int )),
	     this,  SLOT(slotFireActivated(int)));
}


void KScanCombo::slotSetEntry( const QString &t )
{
    if( t.isNull() ) 	return;
    int i = combolist.indexOf( t.toLocal8Bit() );

    /* Important to check value to avoid recursive signals ;) */
    if( i == combo->currentIndex() )
	return;

    if( i > -1 )
	combo->setCurrentIndex( i );
    else
	kDebug() << "Combo item not in list!";
}

void KScanCombo::slotComboChange(const QString &t)
{
    emit valueChanged(t.toLatin1());
}


void KScanCombo::slotSetIcon(const QPixmap &pix, const QString &str)
{
    int i = combo->findText(str);
    if (i!=-1) combo->setItemIcon(i, pix);
}


QString KScanCombo::currentText() const
{
    return (combo->currentText());
}


QString KScanCombo::text( int i ) const
{
   return( combo->itemText(i) );
}

void    KScanCombo::setCurrentItem( int i )
{
   combo->setCurrentIndex( i );
}

int KScanCombo::count() const
{
    return (combo->count());
}

void KScanCombo::slotFireActivated( int i )
{
   emit( activated( i ));
}


void KScanCombo::setEnabled( bool b)
{
    if(combo) combo->setEnabled( b );
}













/* ====================================================================== */

KScanFileRequester::KScanFileRequester( QWidget *parent, const QString& text )
 : KHBox( parent )
{
    entry = new KUrlRequester( this );

    QString fileSelector = "*.pnm *.PNM *.pbm *.PBM *.pgm *.PGM *.ppm *.PPM|PNM Image Files (*.pnm,*.pbm,*.pgm,*.ppm)\n";
    fileSelector += KImageIO::pattern()+"\n";
    fileSelector += i18n("*|All Files\n");
    entry->setFilter(fileSelector);

    connect( entry, SIGNAL( textChanged(const QString& )),
	     SLOT( slotEntryChange(const QString&)));
    connect( entry, SIGNAL( returnPressed()),
	     SLOT( slotReturnPressed()));
}

QString  KScanFileRequester::text( void ) const
{
   QString str = QString::null;
   if(entry) str = entry->url().url();
   return ( str );
}

void KScanFileRequester::slotSetEntry( const QString& t )
{
    if( t == entry->url().url() )
	return;
    /* Important to check value to avoid recursive signals ;) */

    entry->setUrl( t );
}

void KScanFileRequester::slotEntryChange( const QString& t )
{
    emit valueChanged( t.toLatin1()  );
}

void KScanFileRequester::slotReturnPressed( void )
{
   QString t = text();
   emit returnPressed(t.toLatin1());
}

void KScanFileRequester::setEnabled( bool b )
{ if( entry) entry->setEnabled( b ); }
