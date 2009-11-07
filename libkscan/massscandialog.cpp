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

#include "massscandialog.h"
#include "massscandialog.moc"

#include <qlayout.h>
#include <qlabel.h>
#include <qprogressbar.h>
#include <qgroupbox.h>
#include <qframe.h>

#include <klocale.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>


MassScanDialog::MassScanDialog( QWidget *parent )
   : KDialog(parent)
{
    setObjectName("MassScanDialog");

    kDebug();

    setModal(true);
    setButtons(KDialog::User1|KDialog::User2|KDialog::Cancel);
    setDefaultButton(KDialog::User1);
    setCaption(i18n("ADF Scanning"));

    // Layout-Boxes
    QVBoxLayout *bigdad = new QVBoxLayout(this);
    bigdad->setSpacing(5);
    QHBoxLayout *l_but  = new QHBoxLayout(this);  // Buttons
    l_but->setSpacing(10);
 	
    /* Caption */
    QLabel *l1 = new QLabel(i18n("<qt><b>Mass Scanning</b>"), this);
    bigdad->addWidget( l1, 1);
 	
    /* Scan parameter information */
    QGroupBox *f1 = new QGroupBox( i18n("Scan Parameters"), this );
    //f1->setFrameStyle( QFrame::Box | QFrame::Sunken );
    //f1->setMargin(5);
    //f1->setLineWidth( 1 );

    //QVBoxLayout *l_main = new QVBoxLayout( f1, f1->frameWidth()+3, 3 );
    QVBoxLayout *l_main = new QVBoxLayout(f1);
    l_main->setSpacing(3);
    bigdad->addWidget( f1, 6 );
 	
    scanopts = i18n("Scanning <B>%s</B> at <B>%d</B> dpi");
    l_scanopts = new QLabel( scanopts, f1 );
    l_main->addWidget( l_scanopts );

    tofolder = i18n("Saving scans in folder <B>%s</B>");
    l_tofolder = new QLabel( tofolder, f1 );
    l_main->addWidget( l_tofolder );
 	
    /* Scan Progress information */
    QGroupBox *f2 = new QGroupBox( i18n("Scan Progress"));
    f2->setFlat(true);

    QVBoxLayout *l_pro = new QVBoxLayout();
    l_pro->setSpacing(3);

       QHBoxLayout *l_scanp = new QHBoxLayout();
          progress = i18n("Scanning page %1");
          l_progress = new QLabel( progress, f2 );
          l_scanp->addWidget( l_progress, 3 );
          l_scanp->addStretch( 1 );
       l_pro->addLayout( l_scanp, 5 );

       progressbar = new QProgressBar();
       progressbar->setRange(0,1000);
       l_pro->addWidget( progressbar, 3 );

    f2->setLayout(l_pro);
    bigdad->addWidget(f2, 6);

    /* Buttons to start scanning and close the Window */
    bigdad->addLayout( l_but );

    // User1 = Start Scan
    connect(this,SIGNAL(user1Clicked()),SLOT(slotStartScan()));
    // User2 = Finish
    connect(this,SIGNAL(user2Clicked()),SLOT(slotStopScan()));
    // Cancel
    connect(this,SIGNAL(cancelClicked()),SLOT(slotFinished()));

    bigdad->activate();
    show();
}


MassScanDialog::~MassScanDialog()
{
}


void MassScanDialog::slotStartScan()
{
    kDebug();
}

void MassScanDialog::slotStopScan()
{
    kDebug();
}

void MassScanDialog::slotFinished()
{
    accept();
}


void MassScanDialog::setPageProgress( int p )
{
    progressbar->setValue( p );
}
