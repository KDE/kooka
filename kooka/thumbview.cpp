/***************************************************************************
               thumbview.cpp  - Class to display thumbnailed images
                             -------------------                                         
    begin                : Tue Apr 18 2002
    copyright            : (C) 2002 by Klaas Freitag                         
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
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <qpixmap.h>
#include <qpainter.h>

#include <kio/previewjob.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kfileiconview.h>
#include <kfiletreeviewitem.h>
#include <kimageeffect.h>

#include "thumbview.h"
#include "thumbviewitem.h"

ThumbView::ThumbView( QWidget *parent, const char *name )
   : KIconView( parent, name ),
     m_pixWidth(100),
     m_pixHeight(120),
     m_thumbMargin(5)
{
   m_marginColor1 = colorGroup().base();
   m_marginColor2 = colorGroup().foreground();
   QImage ires = KImageEffect::unbalancedGradient( QSize( 2*m_thumbMargin+m_pixWidth,
							  2*m_thumbMargin+m_pixHeight ),
					 m_marginColor1, m_marginColor2, KImageEffect::DiagonalGradient );
   m_basePix.convertFromImage( ires );

   setItemsMovable( false );

   connect( this, SIGNAL( executed( QIconViewItem* )),
	    this, SLOT( slDoubleClicked( QIconViewItem* )));
}

ThumbView::~ThumbView()
{
   
}


void ThumbView::slDoubleClicked( QIconViewItem *qIt )
{
   ThumbViewItem *it = static_cast<ThumbViewItem*>( qIt );

   if( it )
   {
      const KURL url = it->itemUrl();
      
      emit( selectFromThumbnail( url ));
   }
}

void ThumbView::slImageChanged( KFileItem *kfit )
{
   if( ! kfit ) return;
   
   if( deleteImage( kfit ))
   {
      /* Trigger a new reading */
      KFileItemList li;
      li.append( kfit );
      slNewFileItems( li );
   }
}

bool ThumbView::deleteImage( KFileItem *kfit )
{
   if( ! kfit ) return false;

   kdDebug(28000) << "Deleting image from thumbview!" << endl;
   
   KURL searchUrl = kfit->url();
   bool haveItem = false;

   /* iterate over all icon items and compare urls.
    * TODO: Check the parent url to avoid iteration over all */
   for ( QIconViewItem *item = firstItem(); item && !haveItem; item = item->nextItem() )
   {
      if( searchUrl == static_cast<ThumbViewItem*>(item)->itemUrl() )
      {
	 takeItem( item );
	 haveItem = true;
      }
   }
   return( haveItem );
}

void ThumbView::slImageDeleted( KFileItem *kfit )
{
   deleteImage( kfit );
}


void ThumbView::slNewFileItems( const KFileItemList& items )
{
   kdDebug(28000) << "Creating thumbnails for fileItemList" << endl;

   
   KFileItemList startJobOn;
   
   KFileItemListIterator it( items );
   KFileItem *item = 0;
   for ( ; (item = it.current()); ++it )
   {
      QString filename = item->url().prettyURL();
      if( item->isDir() )
      {
	 /* create a dir pixmap */
      }
      else
      {
	 QPixmap p(m_basePix) ;
	 /* Create a new empty preview pixmap and store the pointer to it */
	 ThumbViewItem *newIconViewIt = new ThumbViewItem( this,
							   item->url().filename(),
							   createPixmap(p),
							   item );

	 newIconViewIt->setItemUrl( item->url() );
	 
	 /* tell the file item about the iconView-representation */
	 item->setExtraData( this, newIconViewIt );
	 
	 startJobOn.append( item );
      }
   }
	  
   if( startJobOn.count() > 0 )
   {
      /* start a preview-job */
      KIO::PreviewJob *job;
      job = KIO::filePreview(startJobOn, m_pixWidth, m_pixHeight );

      if( job )
      {
	 connect( job, SIGNAL( result( KIO::Job * )),
		  this, SLOT( slPreviewResult( KIO::Job * )));
	 connect( job, SIGNAL( gotPreview( const KFileItem*, const QPixmap& )),
		  SLOT( slGotPreview( const KFileItem*, const QPixmap& ) ));
        // connect( job, SIGNAL( failed( const KFileItem* )),
        //          this, SLOT( slotFailed( const KFileItem* ) ));

      }
   }
}


      
void ThumbView::slGotPreview( const KFileItem* newFileItem, const QPixmap& newPix )
{
   if( ! newFileItem ) return;
   KFileIconViewItem *item = static_cast<KFileIconViewItem*>(const_cast<void*>(newFileItem->extraData( this )));

   if( ! item ) return;

   const QPixmap px = createPixmap(newPix);
   item->setPixmap( createPixmap(newPix) );
   
}

void ThumbView::slPreviewResult( KIO::Job * )
{
   
}


QPixmap ThumbView::createPixmap( const QPixmap& preview ) const
{
   QImage ires = KImageEffect::unbalancedGradient( QSize( 2*m_thumbMargin+ preview.width(),
							  2*m_thumbMargin+ preview.height()),
						   m_marginColor1, m_marginColor2,
						   KImageEffect::DiagonalGradient );


   // QPixmap pixRet( m_basePix );
   QPixmap pixRet;
   pixRet.convertFromImage( ires );
   QPainter p( &pixRet );

   p.drawPixmap( m_thumbMargin, m_thumbMargin, preview );
   p.flush();
   // draw on pixmap

   return( pixRet );
   
}
