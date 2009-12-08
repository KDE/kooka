/***************************************************** -*- mode:c++; -*- ***
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

#ifndef THUMBVIEWDIROPERATOR_H
#define THUMBVIEWDIROPERATOR_H

#include <kdiroperator.h>

class KFileItem;
class KMenu;

class QPoint;
class QMouseEvent;


class ThumbViewDirOperator : public KDirOperator
{
    Q_OBJECT

public:
    ThumbViewDirOperator(const KUrl &url = KUrl(), QWidget *parent = NULL);
    virtual ~ThumbViewDirOperator() {};

    KMenu *contextMenu() const { return (m_menu); }

protected slots:
    void activatedMenu(const KFileItem &kfi, const QPoint &pos);

signals:
    void contextMenuAboutToShow(const KFileItem &kfi, QMenu *menu);

private:
    KMenu *m_menu;
};


#endif							// THUMBVIEWDIROPERATOR_H
