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

#include "imageselectline.h"
#include "imageselectline.moc"

#include <qpushbutton.h>
#include <qlabel.h>

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kurlcombobox.h>
#include <kfiledialog.h>
#include <kiconloader.h>


/* ############################################################################## */

/*
 * This widget just combines a label, a combobox holding a path and a select button
 * together in a row. The button opens a file selector box to pick a file.
 */

ImageSelectLine::ImageSelectLine(QWidget *parent, const QString &label)
    : KHBox(parent)
{
   setSpacing( 5 );

   (void) new QLabel(label, this );

   m_urlCombo       = new KUrlComboBox( KUrlComboBox::Files, this );
   m_urlCombo->setMaxItems(5);
   connect( m_urlCombo, SIGNAL( urlActivated( const KUrl& )),
	    SLOT( slotUrlActivated( const KUrl& )));

   m_buttFileSelect = new QPushButton(this);
   m_buttFileSelect->setIcon(KIcon("fileopen"));
   connect( m_buttFileSelect, SIGNAL( clicked() ),
	    SLOT( slotSelectFile()));
}


void ImageSelectLine::slotSelectFile()
{
   KUrl newUrl;
   newUrl = KFileDialog::getImageOpenUrl();

   QStringList l = m_urlCombo->urls();

   if( ! newUrl.isEmpty())
   {
      l.prepend( newUrl.url() );
      m_urlCombo->setUrls( l );
      m_currUrl = newUrl;
   }
}


void ImageSelectLine::slotUrlActivated(const KUrl &url)
{
    kDebug() << url.prettyUrl();
    m_currUrl = url;
}


KUrl ImageSelectLine::selectedURL() const
{
    return (m_currUrl);
}


void ImageSelectLine::setURL( const KUrl& url )
{
   if( m_urlCombo ) m_urlCombo->setUrl( url );
   m_currUrl = url;
}


void ImageSelectLine::setURLs( const QStringList& list )
{
   if( m_urlCombo ) m_urlCombo->setUrls( list );
}
