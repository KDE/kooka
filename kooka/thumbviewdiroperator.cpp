/***************************************************************************
                             -------------------
    copyright            : (C) 2008 by Jonathan Marten
    email                : jjm@keelhaul.me.uk

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

#include <kdebug.h>
#include <kfileitem.h>
#include <klocale.h>
#include <kactionclasses.h>
#include <kpopupmenu.h>
#include <kdiroperator.h>

#include "thumbviewdiroperator.h"
#include "thumbviewdiroperator.moc"


ThumbViewDirOperator::ThumbViewDirOperator(const KURL &url,
                                           QWidget *parent,const char *name)
    : KDirOperator(url,parent,name)
{
    kdDebug(28000) << k_funcinfo << endl;

    setupMenu(0);					// don't want the built-in actions

    m_menu = static_cast<KActionMenu*>(actionCollection()->action("popupMenu"));
    if (m_menu==NULL) kdDebug(28000) << k_funcinfo << "no popup menu!" << endl;
    else m_menu->popupMenu()->insertTitle(i18n("Thumbnails"));
}


void ThumbViewDirOperator::activatedMenu(const KFileItem *kfi,const QPoint &pos)
{
    if (m_menu!=NULL) m_menu->popup(pos);
}


QPopupMenu *ThumbViewDirOperator::contextMenu() const
{
    return (m_menu!=NULL ? static_cast<QPopupMenu *>(m_menu->popupMenu()) : NULL);
}
