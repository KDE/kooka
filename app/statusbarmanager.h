/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2013-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#ifndef STATUSBARMANAGER_H
#define STATUSBARMANAGER_H

#include <qobject.h>

class KXmlGuiWindow;
class QStatusBar;
class QLabel;
class SizeIndicator;

/**
 * @short Status bar for the Kooka application
 *
 * This class manages all of the status bar operations and display
 * for the Kooka application.
 *
 * It is a separate class and header file in order to resolve a
 * circular dependency between Kooka and KookaView.
 *
 * @author Jonathan Marten
 **/

class StatusBarManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Item IDs for the status bar.
     *
     **/
    enum Item {
        Message,					/**< Message line **/
        ImageDims,					/**< Image size indicator **/
        PreviewDims,					/**< Preview size indicator **/
        FileSize					/**< File size indicator **/
    };

    /**
     * Constructor.
     *
     * @param mainWindow The parent main window
     **/
    explicit StatusBarManager(KXmlGuiWindow *mainWindow);

    /**
     * Destructor.
     **/
    virtual ~StatusBarManager()				{}

public slots:
    /**
     * Set the text of a status bar item.
     *
     * @param text The new text
     * @param item The status bar item to set
     **/
    void setStatus(const QString &text, StatusBarManager::Item item = StatusBarManager::Message);

    /**
     * Clear the text of a status bar item.
     *
     * @param item The status bar item to clear
     **/
    void clearStatus(StatusBarManager::Item item = StatusBarManager::Message);

    /**
     * Set the file size in bytes for the graphical indicator.
     *
     * @param size File size in bytes
     **/
    void setFileSize(long size);

private:
    void setLabelText(QLabel *l, const QString &text);

private:
    QStatusBar *mStatusBar;
    int mMargin;

    QLabel *mMessageLabel;
    QLabel *mImageDimsLabel;
    QLabel *mPreviewDimsLabel;
    SizeIndicator *mFileSize;
};

#endif							// STATUSBARMANAGER_H
