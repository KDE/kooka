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

#ifndef DWMENUACTION_H
#define DWMENUACTION_H

#include <ktoggleaction.h>

class K3DockWidget;
class K3DockMainWindow;


/**
 * This class is just a helper class since the KDockWidget classes do not yet
 * export KActions but only a QPopup-Pointer, which is quite useless in case
 * you have a xml-file driven gui.
 * This class provides Actions for show and hide parts of the GUI (dockwidgets)
 * Maybe that classes can be removed as soon the DockWidget know Actions
 */

class dwMenuAction : public KToggleAction
{
    Q_OBJECT

public:
    dwMenuAction(const QString &text,
		 K3DockWidget *dw,
		 K3DockMainWindow *mw,
                 QObject *parent = NULL);
    virtual ~dwMenuAction();

private:
    K3DockWidget *m_dw;
    K3DockMainWindow *m_mw;

protected slots:
    void slotToggled(bool);
    void anDWChanged();
    void slotWidgetDestroyed();
};

#endif							// DWMENUACTION_H
