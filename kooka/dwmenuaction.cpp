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
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/

#include "dwmenuaction.h"

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
