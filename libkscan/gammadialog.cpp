/***************************************************************************
                          gammadialog.cpp  -  description
                             -------------------
    begin                : Sat Aug 12 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlabel.h>
#include <qpushbutton.h>

#include <kscanslider.h>
#include <klocale.h>
#include <kdebug.h>

#include "gammadialog.h"

GammaDialog::GammaDialog( QWidget *parent ) :
   KDialogBase( parent,  "GammaDialog", true, i18n("Custom Gamma Tables"),
		 Ok|Cancel|Apply, Ok, true )
{
    gt = new KGammaTable();
    QWidget *page = new QWidget( this );

    CHECK_PTR( page );
    setMainWidget( page );

    /* This connect is for recalculating the table everytimes a new
     * Bright., Contrast or Gamma-Value is set */
    connect( gt, SIGNAL(tableChanged()), gt, SLOT(getTable()));

    gtDisp = new DispGamma( page );
    gtDisp->setValueRef( gt->getArrayPtr() );
    gtDisp->resize( 280, 280 );

    connect( gt, SIGNAL(tableChanged()), gtDisp, SLOT( repaint()));

    // setCaption( i18n( "Gamma Table" ));

    // Layout-Boxes
    QVBoxLayout *bigdad    = new QVBoxLayout( page, 10 );
    QHBoxLayout *lhMiddle  = new QHBoxLayout( 5 );
    QVBoxLayout *lvSliders = new QVBoxLayout( 10 );

    QLabel *l_top = new QLabel( i18n( "<B>Edit the custom gamma table</B><BR>This gamma table is passed to the scanner hardware." ), page );
    bigdad->addWidget( l_top, 1 );
    bigdad->addLayout( lhMiddle, 6 );

    lhMiddle->addLayout( lvSliders, 3);
    lhMiddle->addWidget( gtDisp, 2 );

    /* Slider Widgets for gamma, brightness, contrast */
    wBright   = new KScanSlider ( page, i18n("Brightness"), -50.0, 50.0 );
    CHECK_PTR(wBright);
    wBright->slSetSlider( 0 );
    connect( wBright, SIGNAL(valueChanged(int)), gt, SLOT(setBrightness(int)));

    wContrast = new KScanSlider ( page, i18n("Constrast") , -50.0, 50.0 );
    CHECK_PTR(wContrast);
    wContrast->slSetSlider( 0 );
    connect( wContrast, SIGNAL(valueChanged(int)), gt, SLOT(setContrast(int)));

    wGamma    = new KScanSlider ( page, i18n("Gamma"),  30.0, 300.0 );
    CHECK_PTR(wGamma);
    wGamma->slSetSlider(100);
    connect( wGamma, SIGNAL(valueChanged(int)), gt, SLOT(setGamma(int)));

    /* and add the Sliders */
    lvSliders->addWidget( wBright,   1 );
    lvSliders->addWidget( wContrast, 1 );
    lvSliders->addWidget( wGamma,    1 );

    // Finished and Activate !
    bigdad->activate();
    resize( 480, 300 );
}


void GammaDialog::setGt(KGammaTable& ngt)
{
   *gt = ngt;

   if( wBright ) wBright->slSetSlider( gt->getBrightness() );
   if( wContrast ) wContrast->slSetSlider( gt->getContrast() );
   if( wGamma ) wGamma->slSetSlider( gt->getGamma() );

}

void GammaDialog::slotApply()
{
   kdDebug(29000) << "Applying !" << endl;

   /* Do some calculation here */

   /* and call a signal */
   emit( gammaToApply( *this ));
}

GammaDialog::~GammaDialog()
{
#if 0
   delete lhMiddle;
   delete lvSliders;
#endif
}
#include "gammadialog.moc"
