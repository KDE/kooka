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

    void setItemUrl( const KURL& u );

    KURL itemUrl() const
      { return m_url; }

private:
   KURL m_url;


};

#endif
