/**************************************************************************
			kooka.h  -  Main program class
                             -------------------                                         
    begin                : Sun Jan 16 2000                                           
    copyright            : (C) 2000 by Klaas Freitag                         
    email                : freitag@suse.de          
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/
#ifndef KOOKA_H
#define KOOKA_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <kapp.h>
#include <kmainwindow.h>
 


class KPrinter;
class KToggleAction;
class KookaView;

/**
 * This class serves as the main window for Kooka.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Klaas Freitag <freitag@suse.de>
 * @version 0.1
 */
class Kooka : public KMainWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    Kooka();

    /**
     * Default Destructor
     */
    virtual ~Kooka();

    /**
     * Use this method to load whatever file/URL you have
     */
    void load(const QString& url);

   /**
    * Startup, loads (at the moment) only the last displayed image
    **/
   void startup( void );
   
    
protected:
    /**
     * Overridden virtuals for Qt drag 'n drop (XDND)
     */
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);

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
   void filePrint();
   /* ImageViewer-Actions */
   
   void optionsShowToolbar();
   void optionsShowStatusbar();
   void optionsShowScanParams();
   void optionsShowPreviewer();
   void optionsConfigureKeys();
   void optionsConfigureToolbars();
   void optionsPreferences();

   void changeStatusbar(const QString& text);
   void cleanStatusbar(void) { changeStatusbar(""); }
   void changeCaption(const QString& text);

   // void fileSaveAs();

   void slMirrorVertical( void );
   void slMirrorHorizontal( void );
   void slMirrorBoth( void );

   void slRotateClockWise( void );
   void slRotateCounterClockWise( void );
   void slRotate180( void );

private:
   void setupAccel();
   void setupActions();

private:
   KookaView *m_view;

   KPrinter   *m_printer;
   KToggleAction *m_toolbarAction;
   KToggleAction *m_statusbarAction;
   KToggleAction *m_scanParamsAction;
   KToggleAction *m_previewerAction;
};

#endif // KOOKA_H
