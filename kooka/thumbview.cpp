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
#include <kglobal.h>
#include <kstandarddirs.h>

#include "thumbview.h"
#include "thumbviewitem.h"



ThumbView::ThumbView( QWidget *parent, const char *name )
   : KIconView( parent, name )
{
   readSettings();

   m_basePix.resize( QSize( m_pixWidth, m_pixHeight ) );
   m_basePix.fill();  // fills white per default TODO

   
   setItemsMovable( false );

   slSetBackGround();
   
   connect( this, SIGNAL( executed( QIconViewItem* )),
	    this, SLOT( slDoubleClicked( QIconViewItem* )));
}

ThumbView::~ThumbView()
{
   saveConfig();
}

bool ThumbView::readSettings()
{
   KConfig *cfg = KGlobal::config();
   cfg->setGroup( THUMB_GROUP );
   bool dirty = false;
   
   QColor color;
   color = cfg->readColorEntry( MARGIN_COLOR1, &(colorGroup().base()));
   if( color != m_marginColor1 )
   {
      dirty = true;
      m_marginColor1 = color;
   }

   color = cfg->readColorEntry( MARGIN_COLOR2, &(colorGroup().foreground()));
   if( color != m_marginColor2 )
   {
      dirty = true;
      m_marginColor2 = color;
   }

   int value;
   bool sizeDirty = false;
   value = cfg->readNumEntry( THUMB_MARGIN, 5 );
   if( value != m_thumbMargin )
   {
      sizeDirty = true;
      m_thumbMargin = value;
   }

   value = cfg->readNumEntry( PIXMAP_WIDTH, 100 );
   if( value != m_pixWidth || m_pixWidth == 0 )
   {
      sizeDirty  = true;
      m_pixWidth = value;
   }
   
   value = cfg->readNumEntry( PIXMAP_HEIGHT, 120 );
   if( value != m_pixHeight || m_pixHeight == 0 )
   {
      sizeDirty  = true;
      m_pixHeight = value;
   }

   if( sizeDirty )
   {
      int gX = 2*m_thumbMargin+m_pixWidth+10;
      int gY = 2*m_thumbMargin+m_pixHeight+10;
      setGridX(gX);
      setGridY(gY);
      kdDebug(28000) << "Setting Grid " << gX << " - " << gY << endl;
   }

   KStandardDirs stdDir;
   QString newBgImg = cfg->readEntry( BG_WALLPAPER, stdDir.findResource( "data", STD_TILE_IMG ) );
   
   if( m_bgImg != newBgImg )
   {
      m_bgImg = newBgImg;
      slSetBackGround();
   }

   return (sizeDirty || dirty);
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

void ThumbView::slSetBackGround( )
{
   QPixmap bgPix;
   if( m_bgImg.isEmpty())
   {
      bgPix.resize( QSize(16, 16));
      bgPix.fill( QPixmap::blue );
   }
   else
   {
      bgPix.load( m_bgImg );
   }
   
   setPaletteBackgroundPixmap ( bgPix ); 

}

void ThumbView::slImageChanged( KFileItem *kfit )
{
   if( ! kfit ) return;
   // kdDebug(28000) << "changes to one thumbnail!" << endl;

   KURL thumbDir = currentDir();
   KURL itemUrl = kfit->url();

   /* delete filename */
   itemUrl.setFileName( QString());
   if( !itemUrl.cmp( thumbDir, true ))
   {
      // kdDebug(28000) << "returning, because directory does not match: " << itemUrl.prettyURL() << endl;
      // kdDebug(28000) << "and my URL: " << thumbDir.prettyURL() << endl;
      return;
   }
      
   if( deleteImage( kfit ))
   {
      kdDebug(28000) << "was changed, deleted first!" << endl;
   }
   /* Trigger a new reading */
   KFileItemList li;
   li.append( kfit );
   slNewFileItems( li );
}


void  ThumbView::slCheckForUpdate( KFileItem *kfit )
{
   if( ! kfit ) return;

   kdDebug(28000) << "Checking for update of thumbview!" << endl;
   
   KURL searchUrl = kfit->url();
   bool haveItem = false;

   /* iterate over all icon items and compare urls.
    * TODO: Check the parent url to avoid iteration over all */
   for ( QIconViewItem *item = firstItem(); item && !haveItem; item = item->nextItem() )
   {
      if( searchUrl == static_cast<ThumbViewItem*>(item)->itemUrl() )
      {
	 haveItem = true;
      }
   }

   /* if we still do not have the item, it is not in the thumbview. */
   if( ! haveItem )
   {
      KFileItemList kfiList;

      kfiList.append( kfit );
      slNewFileItems( kfiList );
   }
   
}


bool ThumbView::deleteImage( KFileItem *kfit )
{
   if( ! kfit ) return false;

   
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
   kdDebug(28000) << "Deleting image from thumbview, result is " << haveItem << endl;
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
	 QPixmap mime( item->pixmap(0) );

	 if( p.width() > mime.width() && p.height() > mime.height() )
	 {
	    QPainter paint( &p );
	    paint.drawPixmap( (p.width()-mime.width())/2,
			      (p.height()-mime.height())/2,
			      mime );
	    paint.flush();
	 }
	 kdDebug( 28000) << "Base image size " << m_basePix.size().width() << endl;
	 /* Create a new empty preview pixmap and store the pointer to it */
	 ThumbViewItem *newIconViewIt = new ThumbViewItem( this,
							   item->url().filename(),
							   createPixmap( p ),
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

   item->setPixmap( createPixmap(newPix) );
   
}

void ThumbView::slPreviewResult( KIO::Job *job )
{
   if( job && job->error() > 0 )
   {
      kdDebug(28000) << "Thumbnail Creation ERROR: " << job->errorString() << endl;
      job->showErrorDialog( 0 );
   }
}


QPixmap ThumbView::createPixmap( const QPixmap& preview ) const
{
   QImage ires = KImageEffect::unbalancedGradient( QSize( 2*m_thumbMargin+ preview.width(),
							  2*m_thumbMargin+ preview.height()),
						   m_marginColor1, m_marginColor2,
						   KImageEffect::DiagonalGradient );


   QPixmap pixRet;
   pixRet.convertFromImage( ires );
   QPainter p( &pixRet );

   p.drawPixmap( m_thumbMargin, m_thumbMargin, preview );
   p.flush();
   // draw on pixmap

   return( pixRet );
}


void ThumbView::saveConfig()
{
   KConfig *cfg = KGlobal::config();
   cfg->setGroup( THUMB_GROUP );

   cfg->writeEntry( MARGIN_COLOR1, m_marginColor1 );
   cfg->writeEntry( MARGIN_COLOR2, m_marginColor2 );
   cfg->writeEntry( PIXMAP_WIDTH, m_pixWidth );
   cfg->writeEntry( PIXMAP_HEIGHT, m_pixHeight );
   cfg->writeEntry( THUMB_MARGIN, m_thumbMargin );


}
