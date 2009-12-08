/***************************************************************************
                             -------------------
    copyright            : (C) 2008 by Jonathan Marten
    email                : jjm@keelhaul.me.uk

    This wrapper class is used so that the popup menu can be a KMenu (which
    supports displaying a title) as opposed to a QMenu, and accessible from
    outside.

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

#include "thumbviewdiroperator.h"
#include "thumbviewdiroperator.moc"

#include <qevent.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmenu.h>
#include <kdiroperator.h>


ThumbViewDirOperator::ThumbViewDirOperator(const KUrl &url, QWidget *parent)
    : KDirOperator(url, parent)
{
    kDebug();

    m_menu = new KMenu(this);
    m_menu->addTitle(i18n("Thumbnails"));
}


void ThumbViewDirOperator::activatedMenu(const KFileItem &kfi, const QPoint &pos)
{
    updateSelectionDependentActions();
    emit contextMenuAboutToShow(kfi, m_menu);
    m_menu->exec(pos);
}
