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
#include <qlabel.h>
#include <q3groupbox.h>
#include <q3frame.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <klocale.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <KStandardGuiItem>
#include <kguiitem.h>
#include "massscandialog.h"

MassScanDialog::MassScanDialog( QWidget *parent )
   :QDialog( parent )
{
   setObjectName( QLatin1String( "MASS_SCAN" ) );
   setModal( true );
   setWindowTitle( i18n( "ADF Scanning" ) );
   kDebug(29000) << "Starting MassScanDialog!";
   // Layout-Boxes
   QVBoxLayout *bigdad = new QVBoxLayout( this );
   bigdad->setSpacing( 5 );
   // QHBoxLayout *hl1= new QHBoxLayout( );      // Caption
   QHBoxLayout *l_but  = new QHBoxLayout();  // Buttons
   l_but->setSpacing( 10 );
 	
 	/* Caption */
 	QLabel *l1 = new QLabel( i18n( "<B>Mass Scanning</B>" ), this);
   bigdad->addWidget( l1, 1);
 	
 	/* Scan parameter information */
 	Q3GroupBox *f1 = new Q3GroupBox( i18n("Scan Parameter"), this );
 	//f1->setFrameStyle( QFrame::Box | QFrame::Sunken );
 	//f1->setMargin(5);
 	//f1->setLineWidth( 1 );
   QVBoxLayout *l_main = new QVBoxLayout( f1 );
   l_main->setMargin( /*f1->frameWidth()+*/3 );
   l_main->setSpacing( 3 );
 	bigdad->addWidget( f1, 6 );
 	
   scanopts = i18n("Scanning <B>%s</B> with <B>%d</B> dpi");
 	l_scanopts = new QLabel( scanopts, f1 );
 	l_main->addWidget( l_scanopts );

   tofolder = i18n("Storing new images in folder <B>%s</B>");
 	l_tofolder = new QLabel( tofolder, f1 );
 	l_main->addWidget( l_tofolder );
 	
 	/* Scan Progress information */
 	Q3GroupBox *f2 = new Q3GroupBox( i18n("Scan Progress"), this );
 	//f2->setFrameStyle( QFrame::Box | QFrame::Sunken );
 	//f2->setMargin(15);
 	//f2->setLineWidth( 1 );
   QVBoxLayout *l_pro = new QVBoxLayout( f2 );
   l_pro->setMargin( /*f2->frameWidth()+*/3 );
   l_pro->setSpacing( 3 );
 	bigdad->addWidget( f2, 6 );

 	QHBoxLayout *l_scanp = new QHBoxLayout( );
 	l_pro->addLayout( l_scanp, 5 );
#ifdef __GNUC__	
   #warning i18n: Missing an argument to call below
#endif	
   progress = i18n("Scanning page %1");
   l_progress = new QLabel( progress, f2 );
   l_scanp->addWidget( l_progress, 3 );
 	l_scanp->addStretch( 1 );
   QPushButton *pb_cancel_scan = new QPushButton( i18n("Cancel Scan"), f2);
   l_scanp->addWidget( pb_cancel_scan,3 );

   progressbar = new QProgressBar( f2 );
   progressbar->setMaximum(1000);
   l_pro->addWidget( progressbar, 3 );

 	/* Buttons to start scanning and close the Window */
  	bigdad->addLayout( l_but );

   QPushButton *b_start = new QPushButton( i18n("Start Scan"), this);
   connect( b_start, SIGNAL(clicked()), this, SLOT( slStartScan()) );

   QPushButton *b_cancel = new QPushButton( i18n("Stop"), this );
   connect( b_cancel, SIGNAL(clicked()), this, SLOT(slStopScan()) );

   QPushButton *b_finish = new KPushButton( KStandardGuiItem::close(), this);
   connect( b_finish, SIGNAL(clicked()), this, SLOT(slFinished()) );

   l_but->addWidget( b_start );
   l_but->addWidget( b_cancel );
   l_but->addWidget( b_finish );

   bigdad->activate();
   show();
}

MassScanDialog::~MassScanDialog()
{

}

void MassScanDialog::slStartScan( void )
{

}

void MassScanDialog::slStopScan( void )
{

}

void MassScanDialog::slFinished( void )
{
    delete this;
}

#include "massscandialog.moc"
