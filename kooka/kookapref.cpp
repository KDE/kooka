/***************************************************************************
                  kookapref.cpp  -  Kookas preferences dialog
                             -------------------
    begin                : Wed Jan 5 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "kookapref.h"

#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>

#include <qlayout.h>
#include <qlabel.h>

#include <devselector.h>

KookaPreferences::KookaPreferences()
    : KDialogBase(IconList, i18n("Preferences"),
                  Help|Default|Ok|Apply|Cancel, Ok)
{
    // this is the base class for your preferences dialog.  it is now
    // a Treelist dialog.. but there are a number of other
    // possibilities (including Tab, Swallow, and just Plain)
    setupStartupPage();
    
}


void KookaPreferences::setupStartupPage(void)
{
    KConfig *konf = KGlobal::config ();

    /* startup options */
    konf->setGroup( GROUP_STARTUP );
    
    QFrame *page = addPage( i18n("Startup"), i18n("Kooka Startup Preferences" ),
			    BarIcon("gear", KIcon::SizeMedium ) );
    QVBoxLayout *top = new QVBoxLayout( page, 0, spacingHint() );
    top->addWidget( new QLabel( i18n("Note that changing this options will affect Kooka's next start!"), page ));
    
    cbNetQuery = new QCheckBox( i18n("Query network for available scanners"),
				page,  "CB_NET_QUERY" );
    cbNetQuery->setChecked( ! konf->readBoolEntry( "QueryLocalOnly" ) );
    
    cbShowScannerSelection = new QCheckBox( i18n("Show the scanner selection box on next startup"),
					    page,  "CB_SHOW_SELECTION" );
    cbShowScannerSelection->setChecked( !konf->readBoolEntry( "SkipStartupAsk" ));
    
    top->addWidget( cbNetQuery );
    top->addWidget( cbShowScannerSelection );

    top->addStretch(10);
}

void KookaPreferences::slotOk( void )
{
  slotApply();
  accept();
    
}


void KookaPreferences::slotApply( void )
{
    KConfig *konf = KGlobal::config ();

    /* startup options */
    konf->setGroup( GROUP_STARTUP );
    
    konf->writeEntry( STARTUP_ONLY_LOCAL,  !cbNetQuery->isChecked() );
    if( cbShowScannerSelection->isChecked() )
    {
	konf->writeEntry( STARTUP_SKIP_ASK, false );
    }

}

void KookaPreferences::slotDefault( void )
{
    
}



#include "kookapref.moc"

