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
#include <kimageeffect.h>

#include "thumbview.h"

ThumbView::ThumbView( QWidget *parent, const char *name )
   : KIconView( parent, name ),
     m_pixWidth(100),
     m_pixHeight(140),
     m_thumbMargin(5)
{
   QImage ires = KImageEffect::unbalancedGradient( QSize( 2*m_thumbMargin+m_pixWidth,
							  2*m_thumbMargin+m_pixHeight ),
					 Qt::white, Qt::blue, KImageEffect::DiagonalGradient );
   m_basePix.convertFromImage( ires );

   setItemsMovable( false );
}

ThumbView::~ThumbView()
{
   
}

void ThumbView::slNewFileItems( const KFileItemList& items )
{
   kdDebug(28000) << "Creating thumbnails for fileItemList" << endl;

   
   QPixmap p;
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
	 QPixmap p;
	 /* Create a new empty preview pixmap and store the pointer to it */
	 KFileIconViewItem *newIconViewIt = new KFileIconViewItem( this,
								   item->url().filename(),
								   createPixmap(p),
								   item );

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
   KFileIconViewItem *item = static_cast<KFileIconViewItem*>(newFileItem->extraData( this ));

   if( ! item ) return;

   item->setPixmap( createPixmap(newPix) );
   
}

void ThumbView::slPreviewResult( KIO::Job * )
{
   
}


QPixmap ThumbView::createPixmap( const QPixmap& preview ) const
{
   QPixmap pixRet( m_basePix );

   QPainter p( &pixRet );

   p.drawPixmap( m_thumbMargin, m_thumbMargin, preview );
   p.flush();
   // draw on pixmap

   return( pixRet );
   
}
