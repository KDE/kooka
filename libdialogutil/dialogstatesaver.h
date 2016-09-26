/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2016 Jonathan Marten <jjm@keelhaul.me.uk>		*
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

#ifndef DIALOGSTATESAVER_H
#define DIALOGSTATESAVER_H

#include "libdialogutil_export.h"

class QDialog;
class QWidget;
class KConfigGroup;


/**
 * @short Save and restore the size and state of a dialog box.
 *
 * This class manages saving and restoring a dialog's size in the
 * application config file.  All that is necessary is to create a
 * DialogStateWatcher object in the dialog's constructor, passing the
 * dialog as a parameter.  If the dialog is a subclass of DialogBase
 * then a watcher and saver will be created automatically.
 *
 * The saver can be subclassed if necessary in order to save additional
 * information (e.g. the column states of a list view).  Since it does
 * not inherit QObject, the dialog itself can be the DialogStateSaver
 * to do its own saving and restoring, having access to its own internal
 * state.
 *
 * @author Jonathan Marten
 **/

class LIBDIALOGUTIL_EXPORT DialogStateSaver
{
public:
    /**
     * Constructor.
     *
     * @param pnt the parent dialog
     **/
    explicit DialogStateSaver(QDialog *pnt);

    /**
     * Destructor.
     **/
    virtual ~DialogStateSaver();

    /**
     * Set the default option of whether the size of dialog boxes
     * is saved when accepted and restored when shown.  This is an
     * application-wide setting which takes effect immediately.
     * The default is @c true.
     *
     * @param on Whether the size is to be saved/restored
     *
     * @note The setting is saved in the application's default configuration
     * file (as used by @c KSharedConfig::openConfig()) in a section named
     * by the dialog's object name.  If no object name is set then the
     * dialog's class name is used.
     *
     * @see KSharedConfig
     * @see QObject::objectName()
     **/
    static void setSaveSettingsDefault(bool on);

    /**
     * Save the parent dialog size to the application config file.
     *
     * This is called by the dialog state watcher and should not need
     * to be called explicitly.  It simply calls the virtual method
     * of the same name, which may be reimplemented in a subclass in order
     * to save other settings (e.g. the column states of a list view).
     **/
    void saveConfig() const;

    /**
     * Restore the dialog size from the application config file.
     *
     * This is called by the dialog state watcher and should not need
     * to be called explicitly.  It simply calls the virtual method
     * of the same name, which may be reimplemented in a subclass in order
     * to restore other settings (e.g. the column states of a list view).
     **/
    void restoreConfig();

    /**
     * Save the state of a window.
     *
     * The window need not be a dialog, therefore this can be used for
     * saving the state of any window.  The state is saved to a group
     * named as appropriate for the window.
     *
     * @param window window to save the state of
     **/
    static void saveWindowState(QWidget *window);

    /**
     * Save the state of a window.
     *
     * The window need not be a dialog, therefore this can be used for
     * saving the state of any window.  The state is saved to the
     * specified group.
     *
     * @param window window to save the state of
     * @param grp group to save the configuration to
     **/
    static void saveWindowState(QWidget *window, KConfigGroup &grp);

    /**
     * Restore the state of a window.
     *
     * The window need not be a dialog, therefore this can be used for
     * restoring the state of any window.  The state is restore from a group
     * named as appropriate for the window.
     *
     * @param window window to restore the state of
     **/
    static void restoreWindowState(QWidget *window);

    /**
     * Restore the state of a window.
     *
     * The window need not be a dialog, therefore this can be used for
     * restoring the state of any window.  The state is restored from
     * the specified group.
     *
     * @param window window to restore the state of
     * @param grp group to restore the configuration from
     **/
    static void restoreWindowState(QWidget *window, const KConfigGroup &grp);

protected:
    /**
     * Save the dialog size to the application config file.
     *
     * This may be reimplemented in a subclass if necessary, in order
     * to save other settings (e.g. the column states of a list view).
     * Call the base class implementation to save the dialog size.
     *
     * @param dialog dialog to save the state of
     * @param grp group to save the configuration to
     **/
    virtual void saveConfig(QDialog *dialog, KConfigGroup &grp) const;

    /**
     * Restore the dialog size from the application config file.
     *
     * This may be reimplemented in a subclass if necessary, in order
     * to restore other settings (e.g. the column states of a list view).
     * Call the base class implementation to restore the dialog size.
     *
     * @param dialog dialog to restore the state of
     * @param grp group to restore the configuration from
     **/
    virtual void restoreConfig(QDialog *dialog, const KConfigGroup &grp);

private:
    QDialog *mParent;
};

#endif							// DIALOGSTATESAVER_H
