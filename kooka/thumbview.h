/***************************************************************************
               thumbview.h  - Class to display thumbnailed images
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

#ifndef __THUMBVIEW_H__
#define __THUMBVIEW_H__

#include <qwidget.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qcolor.h>
#include <qvbox.h>

#include <kiconview.h>
#include <kurl.h>
#include <kio/previewjob.h>
#include <kfileitem.h>
#include <kfileiconview.h>

/* KConfig group definitions */
#define MARGIN_COLOR1 "MarginColor1"
#define MARGIN_COLOR2 "MarginColor2"
#define PIXMAP_WIDTH  "pixmapWidth"
#define PIXMAP_HEIGHT "pixmapHeight"
#define THUMB_MARGIN  "thumbnailMargin"
#define THUMB_GROUP   "thumbnailView"
#define BG_WALLPAPER  "BackGroundTile"
#define STD_TILE_IMG  "kooka/pics/thumbviewtile.png"

class QPixmap;
class QListViewItem;
class KProgress;
class KIO::PreviewJob;

class ThumbView: public QVBox /* KIconView */
{
   Q_OBJECT

public:

   ThumbView( QWidget *parent, const char *name=0 );
   ~ThumbView();

   void setCurrentDir( const KURL& s)
      { m_currentDir = s; }
   KURL currentDir( ) const
      { return m_currentDir; }

   QSize tumbSize( ) const
      {
	 return( QSize( m_pixWidth, m_pixHeight ));
      }

   int thumbMargin() const
      {
	 return m_thumbMargin;
      }
public slots:
   void slSetThumbSize( int w, int h )
      {
	 m_pixWidth  = w;
	 m_pixHeight = h;
      }
   void slSetThumbSize( const QSize& s )
      {
	 m_pixWidth  = s.width();
	 m_pixHeight = s.height();
      }

   void slSetThumbMargin( int m )
      {
	 m_thumbMargin = m;
      }

   void slNewFileItems( const KFileItemList& );
   void slGotPreview( const KFileItem*, const QPixmap& );
   void slPreviewResult( KIO::Job* );

   /**
    *  This connects to the IconView's executed signal and tells the packager
    *  to select the image
    */
   void slDoubleClicked( QIconViewItem* );

   /**
    *  indication that a image changed, needs to be reloaded.
    */
   void slImageChanged( KFileItem * );
   void slImageDeleted( KFileItem * );
   void slSetBackGround( );
   void slCheckForUpdate( KFileItem* );
   bool readSettings();
   void clear();
protected:

   void saveConfig();

signals:
   /**
    * selects a QListViewItem from the thumbnail. This signal only makes
    * sense if connected to a ScanPackager.
    */
   void selectFromThumbnail( const KURL& );

private:
   QPixmap createPixmap( const QPixmap& ) const;

   bool    deleteImage( KFileItem* );
   KIconView *m_iconView;
   KProgress *m_progress;

   KURL    m_currentDir;
   QPixmap m_basePix;
   int     m_pixWidth;
   int     m_pixHeight;
   int     m_thumbMargin;
   QColor  m_marginColor1;
   QColor  m_marginColor2;
   QString m_bgImg;
   int     m_cntJobsStarted;
   KIO::PreviewJob *m_job;
};

#endif
