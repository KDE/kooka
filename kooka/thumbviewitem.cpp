/***************************************************************************
               thumbviewitem.cpp  - Thumbview item class
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

#include <kfiletreeviewitem.h>
#include <kfileitem.h>
#include <kfileiconview.h>
#include <kimageeffect.h>

#include "thumbview.h"
#include "thumbviewitem.h"

ThumbViewItem::ThumbViewItem(QIconView *parent, const QString &text,
			     const QPixmap &pixmap,
			     KFileItem *fi ) 
   :KFileIconViewItem( parent, text, pixmap,fi )   
{
   
}


