/************************************************************************
 *                                  *
 *  This file is part of Kooka, a KDE scanning/OCR application.     *
 *                                  *
 *  Copyright (C) 2013 Jonathan Marten <jjm@keelhaul.me.uk>     *
 *                                  *
 *  Kooka is free software; you can redistribute it and/or modify it    *
 *  under the terms of the GNU Library General Public License as    *
 *  published by the Free Software Foundation and appearing in the  *
 *  file COPYING included in the packaging of this file;  either    *
 *  version 2 of the License, or (at your option) any later version.    *
 *                                  *
 *  As a special exception, permission is given to link this program    *
 *  with any version of the KADMOS OCR/ICR engine (a product of     *
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting  *
 *  executable without including the source code for KADMOS in the  *
 *  source distribution.                        *
 *                                  *
 *  This program is distributed in the hope that it will be useful, *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of  *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   *
 *  GNU General Public License for more details.            *
 *                                  *
 *  You should have received a copy of the GNU General Public License   *
 *  along with this program;  see the file COPYING.  If not, write to   *
 *  the Free Software Foundation, Inc., 51 Franklin Street,     *
 *  Fifth Floor, Boston, MA 02110-1301, USA.                *
 *                                  *
 ************************************************************************/

#include "statusbarmanager.h"

#include <qlabel.h>

#include <QDebug>
#include <klocale.h>
#include <kstatusbar.h>
#include <kxmlguiwindow.h>

#include "imagecanvas.h"
#include "previewer.h"
#include "sizeindicator.h"

StatusBarManager::StatusBarManager(KXmlGuiWindow *mainWindow)
    : QObject(mainWindow)
{
    mStatusBar = mainWindow->statusBar();
#if 0 //QT5
    // Messages
    //QT5 mStatusBar->insertItem(i18nc("@info:status", "Ready"), StatusBarManager::Message, 1);
    //QT5 mStatusBar->setItemAlignment(StatusBarManager::Message, Qt::AlignLeft);

    // Image dimensions
    QString s = ImageCanvas::imageInfoString(2000, 2000, 48);
    initItem(s, StatusBarManager::ImageDims,
             i18nc("@info:tooltip", "The size of the image being viewed in the gallery"));

    // Preview dimensions
    s = Previewer::previewInfoString(500.0, 500.0, 1200, 1200);
    initItem(s, StatusBarManager::PreviewDims,
             i18nc("@info:tooltip", "The size of the selected area that will be scanned"));

    // Preview size
    mFileSize = new SizeIndicator(NULL);
    mFileSize->setMaximumWidth(100);
    mFileSize->setFrameStyle(QFrame::NoFrame);
    mFileSize->setToolTip(i18nc("@info:tooltip", "<qt>This is the uncompressed size of the scanned image. "
                                "It tries to warn you if you try to produce too big an image by "
                                "changing its background color."));
    mStatusBar->addPermanentWidget(mFileSize);
#endif
    mStatusBar->show();
}

StatusBarManager::~StatusBarManager()
{
}

void StatusBarManager::initItem(const QString &text,
                                StatusBarManager::Item item,
                                const QString &tooltip)
{
    //QT5 mStatusBar->insertPermanentItem(QString::null, item);

    // There is no official access within KStatusBar to the widgets created
    // for each item.  So to set a tool tip we have to search for them as
    // children of the status bar.
    //
    // The item just added will be the last QLabel child of the status bar.
    // It is always a QLabel, see KStatusBar::insertPermanentItem().
    // It is always the last, see QObject::children().

    if (!tooltip.isEmpty()) {
        const QList<QLabel *> childs = mStatusBar->findChildren<QLabel *>();
        Q_ASSERT(!childs.isEmpty());
        QLabel *addedItem = childs.last();
        addedItem->setToolTip(tooltip);
    }

    setStatus((text + "--"), item);         // allow some extra space
    //QT5 mStatusBar->setItemFixed(item);           // fix at current size
    //QT5 mStatusBar->changeItem(QString::null, item);  // clear initial contents
}

void StatusBarManager::setStatus(const QString &text, StatusBarManager::Item item)
{
    switch (item) {
    case StatusBarManager::ImageDims:
        //QT5 mStatusBar->changeItem(i18nc("@info:status", "Image: %1", text), item);
        break;

    case StatusBarManager::PreviewDims:
        //QT5 mStatusBar->changeItem(i18nc("@info:status", "Scan: %1", text), item);
        break;

    default:
        //QT5 mStatusBar->changeItem(text, item);
        break;
    }
}

void StatusBarManager::clearStatus(StatusBarManager::Item item)
{
    //QT5 mStatusBar->changeItem(QString::null, item);
}

void StatusBarManager::setFileSize(long size)
{
    mFileSize->setSizeInByte(size);
}
