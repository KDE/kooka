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

#include <kxmlguiwindow.h>

#define KOOKA_STATE_GROUP "State"
#define PREFERENCE_DIA_TAB "PreferencesTab"

class KConfigGroup;
class KPrinter;
class KToggleAction;
class KAction;
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
class Kooka : public KXmlGuiWindow
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     */
    Kooka(const QByteArray &deviceToUse);

    /**
     * Default Destructor
     */
    ~Kooka();

   /**
    * Startup, loads (at the moment) only the last displayed image
    **/
   void startup();


protected:
    virtual void closeEvent(QCloseEvent *ev);

    /**
     * Overridden virtuals for Qt drag 'n drop (XDND)
     */
    virtual void dragEnterEvent(QDragEnterEvent *ev);
    // virtual void dropEvent(QDropEvent *event);

    /**
     * This function is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfigGroup &grp);

    /**
     * This function is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(const KConfigGroup &grp);

protected slots:
    void slotUpdateScannerActions(bool haveConnection);
    void slotUpdateRectangleActions(bool haveSelection);
    void slotUpdateGalleryActions(bool isDir,int howmanySelected);
    void slotUpdateLoadedActions(bool isLoaded,bool isDir);
    void slotUpdateOcrResultActions(bool haveText);
    void slotOpenWithMenu();

private slots:

//    void createMyGUI( KParts::Part* );

   void filePrint();
   /* ImageViewer-Actions */

//   void optionsConfigureToolbars();
   void optionsPreferences();
   void optionsOcrPreferences();

   void changeStatusbar(const QString& text);
   void cleanStatusbar(void) { changeStatusbar(""); }
   void changeCaption(const QString& text);
//   void newToolbarConfig();

   void slotMirrorVertical( void );
   void slotMirrorHorizontal( void );

   void slotRotateClockWise( void );
   void slotRotateCounterClockWise( void );
   void slotRotate180( void );

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

   KAction *scanAction;
   KAction *previewAction;
   KAction *photocopyAction;
   KAction *paramsAction;
   KAction *ocrAction;
   KAction *ocrSelectAction;
   KAction *ocrSpellAction;

   KAction *newFromSelectionAction;
   KAction *scaleToWidthAction;
   KAction *scaleToHeightAction;
   KAction *scaleToOriginalAction;
   KAction *mirrorVerticallyAction;
   KAction *mirrorHorizontallyAction;
   KAction *rotateCwAction;
   KAction *rotateAcwAction;
   KAction *rotate180Action;

   KAction *createFolderAction;
   KAction *saveImageAction;
   KAction *printImageAction;
   KAction *importImageAction;
   KAction *deleteImageAction;
   KAction *renameImageAction;
   KAction *unloadImageAction;
   KAction *propsImageAction;

   KActionMenu *openWithMenu;
};

#endif // KOOKA_H
