/***************************************************************************
                     kocrbase.cpp - base dialog for ocr
                             -------------------
    begin                : Fri Now 10 2000
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

#include <qlayout.h>
#include <qlabel.h>
#include <qfileinfo.h>
#include <qtooltip.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kanimwidget.h>
#include <kseparator.h>
#include <kmessagebox.h>

#include "resource.h"
#include "kocrbase.h"
#include "ksaneocr.h"
#include "kookaimage.h"

#include <kscanslider.h>


KOCRBase::KOCRBase( QWidget *parent, KDialogBase::DialogType face )
   :KDialogBase( face, i18n("Optical Character Recognition"),
		 Cancel|User1, User1, parent,0, false, true,
		 KGuiItem( i18n("Start OCR" ), "launch",
			   i18n("Start the Optical Character Recognition process" )) ),
    m_animation(0)
{
   kdDebug(28000) << "OCR Base Dialog!" << endl;
   // Layout-Boxes

   /* Connect signals which disable the fields and store the configuration */
   connect( this, SIGNAL( user1Clicked()), this, SLOT( writeConfig()));
   connect( this, SIGNAL( user1Clicked()), this, SLOT( startOCR() ));
}


KAnimWidget* KOCRBase::getAnimation(QWidget *parent)
{
   if( ! m_animation )
   {
      m_animation = new KAnimWidget( QString("kde"), 48, parent, "ANIMATION" );
   }
   return( m_animation );
}

void KOCRBase::stopAnimation()
{
   if( m_animation )
      m_animation->stop();
}

void KOCRBase::startAnimation()
{
   if( m_animation )
      m_animation->start();
}

KOCRBase::~KOCRBase()
{

}

void KOCRBase::enableFields(bool b)
{
    enableButton( User1, b );
}

void KOCRBase::introduceImage( KookaImage*)
{

}

void KOCRBase::writeConfig()
{

}


void KOCRBase::startOCR()
{

}

/* The End ;) */
#include "kocrbase.moc"
