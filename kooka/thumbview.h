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

class QPixmap;

   
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
private:
   QPixmap createPixmap( const QPixmap& ) const;

   KURL    m_currentDir;
   QPixmap m_basePix;
   int     m_pixWidth;
   int     m_pixHeight;
   int     m_thumbMargin;
   QColor  m_marginColor1;
   QColor  m_marginColor2;
};

#endif
