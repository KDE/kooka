/***************************************************************************
        dwmenuaction.h - dockwidget visibility switches to actions
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

#ifndef __DW_MENU_ACTION
#define __DW_MENU_ACTION
#include <kdockwidget.h>
#include <qstring.h>
#include <kaction.h>

/**
 * This class is just a helper class since the KDockWidget classes do not yet 
 * export KActions but only a QPopup-Pointer, which is quite useless in case
 * you have a xml-file driven gui.
 * This class provides Actions for show and hide parts of the GUI (dockwidgets)
 * Maybe that classes can be removed as soon the DockWidget know Actions
 */
class dwMenuAction:public KToggleAction
{
   Q_OBJECT
public:
   dwMenuAction( const QString& text,
		 const KShortcut& cut = KShortcut(),
		 KDockWidget *dw=0, QObject* parent = 0,
		 KDockMainWindow * mw=0, const char* name = 0 );
   virtual ~dwMenuAction();

private:
   KDockWidget *m_dw;
   KDockMainWindow *m_mw;
protected slots:
   void slotToggled(bool);
   void anDWChanged();
   void slotWidgetDestroyed();
};

#endif
