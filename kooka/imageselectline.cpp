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
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/
#include <q3hbox.h>
#include <q3vbox.h>
#include <q3button.h>
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
   : Q3HBox( parent )
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
