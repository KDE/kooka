/***************************************************************************
        dwmenuaction.cpp - dockwidget visibility switches to actions
                             -------------------
    begin                : 16.07.2002
    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de

    $Id$
    Based on code from the from Joseph Wenninger (kate project)
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

#include "dwmenuaction.h"
#include "dwmenuaction.moc"
//-------------------------------------

dwMenuAction::dwMenuAction( const QString& text, const KShortcut& cut,
			    KDockWidget *dw,QObject* parent,
			    KDockMainWindow *mw, const char* name )
    :KToggleAction(text,cut,parent,name),m_dw(dw),m_mw(mw)
{
    connect(this,SIGNAL(toggled(bool)),this,SLOT(slotToggled(bool)));
    connect(m_dw->dockManager(),SIGNAL(change()),this,SLOT(anDWChanged()));
    connect(m_dw,SIGNAL(destroyed()),this,SLOT(slotWidgetDestroyed()));
    setChecked(m_dw->mayBeHide());
}


dwMenuAction::~dwMenuAction(){;}

void dwMenuAction::anDWChanged()
{
    if (isChecked() && m_dw->mayBeShow()) setChecked(false);
    else if ((!isChecked()) && m_dw->mayBeHide()) setChecked(true);
}


void dwMenuAction::slotToggled(bool t)
{

    if ((!t) && m_dw->mayBeHide() ) m_dw->undock();
        else
    if ( t && m_dw->mayBeShow() ) m_mw->makeDockVisible(m_dw);

}


void dwMenuAction::slotWidgetDestroyed()
{
    unplugAll();
    deleteLater();
}


/* END */
