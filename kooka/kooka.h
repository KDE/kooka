#ifndef KOOKA_H
#define KOOKA_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <kapp.h>
#include <kmainwindow.h>
 
#include "kookaview.h"

class QPrinter;
class KToggleAction;

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

protected:
    /**
     * Overridden virtuals for Qt drag 'n drop (XDND)
     */
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);

protected:
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
   void optionsShowToolbar();
   void optionsShowStatusbar();
   void optionsShowScanParams();
   void optionsShowPreviewer();
   void optionsConfigureKeys();
   void optionsConfigureToolbars();
   void optionsPreferences();

   void changeStatusbar(const QString& text);
   void changeCaption(const QString& text);

private:
   void setupAccel();
   void setupActions();

private:
   KookaView *m_view;

   QPrinter   *m_printer;
   KToggleAction *m_toolbarAction;
   KToggleAction *m_statusbarAction;
   KToggleAction *m_scanParamsAction;
   KToggleAction *m_previewerAction;
};

#endif // KOOKA_H
