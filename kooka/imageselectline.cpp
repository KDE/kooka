/***************************************************************************
              imageselectline.cpp - select a background image.
                             -------------------
    begin                : Fri Dec 17 1999
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
 ***************************************************************************/
#include <qhbox.h>
#include <qvbox.h>
#include <qbutton.h>
#include <qpushbutton.h>
#include <qlabel.h>

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kurlcombobox.h>
#include <kfiledialog.h>
#include <kiconloader.h>

#include "imageselectline.h"

/* ############################################################################## */

/*
 * This widget just combines a label, a combobox holding a path and a select button
 * together in a row. The button opens a file selector box to pick a file.
 */

ImageSelectLine::ImageSelectLine( QWidget *parent, const QString& text )
   : QHBox( parent )
{
   setSpacing( 5 );
   (void) new QLabel( text, this );
   m_urlCombo       = new KURLComboBox( KURLComboBox::Files, this );
   m_buttFileSelect = new QPushButton( this );
   m_buttFileSelect->setPixmap( SmallIcon( "fileopen" ) );

   m_urlCombo->setMaxItems(5);

   connect( m_urlCombo, SIGNAL( urlActivated( const KURL& )),
	    this, SLOT( slUrlActivated( const KURL& )));

   connect( m_buttFileSelect, SIGNAL( clicked() ),
	    this, SLOT( slSelectFile()));
}

void ImageSelectLine::slSelectFile()
{
   KURL newUrl;
   newUrl = KFileDialog::getImageOpenURL();

   QStringList l = m_urlCombo->urls();

   if( ! newUrl.isEmpty())
   {
      l.prepend( newUrl.url() );
      m_urlCombo->setURLs( l );
      m_currUrl = newUrl;
   }
}

void ImageSelectLine::slUrlActivated( const KURL& url )
{
   kdDebug(28000) << "Activating url: " << url.url() << endl;
   m_currUrl = url;
}

KURL ImageSelectLine::selectedURL() const
{
   return m_currUrl;
}

void ImageSelectLine::setURL( const KURL& url )
{
   if( m_urlCombo ) m_urlCombo->setURL( url );
   m_currUrl = url;
}

void ImageSelectLine::setURLs( const QStringList& list )
{
   if( m_urlCombo ) m_urlCombo->setURLs( list );
}

#include "imageselectline.moc"
