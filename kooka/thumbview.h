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

#ifndef __THUMBVIEW_H__
#define __THUMBVIEW_H__

#include <qwidget.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qcolor.h>

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
   
class ThumbView: public KIconView
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
   
   KURL    m_currentDir;
   QPixmap m_basePix;
   int     m_pixWidth;
   int     m_pixHeight;
   int     m_thumbMargin;
   QColor  m_marginColor1;
   QColor  m_marginColor2;
   QString m_bgImg;
   
};

#endif
