/**************************************************************************
			kooka.h  -  Main program class
                             -------------------
    begin                : Sun Jan 16 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : freitag@suse.de
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

#ifndef KOOKA_H
#define KOOKA_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kapplication.h>
#include <kmainwindow.h>
#include <kdockwidget.h>
#include <kparts/dockmainwindow.h>

#define KOOKA_STATE_GROUP "State"
#define PREFERENCE_DIA_TAB "PreferencesTab"

class KPrinter;
class KToggleAction;
class KActionMenu;
class KookaView;

/**
 * This class serves as the main window for Kooka.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Klaas Freitag <freitag@suse.de>
 * @version 0.1
 */
class Kooka : public KParts::DockMainWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    Kooka(const QCString& deviceToUse);

    /**
     * Default Destructor
     */
    ~Kooka();

   /**
    * Startup, loads (at the moment) only the last displayed image
    **/
   void startup( void );


protected:
    /**
     * Overridden virtuals for Qt drag 'n drop (XDND)
     */
    virtual void dragEnterEvent(QDragEnterEvent *event);
    // virtual void dropEvent(QDropEvent *event);

    /**
     * This function is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfig *);

    /**
     * This function is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(KConfig *);


private slots:

    void createMyGUI( KParts::Part* );

   void filePrint();
   /* ImageViewer-Actions */

   void optionsShowScanParams();
   void optionsShowPreviewer();
   void optionsConfigureToolbars();
   void optionsPreferences();

   void changeStatusbar(const QString& text);
   void cleanStatusbar(void) { changeStatusbar(""); }
   void changeCaption(const QString& text);
   void newToolbarConfig();

   // void fileSaveAs();

   void slMirrorVertical( void );
   void slMirrorHorizontal( void );
   void slMirrorBoth( void );

   void slRotateClockWise( void );
   void slRotateCounterClockWise( void );
   void slRotate180( void );

   void slEnableWarnings();

private:
   void setupAccel();
   void setupActions();

private:
   KookaView *m_view;

   KPrinter   *m_printer;
   KToggleAction *m_scanParamsAction;
   KToggleAction *m_previewerAction;
   KActionMenu   *m_settingsShowDocks;

    KAction      *m_saveOCRTextAction;
   int m_prefDialogIndex;
};

#endif // KOOKA_H
