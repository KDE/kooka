/***************************************************************************
               thumbviewitem.h  - Thumbnailview items
                             -------------------                                         
    begin                : Tue Apr 24 2002
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

#ifndef __THUMBVIEWITEM_H__
#define __THUMBVIEWITEM_H__

#include <kiconview.h>
#include <kurl.h>
#include <kio/previewjob.h>
#include <kfileitem.h>
#include <kfileiconview.h>

class KFileTreeViewItem;

   
class ThumbViewItem: public KFileIconViewItem
{
public:
   ThumbViewItem( QIconView *parent,
		  const QString &text,
		  const QPixmap &pixmap,
		  KFileItem *fi );
   
   void setItemUrl( const KURL& u )
      { m_url = u; }
   KURL itemUrl() const
      { return m_url; }
   
private:
   KURL m_url;


};

#endif
