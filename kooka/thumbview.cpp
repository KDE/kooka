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
#include <kprogress.h>

#include "thumbview.h"
#include "thumbview.moc"

#include "thumbviewitem.h"



ThumbView::ThumbView( QWidget *parent, const char *name )
   : QVBox( parent ),
     m_iconView(0),
     m_job(0)
{
   setMargin(3);
   m_pixWidth = 0;
   m_pixHeight = 0;
   m_thumbMargin = 5;
   m_iconView = new KIconView( this, name );
   m_progress = new KProgress( this );
   m_progress->hide();

   m_pixWidth  = 100;
   m_pixHeight = 100;

   readSettings();

   m_basePix.resize( QSize( m_pixWidth, m_pixHeight ) );
   m_basePix.fill();  // fills white per default TODO


   m_iconView->setItemsMovable( false );

   slSetBackGround();

   connect( m_iconView, SIGNAL( executed( QIconViewItem* )),
	    this, SLOT( slDoubleClicked( QIconViewItem* )));

   m_pendingJobs.setAutoDelete(false);
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
      m_iconView->setGridX(gX);
      m_iconView->setGridY(gY);
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

   m_iconView->setPaletteBackgroundPixmap ( bgPix );
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
   if( !itemUrl.equals( thumbDir, true ))
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

void ThumbView::slImageRenamed( KFileItem *kfit, const KURL& newUrl )
{
    const KURL url = kfit->url();

    for ( QIconViewItem *item = m_iconView->firstItem(); item; item = item->nextItem() )
    {
        ThumbViewItem *it=static_cast<ThumbViewItem*>( item );

        if( url == it->itemUrl() )
        {
            it->setItemUrl( newUrl );

            break;
        }
   }
}


void  ThumbView::slCheckForUpdate( KFileItem *kfit )
{
   if( ! kfit ) return;

   kdDebug(28000) << "Checking for update of thumbview!" << endl;

   KURL searchUrl = kfit->url();
   bool haveItem = false;

   /* iterate over all icon items and compare urls.
    * TODO: Check the parent url to avoid iteration over all */
   for ( QIconViewItem *item = m_iconView->firstItem(); item && !haveItem;
	 item = item->nextItem() )
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
   for ( QIconViewItem *item = m_iconView->firstItem(); item && !haveItem; item = item->nextItem() )
   {
      if( searchUrl == static_cast<ThumbViewItem*>(item)->itemUrl() )
      {
	 m_iconView->takeItem( item );
	 haveItem = true;
      }
   }
   kdDebug(28000) << "Deleting image from thumbview, result is " << haveItem << endl;
   return( haveItem );
}

void ThumbView::slImageDeleted( KFileItem *kfit )
{
   deleteImage( kfit );


   /*
    From a mail from Waldo pointing out two probs in Thumbview:

    1) KDirLister is the owner of the KFileItems it emits, this means
       that you must watch it's deleteItem() signal vigourously,
       otherwise you may end up with KFileItems that are already
       deleted. This burden is propagated to classes that use
       KDirLister, such as KFileIconView.

       This has a tendency to go wrong in combination with PreviewJob,
       because it stores a list of KFileItems while running. This has
       the potential to crash if the fileitems are being deleted
       during this time. The remedy is to make sure to remove
       fileitems that get deleted from the PreviewJob with
       PreviewJob::removeItem.

   */
   if( m_job )  /* is a job running? Remove the item from it if existing. */
   {
       m_job->removeItem( kfit );
   }

   /* check if it is in the pending list */
   m_pendingJobs.removeRef(kfit);
}


void ThumbView::slNewFileItems( const KFileItemList& items )
{
   kdDebug(28000) << "Creating thumbnails for fileItemList" << endl;

   /* Fill the pending jobs list. */
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

	 /* Create a new empty preview pixmap and store the pointer to it */
	 ThumbViewItem *newIconViewIt = new ThumbViewItem( m_iconView,
							   item->url().filename(),
							   createPixmap( p ),
							   item );

	 newIconViewIt->setItemUrl( item->url() );

	 /* tell the file item about the iconView-representation */
	 item->setExtraData( this, newIconViewIt );

	 m_pendingJobs.append( item );
      }
   }

   /*
     From a mail from Waldo Bastian pointing out problems with thumbview:

     2) I think you may end up creating two PreviewJob's in parallel
        when the slNewFileItems() function is called two times in
        quick succession. The current code doesn't seem to expect
        that, given the comment in slPreviewResult(). In the light of
        1) it might become fatal since you will not be able to call
        PreviewJob::removeItem on the proper job. I suggest to queue
        new items when a job is already running and start a new job
        once the first one is finished when there are any items left
        in the queue. Don't forget to delete items from the queue if
        they get deleted in the mean time.

        The strategy is as follows: In the global list m_pendingJobs
        the jobs to start are appended. Only if m_job is zero (no job
        is running) a job is started on the current m_pendingJobs list.
        The m_pendingJobs list is clear afterwords.
   */

   if( ! m_job && m_pendingJobs.count() > 0 )
   {
      /* Progress-Bar */
      m_progress->show();
      m_progress->setTotalSteps(m_pendingJobs.count());
      m_cntJobsStarted = 0;

      /* start a preview-job */
      m_job = KIO::filePreview(m_pendingJobs, m_pixWidth, m_pixHeight );

      if( m_job )
      {
	 connect( m_job, SIGNAL( result( KIO::Job * )),
		  this, SLOT( slPreviewResult( KIO::Job * )));
	 connect( m_job, SIGNAL( gotPreview( const KFileItem*, const QPixmap& )),
		  SLOT( slGotPreview( const KFileItem*, const QPixmap& ) ));

         m_pendingJobs.clear();

         /* KIO::Jo result is called in any way: Success, Failed, Error,
	  * thus connecting the failed is not really necessary.
	  */
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
   m_cntJobsStarted+=1;

   m_progress->setProgress(m_cntJobsStarted);

   // kdDebug(28000)<< "jobs-Counter: " << m_cntJobsStarted << endl;

}

void ThumbView::slPreviewResult( KIO::Job *job )
{
   if( job && job->error() > 0 )
   {
      kdDebug(28000) << "Thumbnail Creation ERROR: " << job->errorString() << endl;
      job->showErrorDialog( 0 );
   }

   if( job != m_job )
   {
      kdDebug(28000) << "Very obscure: Job finished is not mine!"  << endl;
   }
   /* finished */
   kdDebug(28000) << "Thumbnail job finished." << endl;
   m_cntJobsStarted = 0;
   m_progress->reset();
   m_progress->hide();
   m_job = 0L;

   /* maybe there is a new job to start because of pending items? */
   if( m_pendingJobs.count() > 0 )
   {
       slNewFileItems( KFileItemList() );  /* Call with an empty list */
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


void ThumbView::clear()
{
   if( m_job )
      m_job->kill( false /* not silently to get result-signal */ );
   m_iconView->clear();
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
