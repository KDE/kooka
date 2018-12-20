/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#ifndef KOOKA_H
#define KOOKA_H

#include <kxmlguiwindow.h>

#include "kookaview.h"

class KConfigGroup;
class KToggleAction;
class QAction;
class KActionMenu;


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
    explicit Kooka(const QByteArray &deviceToUse);

    /**
     * Default Destructor
     */
    ~Kooka() override;

    /**
     * Startup, loads (at the moment) only the last displayed image
     **/
    void startup();

protected:
    void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE;

    /**
     * Overridden virtuals for Qt drag 'n drop (XDND)
     */
    void dragEnterEvent(QDragEnterEvent *ev) Q_DECL_OVERRIDE;
    // virtual void dropEvent(QDropEvent *event);

    /**
     * This function is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfigGroup &grp) Q_DECL_OVERRIDE;

    /**
     * This function is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(const KConfigGroup &grp) Q_DECL_OVERRIDE;

    virtual void applyMainWindowSettings(const KConfigGroup &grp) Q_DECL_OVERRIDE;

protected slots:
    void slotUpdateScannerActions(bool haveConnection);
    void slotUpdateRectangleActions(bool haveSelection);
    void slotUpdateViewActions(KookaView::StateFlags state);
    void slotUpdateOcrResultActions(bool haveText);
    void slotOpenWithMenu();
    void slotUpdateReadOnlyActions(bool ro);
    void slotUpdateAutoSelectActions(bool isAvailable, bool isOn);

private slots:
    void filePrint();

    void optionsPreferences();
    void optionsOcrPreferences();

private:
    void setupAccel();
    void setupActions();

private:
    KookaView *m_view;

    KToggleAction *m_scanParamsAction;
    KToggleAction *m_previewerAction;

    QAction *m_saveOCRTextAction;
    int m_prefDialogIndex;

    QAction *scanAction;
    QAction *previewAction;
    QAction *photocopyAction;
    QAction *paramsAction;
    KToggleAction *autoselAction;
    QAction *ocrAction;
    QAction *ocrSelectAction;
    QAction *ocrSpellAction;

    QAction *newFromSelectionAction;
    QAction *scaleToWidthAction;
    QAction *scaleToHeightAction;
    QAction *scaleToOriginalAction;
    QAction *scaleToZoomAction;
    KToggleAction *keepZoomAction;
    QAction *mirrorVerticallyAction;
    QAction *mirrorHorizontallyAction;
    QAction *rotateCwAction;
    QAction *rotateAcwAction;
    QAction *rotate180Action;

    QAction *createFolderAction;
    QAction *saveImageAction;
    QAction *printImageAction;
    QAction *importImageAction;
    QAction *deleteImageAction;
    QAction *renameImageAction;
    QAction *unloadImageAction;
    QAction *propsImageAction;

    QAction *selectDeviceAction;
    QAction *addDeviceAction;

    KActionMenu *openWithMenu;

    bool m_imageChangeAllowed;
};

#endif                          // KOOKA_H
