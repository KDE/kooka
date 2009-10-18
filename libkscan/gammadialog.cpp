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

#include "gammadialog.h"
#include "gammadialog.moc"

#include <qlabel.h>
#include <qlayout.h>

#include <klocale.h>
#include <kdebug.h>
#include <kgammatable.h>

#include "kscancontrols.h"


GammaDialog::GammaDialog(QWidget *parent)
    : KDialog(parent)
{
    setObjectName("GammaDialog");

    setButtons(KDialog::Ok|KDialog::Cancel|KDialog::Apply);
    setCaption(i18n("Custom Gamma Tables"));
    setModal(true);
    showButtonSeparator(true);

    QWidget *page = new QWidget(this);
    Q_CHECK_PTR( page );
    setMainWidget( page );

    gt = new KGammaTable();
    /* This connect is for recalculating the table every time a new
     * Bright., Contrast or Gamma-Value is set */
    connect( gt, SIGNAL(tableChanged()), gt, SLOT(getTable()));

    gtDisp = new DispGamma( page );
    gtDisp->setValueRef( gt->getArrayPtr() );
    gtDisp->resize( 280, 280 );

    connect( gt, SIGNAL(tableChanged()), gtDisp, SLOT( repaint()));

    // setCaption( i18n( "Gamma Table" ));

    // Layout-Boxes
    QVBoxLayout *bigdad    = new QVBoxLayout(page);
    QHBoxLayout *lhMiddle  = new QHBoxLayout(page);
    QVBoxLayout *lvSliders = new QVBoxLayout(page);

    QLabel *l_top = new QLabel( i18n( "<B>Edit the custom gamma table</B><BR>This gamma table is passed to the scanner hardware." ), page );
    bigdad->addWidget( l_top, 1 );
    bigdad->addLayout( lhMiddle, 6 );

    lhMiddle->addLayout( lvSliders, 3);
    lhMiddle->addWidget( gtDisp, 2 );

    /* Slider Widgets for gamma, brightness, contrast */
    wBright   = new KScanSlider ( page, i18n("Brightness"), -50.0, 50.0 );
    Q_CHECK_PTR(wBright);
    wBright->slotSetSlider( 0 );
    connect( wBright, SIGNAL(valueChanged(int)), gt, SLOT(setBrightness(int)));

    wContrast = new KScanSlider ( page, i18n("Contrast") , -50.0, 50.0 );
    Q_CHECK_PTR(wContrast);
    wContrast->slotSetSlider( 0 );
    connect( wContrast, SIGNAL(valueChanged(int)), gt, SLOT(setContrast(int)));

    wGamma    = new KScanSlider ( page, i18n("Gamma"),  30.0, 300.0 );
    Q_CHECK_PTR(wGamma);
    wGamma->slotSetSlider(100);
    connect( wGamma, SIGNAL(valueChanged(int)), gt, SLOT(setGamma(int)));

    /* and add the Sliders */
    lvSliders->addWidget( wBright,   1 );
    lvSliders->addWidget( wContrast, 1 );
    lvSliders->addWidget( wGamma,    1 );

    connect(this,SIGNAL(okClicked()),SLOT(slotApply));
    connect(this,SIGNAL(applyClicked()),SLOT(slotApply));

    // Finished and Activate !
    bigdad->activate();
    resize( 480, 300 );
}


void GammaDialog::setGt(KGammaTable& ngt)
{
   *gt = ngt;

   if( wBright ) wBright->slotSetSlider( gt->getBrightness() );
   if( wContrast ) wContrast->slotSetSlider( gt->getContrast() );
   if( wGamma ) wGamma->slotSetSlider( gt->getGamma() );

}


void GammaDialog::slotApply()
{
   /* and call a signal */
   KGammaTable *myTable = getGt();
   emit gammaToApply(myTable);
}


GammaDialog::~GammaDialog()
{
}
